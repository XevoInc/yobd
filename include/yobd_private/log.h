/**
 * @file      log.h
 * @brief     Private logging header.
 * @author    Martin Kelly <mkelly@xevo.com>
 * @copyright Copyright (C) 2018 Xevo Inc. All Rights Reserved.
 *
 */

#ifndef YOBD_PRIVATE_LOG_H_
#define YOBD_PRIVATE_LOG_H_

#include <stdarg.h>
#include <syslog.h>

void log_init(void);
void log_destroy(void);

void log_msg(int priority, const char *fmt, ...);
void log_vmsg(int priority, const char *fmt, va_list args);
void log_assert_msg(const char *msg);

/*
 * Sadly, we have two versions of log_err because having empty __VA_ARGS__ in a
 * vararg macro is banned by C99.
 */
#define yobd_log_err(err, fmt, ...) \
    do { \
        log_msg(LOG_ERR, fmt ", err: %d (%s)'", __VA_ARGS__, err, yobd_strerror(err)); \
    } while (0);

#define yobd_log_err_nofmt(err, msg) \
    do { \
        log_msg(LOG_ERR, msg ", err: %d (%s)", err, yobd_strerror(err)); \
    } while (0);

#endif /* YOBD_PRIVATE_LOG_H_ */
