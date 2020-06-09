/**
 * @file      parser.h
 * @brief     yobd parser header.
 * @author    Martin Kelly <mkelly@xevo.com>
 * @copyright Copyright (C) 2017 Xevo Inc. All Rights Reserved.
 */

#ifndef YOBD_PRIVATE_PARSER_H_
#define YOBD_PRIVATE_PARSER_H_

#include <stdint.h>
#include <xlib/xhash.h>
#include <yobd/yobd.h>
#include <yobd-private/unit.h>

struct parse_pid_ctx {
    convert_func convert_func;
    pid_data_type pid_type;
    struct expr expr;
    /* Public PID descriptor. */
    struct yobd_pid_desc desc;
};

struct parse_pid_ctx *get_pid_ctx(
    const struct yobd_ctx *ctx,
    yobd_mode mode,
    yobd_pid pid);

XHASH_MAP_INIT_INT(MODEPID_MAP, struct parse_pid_ctx)

struct yobd_ctx {
    bool big_endian;
    xhash_t(MODEPID_MAP) *modepid_map;
};

#endif /* YOBD_PRIVATE_PARSER_H_ */
