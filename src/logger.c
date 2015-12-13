/*
 * vim: sw=2 ts=2 expandtab
 *
 * "THE PIZZA-WARE LICENSE" (derived from "THE BEER-WARE LICENCE"):
 * Thomas Kolb <cfr34k@tkolb.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a pizza in return.
 * - Thomas Kolb
 */

#include <stdio.h>
#include <malloc.h>

#include <time.h>
#include <sys/time.h>

#include "logger.h"

// define constants
const char *LOGGER_STR_FATAL = "FATAL";
const char *LOGGER_STR_ERR   = "ERROR";
const char *LOGGER_STR_WARN  = "WARN ";
const char *LOGGER_STR_INFO  = "INFO ";
const char *LOGGER_STR_DEBUG = "DEBUG";
const char *LOGGER_STR_DUMP  = "DUMP ";

const char *LOGGER_COLOR_FATAL = "\033[1;31m";
const char *LOGGER_COLOR_ERR   = "\033[1;31m";
const char *LOGGER_COLOR_WARN  = "\033[1;33m";
const char *LOGGER_COLOR_INFO  = "\033[1;32m";
const char *LOGGER_COLOR_DEBUG = "\033[1m";
const char *LOGGER_COLOR_DUMP  = "\033[1;30m";
const char *LOGGER_COLOR_NONE  = "\033[0m";

// global variables
sem_t logger_semaphore;
int logger_verbosity;
int logger_use_colors;

void logger_init(void) {
  // Initialize the semaphore
  sem_init(&logger_semaphore, 0, 1);

  logger_verbosity = 2147483647;
  logger_use_colors = 1;
}

void logger_shutdown(void) {
  sem_destroy(&logger_semaphore);
}

void logger_enable_colors(int enable) {
	logger_use_colors = enable;
}

void logger_set_verbosity(int verbosity) {
	logger_verbosity = verbosity;
}

void logger_debug_message(const char *prefix, const char *fmt, va_list ap) {
	/* Guess we need no more than 100 bytes. */
	int n, size = 100;
	char *p, *np;
	va_list internal_ap;

	if ((p = (char*)malloc(size)) == NULL) {
		fprintf(stderr, "[%s] FATAL: Cannot allocate string buffer while processing arguments.\n", LOGGER_STR_ERR);
		return;
	}

	while (1) {
		/* Try to print in the allocated space. */
		va_copy(internal_ap, ap);
		n = vsnprintf(p, size, fmt, internal_ap);
		va_end(internal_ap);

		/* If that worked, return the string. */
		if (n > -1 && n < size)
			break;

		/* Else try again with more space. */
		if (n > -1)    /* glibc 2.1 */
			size = n+1; /* precisely what is needed */
		else           /* glibc 2.0 */
			size *= 2;  /* twice the old size */

		if ((np = (char*)realloc (p, size)) == NULL) {
			free(p);
			fprintf(stderr, "[%s] FATAL: Cannot reallocate string buffer while processing arguments.\n", LOGGER_STR_ERR);
			return;
		} else {
			p = np;
		}
	}

  sem_wait(&logger_semaphore);
	fprintf(stderr, "%s %s\n", prefix, p);
  sem_post(&logger_semaphore);

	free(p);
}

void logger_log(int level, const char *format, ...) {
	va_list argptr;

  char timebuf[32];
  char timebuf2[32];

  char prefixbuf[64];
  const char *prefixcolor = "", *prefixtext = "";

	if(level > logger_verbosity)
		return;

  struct timeval tv;
  gettimeofday(&tv, NULL);
  strftime(timebuf, 32, "%Y-%M-%d %H:%M:%S.%%03d", localtime(&(tv.tv_sec)));
  snprintf(timebuf2, 32, timebuf, tv.tv_usec/1000);

	if(level >= LVL_DUMP) {
		if(logger_use_colors)
			prefixcolor = LOGGER_COLOR_DUMP;

		prefixtext = LOGGER_STR_DUMP;
	} else if(level >= LVL_DEBUG) {
		if(logger_use_colors)
			prefixcolor = LOGGER_COLOR_DEBUG;

		prefixtext = LOGGER_STR_DEBUG;
	} else if(level >= LVL_INFO) {
		if(logger_use_colors)
			prefixcolor = LOGGER_COLOR_INFO;

		prefixtext = LOGGER_STR_INFO;
	} else if(level >= LVL_WARN) {
		if(logger_use_colors)
			prefixcolor = LOGGER_COLOR_WARN;

		prefixtext = LOGGER_STR_WARN;
	} else if(level >= LVL_ERR) {
		if(logger_use_colors)
			prefixcolor = LOGGER_COLOR_ERR;

		prefixtext = LOGGER_STR_ERR;
	} else {
		if(logger_use_colors)
			prefixcolor = LOGGER_COLOR_FATAL;

		prefixtext = LOGGER_STR_FATAL;
	}

	if(logger_use_colors) {
    sprintf(prefixbuf, "%s [%s%s%s]", timebuf2, prefixcolor, prefixtext, LOGGER_COLOR_NONE);
  } else {
    sprintf(prefixbuf, "%s [%s]", timebuf2, prefixtext);
  }

	va_start(argptr, format);
	logger_debug_message(prefixbuf, format, argptr);
	va_end(argptr);
}
