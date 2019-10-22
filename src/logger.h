/*
 * vim: sw=2 ts=2 expandtab
 *
 * Copyright (c) 2015-2019 Thomas Kolb
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
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
