/**
 * @file      error.c
 * @brief     yobd error handling functions.
 * @author    Martin Kelly <mkelly@xevo.com>
 * @copyright Copyright (C) 2017 Xevo Inc. All Rights Reserved.
 */

#include <yobd/yobd.h>
#include <yobd-private/api.h>
#include <yobd-private/error.h>
#include <yobd-private/log.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>

PUBLIC_API
const char *yobd_strerror(yobd_err err)
{
    /* Not a yobd error code. */
    if (err > 0) {
        return strerror(err);
    }

    switch (err) {
        case YOBD_OK:
            return "OK";
        case YOBD_OOM:
            return "out of memory!";
        case YOBD_PID_DOES_NOT_EXIST:
            return "specified PID does not exist";
        case YOBD_INVALID_PARAMETER:
            return "invalid parameter";
        case YOBD_INVALID_PATH:
            return "invalid path";
        case YOBD_CANNOT_OPEN_FILE:
            return "cannot open file";
        case YOBD_UNKNOWN_ID:
            return "unknown CAN ID";
        case YOBD_INVALID_DLC:
            return "invalid CAN DLC code";
        case YOBD_INVALID_MODE:
            return "invald mode";
        case YOBD_INVALID_PID:
            return "invalid PID";
        case YOBD_UNKNOWN_MODE_PID:
            return "unknown mode/PID combination specified";
        case YOBD_UNKNOWN_UNIT:
            return "unknown unit specified";
        case YOBD_INVALID_DATA_BYTES:
            return "bytes specified is different than expected";
        case YOBD_PARSE_FAIL:
            return "failed to parse YOBD schema";
    }

    /*
     * The user passed in something that wasn't a valid error code. We could
     * could make this the default case, but then the compiler would not be able
     * to check that we covered all valid error cases.
     */
    return NULL;
}
