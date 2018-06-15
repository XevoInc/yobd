/**
 * @file      assert.h
 * @brief     Test assert code.
 * @author    Martin Kelly <mkelly@xevo.com>
 * @copyright Copyright (C) 2018 Xevo Inc. All Rights Reserved.
 */

#ifndef YOBD_TEST_ASSERT_H_
#define YOBD_TEST_ASSERT_H_

#include <yobd/yobd.h>
#include <stdio.h>
#include <xlib/xassert.h>

static inline
void print_assert_msg(const char *msg)
{
    fputs(msg, stderr);
}

XASSERT_DEFINE_ASSERTS(print_assert_msg)

#define XASSERT_ERRCODE(x, y) _XASSERT_ERRCODE(x, y, yobd_strerror)
#define XASSERT_OK(err) XASSERT_ERRCODE(err, YOBD_OK)

#endif /* YOBD_TEST_ASSERT_H_ */
