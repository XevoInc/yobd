/**
 * @file      assert.c
 * @brief     yobd assertion code.
 * @author    Martin Kelly <mkelly@xevo.com>
 * @copyright Copyright (C) 2017 Xevo Inc. All Rights Reserved.
 */

#include <yobd_private/assert.h>
#include <syslog.h>

/**
 * Log the given message to syslog.
 *
 * @param msg a message to log
 */
void log_syslog(const char *msg)
{
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wformat-security"
    syslog(LOG_CRIT, msg);
    #pragma GCC diagnostic pop
}
