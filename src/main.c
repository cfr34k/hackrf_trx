#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <poll.h>
#include <unistd.h>
#include <sys/sendfile.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>

#include <errno.h>
#include <string.h>

#include <libhackrf/hackrf.h>

#include "logger.h"

#include "config.h"

#define NFDS 1

enum TRXMode {AUTO, RX, TX};

enum TRXMode mode = AUTO, nextMode = AUTO;
sem_t sem_mode_switch;

int running = 1;

int8_t last_dc_byte_i = 0;
int8_t last_dc_byte_q = 0;
int8_t dc_check_buffer[DC_CHECK_BUFFER_LEN];
ssize_t dc_check_buffer_used = 0;

void sighandler(int sig)
{
	LOG(LVL_INFO, "Caught signal %i, shutting down.", sig);
	running = 0;
	sem_post(&sem_mode_switch);
}

int8_t check_for_dc(int8_t *data, size_t len)
{
	for(size_t i = 0; i < len; i+=2) {
		int8_t byte_i = data[i];
		int8_t byte_q = data[i+1];

		int8_t delta_i = abs(byte_i - last_dc_byte_i);
		int8_t delta_q = abs(byte_q - last_dc_byte_q);

		if(delta_i > DC_CHECK_TOLERANCE || delta_q > DC_CHECK_TOLERANCE) {
			return 0;
		}
	}

	return 1;
}

int rx_callback(hackrf_transfer *transfer)
{
	static ssize_t total_written = 0;

	int ret;
	ssize_t total_size = 0;
	ssize_t loops = 0;

	LOG(LVL_DEBUG, "RX callback called - data available: %i bytes.", transfer->valid_length);

	struct pollfd fd = {STDIN_FILENO, POLLIN, 0};

	// TX switch check
	do {
		LOG(LVL_DEBUG, "RX Mode: checking if TX should be entered.");
		ret = poll(&fd, 1, 1);
		if(ret > 0) {
			// data available on STDIN -> check for DC
			dc_check_buffer_used = read(STDIN_FILENO, dc_check_buffer, DC_CHECK_BUFFER_LEN);

			if(dc_check_buffer_used < 0) {
				LOG(LVL_ERR, "read(%i, %i) failed: %s", STDIN_FILENO, strerror(errno));
			} else if(dc_check_buffer_used == 0) {
				LOG(LVL_WARN, "Input stream shut down.");
				return -1;
			} else {
				total_size += dc_check_buffer_used;
				if(!check_for_dc(dc_check_buffer, dc_check_buffer_used)) {
					// not DC -> switch to TX
					nextMode = TX;
					sem_post(&sem_mode_switch);
					return 0;
				}
			}
		} else if(dc_check_buffer_used < 0) {
			LOG(LVL_ERR, "poll() failed for STDIN.");
			return -1;
		}
		loops++;
	} while(ret > 0 && total_size < transfer->valid_length && loops < DC_CHECK_MAX_LOOPS);

	// output availability check
	fd.fd = STDOUT_FILENO;
	fd.events = POLLOUT;

	ret = poll(&fd, 1, 10); // wait 10 milliseconds for data
	if(ret == 0) {
		// cannot write to rx pipe
		LOG(LVL_WARN, "Output not ready. Samples are lost.");
		return 0;
	} else if(ret < 0) {
		LOG(LVL_ERR, "poll() failed for STDOUT.");
		return -1;
	}

	ret = write(STDOUT_FILENO, transfer->buffer, transfer->valid_length);

	total_written += ret;
	LOG(LVL_DEBUG, "write() returned %i bytes (total %i).", ret, total_written);

	if(ret < 0) {
		if(errno == EAGAIN) {
			LOG(LVL_WARN, "RX pipe is full");
		} else {
			LOG(LVL_ERR, "write(%i, %i) failed: %s", STDOUT_FILENO, transfer->valid_length);
		}
	}

	return 0;
}

int tx_callback(hackrf_transfer *transfer)
{
	static ssize_t dc_transmitted = 0;

	size_t prefill = (transfer->valid_length < dc_check_buffer_used) ?
		transfer->valid_length : dc_check_buffer_used;

	ssize_t bytes_to_read = transfer->valid_length;
	ssize_t bytes_read = prefill;
	ssize_t ret;

	memcpy(transfer->buffer, dc_check_buffer, prefill);
	dc_check_buffer_used = 0;

	LOG(LVL_DEBUG, "TX callback called - to send: %i bytes.", bytes_to_read);

	do {
		struct pollfd fd = {STDIN_FILENO, POLLIN, 0};

		ret = poll(&fd, 1, 10); // wait 10 milliseconds for data
		if((ret == 0) || (dc_transmitted > MAX_DC_BYTES_TX)) {
			// no data available, fill remaining buffer with zeros...
			size_t fillsize = bytes_to_read - bytes_read;

			memset(transfer->buffer + bytes_read, 0, fillsize);
			LOG(LVL_WARN, "TX stopping: %i bytes filled with zeros.", fillsize);

			// ... reset DC counter ...
			dc_transmitted = 0;

			// and switch to RX
			nextMode = RX;
			sem_post(&sem_mode_switch);
			break;
		} else if(ret < 0) {
			LOG(LVL_ERR, "poll() failed for TX pipe.");
			break;
		} else { // data available
			ret = read(STDIN_FILENO, transfer->buffer + bytes_read, bytes_to_read - bytes_read);

			for(ssize_t i = 0; i < ret; i+=2) {
				int8_t byte_i = transfer->buffer[bytes_read + i];
				int8_t byte_q = transfer->buffer[bytes_read + i + 1];

				int8_t delta_i = abs(byte_i - last_dc_byte_i);
				int8_t delta_q = abs(byte_q - last_dc_byte_q);

				if(delta_i <= DC_CHECK_TOLERANCE && delta_q <= DC_CHECK_TOLERANCE) {
					dc_transmitted++;
				} else {
					dc_transmitted = 0;

					last_dc_byte_i = byte_i;
					last_dc_byte_q = byte_q;
				}
			}

			if(ret < 0) {
				LOG(LVL_ERR, "read(%i, %i) failed: %s", STDIN_FILENO, transfer->valid_length);
			} else if(ret == 0) {
				LOG(LVL_WARN, "Input stream shut down. Terminating");
				running = 0;
				sem_post(&sem_mode_switch);
				break;
			} else {
				bytes_read += ret;
				LOG(LVL_DEBUG, "read %i bytes, %i DC so far", ret, dc_transmitted);
			}
		}
	} while(ret > 0 && bytes_read < bytes_to_read);

	return 0;
}

int setup_hackrf(hackrf_device **hackrf, enum TRXMode trxmode)
{
	int result;
	uint64_t center_freq;

	result = hackrf_open(hackrf);
	if(result != HACKRF_SUCCESS) {
		LOG(LVL_ERR, "hackrf_open() failed: %s (%d)", hackrf_error_name(result), result);
		return -1;
	}

	// set up parameters
	switch(trxmode) {
		case TX:
			center_freq = CENTER_FREQ_TX;
			break;

		case RX:
		default:
			center_freq = CENTER_FREQ_RX;
			break;
	}

	result = hackrf_set_freq(*hackrf, center_freq);
	if(result != HACKRF_SUCCESS) {
		LOG(LVL_ERR, "hackrf_set_freq() failed: %s (%d)", hackrf_error_name(result), result);
		goto fail;
	}

	result = hackrf_set_sample_rate(*hackrf, SAMP_RATE);
	if(result != HACKRF_SUCCESS) {
		LOG(LVL_ERR, "hackrf_set_sample_rate() failed: %s (%d)", hackrf_error_name(result), result);
		goto fail;
	}

	return 0;

fail:
	hackrf_close(*hackrf);
	return -1;
}

int setup_mode(hackrf_device **hackrf)
{
	struct pollfd fds[NFDS] = {
		{STDIN_FILENO, POLLIN, 0}
	};

	int result;

	// shutdown previous mode
	switch(mode) {
		case TX:
			LOG(LVL_DEBUG, "Stopping TX");
			hackrf_stop_tx(*hackrf);
			break;

		case RX:
			LOG(LVL_DEBUG, "Stopping RX");
			hackrf_stop_rx(*hackrf);
			break;

		default:
			// do nothing
			break;
	}

	if(mode == RX || mode == TX) {
		hackrf_close(*hackrf);
		*hackrf = NULL;
	}

	/*
	struct timespec sleeptime = {0, 10000000}; // 10 ms
	nanosleep(&sleeptime, NULL);
	*/

	mode = AUTO;

	if(setup_hackrf(hackrf, nextMode) < 0) {
		LOG(LVL_ERR, "Cannot set up hackrf.");
		return -EINVAL;
	}

	// decide on AUTO mode
	if(nextMode == AUTO) {
		result = poll(fds, NFDS, 10); // wait 10 milliseconds for data

		if(result > 0) {
			// data available on STDIN -> TX mode
			nextMode = TX;
			LOG(LVL_DEBUG, "Auto mode decided: TX");
		} else if(result == 0) {
			// timeout on STDIN -> RX mode
			nextMode = RX;
			LOG(LVL_DEBUG, "Auto mode decided: RX");
		} else { // -1 -> error
			LOG(LVL_ERR, "poll(STDIN) failed: %s", strerror(errno));
			return -errno;
		}
	}

	// set up new mode
	switch(nextMode) {
		case TX:
			LOG(LVL_DEBUG, "Starting TX");
			hackrf_set_txvga_gain(*hackrf, TXVGA_GAIN);
			hackrf_start_tx(*hackrf, tx_callback, NULL);
			break;

		case RX:
			LOG(LVL_DEBUG, "Starting RX");
			hackrf_start_rx(*hackrf, rx_callback, NULL);
			break;

		default:
			// do nothing
			break;
	}

	mode = nextMode;

	return 0;
}

int main(void)
{
	int result = 0;
	int ret = EXIT_FAILURE; // main() return value

	hackrf_device *hackrf = NULL;

	logger_init();

	// create mode switch semaphore
	if(sem_init(&sem_mode_switch, 0, 0) == -1) {
		LOG(LVL_ERR, "sem_init() failed: %s", strerror(errno));
		goto fail_sem;
	}

	// init libhackrf
	result = hackrf_init();
	if(result != HACKRF_SUCCESS) {
		LOG(LVL_ERR, "hackrf_init() failed: %s (%d)", hackrf_error_name(result), result);
		goto fail_hackrf_init;
	}

	// set up signal handling
	signal(SIGINT, sighandler);
	signal(SIGTERM, sighandler);

	while(running) {
		if((result = setup_mode(&hackrf)) < 0) {
			LOG(LVL_ERR, "setup_mode() failed: %s", strerror(-result));
			goto fail;
		}

		if(sem_wait(&sem_mode_switch) == -1) {
			if(errno != EINTR) {
				LOG(LVL_ERR, "sem_wait() failed: %s", strerror(errno));
				goto fail;
			}
		}
	}

	LOG(LVL_INFO, "Shutting down...");

	// shut down stream
	switch(mode) {
		case RX:
			hackrf_stop_rx(hackrf);
			break;

		case TX:
			hackrf_stop_tx(hackrf);
			break;

		default:
			// do nothing
			break;
	}

	ret = EXIT_SUCCESS;

fail:
	if(hackrf) {
		hackrf_close(hackrf);
	}

//fail_hackrf_open:
	hackrf_exit();

fail_hackrf_init:
	sem_destroy(&sem_mode_switch);

fail_sem:
	return ret;
}
