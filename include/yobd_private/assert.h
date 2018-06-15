/**
 * @file      assert.h
 * @brief     yobd assertion header, wrapping xassert.
 * @author    Martin Kelly <mkelly@xevo.com>
 * @copyright Copyright (C) 2017 Xevo Inc. All Rights Reserved.
 */

#ifndef YOBD_PRIVATE_ASSERT_H_
#define YOBD_PRIVATE_ASSERT_H_

#include <xlib/xassert.h>

void log_syslog(const char *msg);

XASSERT_DEFINE_ASSERTS(log_syslog)

#define XASSERT_ERRCODE(x, y) _XASSERT_ERRCODE(x, y, yobd_strerror)
#define XASSERT_OK(err) XASSERT_ERRCODE(err, YOBD_OK)

#endif /* YOBD_PRIVATE_ASSERT_H_ */
