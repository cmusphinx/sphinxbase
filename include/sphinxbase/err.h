/* -*- c-basic-offset: 4; indent-tabs-mode: nil -*- */
/* ====================================================================
 * Copyright (c) 1999-2004 Carnegie Mellon University.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * This work was supported in part by funding from the Defense Advanced
 * Research Projects Agency and the National Science Foundation of the
 * United States of America, and the CMU Sphinx Speech Consortium.
 *
 * THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 *
 */

#ifndef _LIBUTIL_ERR_H_
#define _LIBUTIL_ERR_H_

#include <stdarg.h>
#include <stdio.h>
#ifndef _WIN32_WCE
#include <errno.h>
#endif

/* Win32/WinCE DLL gunk */
#include <sphinxbase/sphinxbase_export.h>

#ifdef __ANDROID__
#include <android/log.h>
#endif

/**
 * @file err.h
 * @brief Implementation of logging routines.
 *
 * Logging, warning, debug and error message output funtionality is provided in this file.
 * Sphinxbase defines several level of logging messages - INFO, WARNING, ERROR, FATAL. By
 * default output goes to standard error output.
 *
 * Logging is implemented through macros. They take same arguments as printf: format string and
 * values. By default source file name and source line are prepended to the message. Log output
 * could be redirected to any file using err_set_logfp() and err_set_logfile() functions. To disable
 * logging in your application, call err_set_logfp(NULL).
 *
 * It's possible to log multiline info messages, to do that you need to start message with
 * E_INFO and output other lines with E_INFOCONT.
 */

#ifdef __cplusplus
extern "C" {
#endif
#if 0
/* Fool Emacs. */
}
#endif

#define FILELINE  __FILE__ , __LINE__

/**
 * Exit with non-zero status after error message
 */
#define E_FATAL(...)                               \
    do {                                           \
        err_msg(ERR_FATAL, FILELINE, __VA_ARGS__); \
        exit(EXIT_FAILURE);                        \
    } while (0)

/**
 * Print error text; Call perror(""); exit(errno);
 */
#define E_FATAL_SYSTEM(fmt, ...)                                           \
    do {                                                                   \
        int local_errno = errno;                                           \
        char *err_fmt = (char *) malloc(strlen("%s: ") + strlen(fmt) + 1); \
        strcat(strcpy(err_fmt, "%s: "), fmt);                              \
        err_msg(ERR_FATAL, FILELINE, err_fmt,                              \
                strerror(local_errno), ##__VA_ARGS__);                     \
        free(err_fmt);                                                     \
        exit(local_errno);                                                 \
    } while (0)

/**
 * Print error text; Call perror("");
 */
#define E_ERROR_SYSTEM(fmt, ...)                                           \
    do {                                                                   \
        int local_errno = errno;                                           \
        char *err_fmt = (char *) malloc(strlen("%s: ") + strlen(fmt) + 1); \
        strcat(strcpy(err_fmt, "%s: "), fmt);                              \
        err_msg(ERR_FATAL, FILELINE, err_fmt,                              \
                strerror(local_errno), ##__VA_ARGS__);                     \
    } while (0)

/**
 * Print error message to error log
 */
#define E_ERROR(...)     err_msg(ERR_ERROR, FILELINE, __VA_ARGS__)

/**
 * Print warning message to error log
 */
#define E_WARN(...)      err_msg(ERR_WARN, FILELINE, __VA_ARGS__)

/**
 * Print logging information to standard error stream
 */
#define E_INFO(...)      err_msg(ERR_INFO, FILELINE, __VA_ARGS__)

/**
 * Print logging information without header, to standard error stream
 */
#define E_INFOCONT(...)  err_msg(ERR_INFO, NULL, 0, __VA_ARGS__)

/**
 * Print logging information without filename.
 */
#define E_INFO_NOFN      E_INFOCONT

/**
 * Print debugging information to standard error stream.
 *
 * This will only print a message if:
 *  1. Debugging is enabled at compile time
 *  2. The debug level is greater than or equal to \a level
 *
 * Note that for portability reasons the format and arguments must be
 * enclosed in an extra set of parentheses.
 */
#ifdef SPHINX_DEBUG
#define E_DEBUG(level, ...) \
        if (err_get_debug_level() >= level) \
            err_msg(ERR_DEBUG, FILELINE, __VA_ARGS__)
#define E_DEBUGCONT(level, ...) \
        if (err_get_debug_level() >= level) \
            err_msg(ERR_DEBUG, NULL, 0, __VA_ARGS__)
#else
#define E_DEBUG(level,x)
#define E_DEBUGCONT(level,x)
#endif


typedef enum err_e {
#ifdef __ANDROID__
    ERR_DEBUG = ANDROID_LOG_DEBUG,
    ERR_INFO = ANDROID_LOG_INFO,
    ERR_WARN = ANDROID_LOG_WARN,
    ERR_ERROR = ANDROID_LOG_ERROR,
    ERR_FATAL = ANDROID_LOG_ERROR
#else
    ERR_DEBUG,
    ERR_INFO,
    ERR_WARN,
    ERR_ERROR,
    ERR_FATAL,
    ERR_MAX
#endif
} err_lvl_t;

typedef void (*err_cb_f)(err_lvl_t, const char *, ...);

/**
 * Sets function to output error messages
 */
SPHINXBASE_EXPORT void
err_set_callback(err_cb_f callback);

SPHINXBASE_EXPORT void
err_msg(err_lvl_t lvl, const char *path, long ln, const char *fmt, ...);

#if   defined __ANDROID__
SPHINXBASE_EXPORT void
err_logcat_cb(err_lvl_t level, const char *fmt, ...);
#elif defined _WIN32_WCE
SPHINXBASE_EXPORT void
err_wince_cb(err_lvl_t level, const char *fmt, ...);
#else
SPHINXBASE_EXPORT void
err_logfp_cb(err_lvl_t level, const char *fmt, ...);
#endif

SPHINXBASE_EXPORT FILE *
err_set_logfile(const char *path);

SPHINXBASE_EXPORT FILE *
err_set_logfp(FILE *stream);

SPHINXBASE_EXPORT FILE *
err_get_logfp(void);

/**
 * Set debugging verbosity level.
 *
 * Note that debugging messages are only enabled when compiled with -DDEBUG.
 *
 * @param level Verbosity level to set, or 0 to disable debug messages.
 */
SPHINXBASE_EXPORT
int err_set_debug_level(int level);

/**
 * Get debugging verbosity level.
 *
 * Note that debugging messages are only enabled when compiled with -DDEBUG.
 */
SPHINXBASE_EXPORT
int err_get_debug_level(void);

#ifdef __cplusplus
}
#endif

#endif /* !_ERR_H */

/* vim: set ts=4 sw=4: */
