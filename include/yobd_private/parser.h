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
#include <yobd_private/unit.h>

typedef enum {
    EVAL_TYPE_EXPR
} eval_type;

struct parse_pid_ctx {
    /* The raw OBD II unit, not yet converted to SI. */
    yobd_unit raw_unit;
    pid_data_type pid_type;
    /* The byte count of the CAN response, not of the OBD II response. */
    eval_type eval_type;
    union {
        struct expr expr;
    };
    /* Public PID descriptor. */
    struct yobd_pid_desc desc;
};

struct parse_pid_ctx *get_parse_ctx(
    const struct yobd_ctx *ctx,
    yobd_mode mode,
    yobd_pid pid);

struct unit_tuple {
    yobd_unit raw_unit;
    yobd_unit si_unit;
};

to_si get_convert_func(const struct yobd_ctx *ctx, yobd_unit unit);

XHASH_MAP_INIT_INT8(UNIT_MAP, struct unit_desc)
XHASH_MAP_INIT_INT(MODEPID_MAP, struct parse_pid_ctx)

XHASH_MAP_INIT_STR(UNIT_NAME_MAP, struct unit_tuple)

struct yobd_ctx {
    yobd_unit next_unit_id;
    bool big_endian;
    xhash_t(UNIT_MAP) *unit_map;
    xhash_t(MODEPID_MAP) *modepid_map;
};

#endif /* YOBD_PRIVATE_PARSER_H_ */
