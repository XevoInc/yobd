/**
 * @file      entrypoint.c
 * @brief     yobd library entrypoint.
 * @author    Martin Kelly <mkelly@xevo.com>
 * @copyright Copyright (C) 2017 Xevo Inc. All Rights Reserved.
 */

#include <syslog.h>

__attribute__((constructor))
static void lib_init(void)
{
    /* TODO: Make log levels configurable. */
    setlogmask(LOG_UPTO(LOG_INFO));
    /*
     * Immediately initialize the logger to avoid I/O latency when we first
     * initialize the logger.
     */
    openlog("yobd", LOG_NDELAY|LOG_PERROR|LOG_CONS|LOG_PID, LOG_SYSLOG);
}

__attribute__((destructor))
static void lib_destroy(void)
{
    closelog();
}
