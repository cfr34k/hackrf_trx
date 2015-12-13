/*
 * vim: sw=2 ts=2 expandtab
 *
 * "THE PIZZA-WARE LICENSE" (derived from "THE BEER-WARE LICENCE"):
 * Thomas Kolb <cfr34k@tkolb.de> wrote this file. As long as you retain this
 * notice you can do whatever you want with this stuff. If we meet some day,
 * and you think this stuff is worth it, you can buy me a pizza in return.
 * - Thomas Kolb
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <semaphore.h>
#include <stdarg.h>

static const int LVL_FATAL =   0; /*!< Fatal message level */
static const int LVL_ERR   =   5; /*!< Error message level */
static const int LVL_WARN  =  50; /*!< Warning message level */
static const int LVL_INFO  = 100; /*!< Information message level */
static const int LVL_DEBUG = 200; /*!< Debug message level */
static const int LVL_DUMP  = 500; /*!< Dump message level */

extern sem_t logger_semaphore;
extern int logger_verbosity;
extern int logger_use_colors;

void logger_init(void);
void logger_shutdown(void);
void logger_enable_colors(int enable);
void logger_set_verbosity(int verbosity);

void logger_log(int level, const char *format, ...);

#define LOG(level, ...) logger_log(level, __VA_ARGS__)

#endif // LOGGER_H
