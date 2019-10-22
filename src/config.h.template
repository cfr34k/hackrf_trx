#ifndef CONFIG_H
#define CONFIG_H

/* Sample rate */
#define SAMP_RATE 2400000

/* Center frequencies */
#define CENTER_FREQ_TX 438050000
#define CENTER_FREQ_RX (CENTER_FREQ_TX + 100000)

/* Gain settings */
#define TXVGA_GAIN 0 //47

#define TXPA_ENABLE 0

/* Number of repeated TX values before a switch to RX occurs */
#define MAX_DC_BYTES_TX ((1L<<18) * 14 / 10)

/* Max. iterations of DC check loop. This is used to drain the input fifo */
#define DC_CHECK_MAX_LOOPS 128

/* Tolerance for DC values (keep this small!) */
#define DC_CHECK_TOLERANCE 1

/* Number of bytes to read during a DC check loop */
#define DC_CHECK_BUFFER_LEN (1<<18)

#define FREQ_OFFSET_PPM (0)

/* do not change */
#define REAL_CENTER_FREQ_TX (uint64_t)(CENTER_FREQ_TX * (1.0 + FREQ_OFFSET_PPM * 1e-6))
#define REAL_CENTER_FREQ_RX (uint64_t)(CENTER_FREQ_RX * (1.0 + FREQ_OFFSET_PPM * 1e-6))

#endif // CONFIG_H
