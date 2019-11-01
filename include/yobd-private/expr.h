/**
 * @file      expr.h
 * @brief     yobd expression parser header.
 * @author    Martin Kelly <mkelly@xevo.com>
 * @copyright Copyright (C) 2017 Xevo Inc. All Rights Reserved.
 */

#ifndef YOBD_PRIVATE_EXPR_H_
#define YOBD_PRIVATE_EXPR_H_

#include <stddef.h>
#include <stdint.h>
#include <yobd/yobd.h>
#include <yobd-private/stack.h>
#include <yobd-private/types.h>

struct expr_token {
    enum {
        EXPR_A,
        EXPR_B,
        EXPR_C,
        EXPR_D,
        EXPR_OP,
        EXPR_FLOAT,
        EXPR_INT32
    } type;
    union {
        enum expr_op_token {
            EXPR_OP_ADD,
            EXPR_OP_SUB,
            EXPR_OP_MUL,
            EXPR_OP_DIV
        } as_op;
        float as_float;
        int_fast32_t as_int32_t;
    };
};

DEFINE_STACK(EXPR_STACK, struct expr_token)

struct expr {
    size_t size;
    struct expr_token *data;
    struct EXPR_STACK stack;
};

yobd_err parse_expr(
    const char *str,
    struct expr *expr,
    pid_data_type type);

void destroy_expr(struct expr *expr);

#endif /* YOBD_PRIVATE_EXPR_H_ */
