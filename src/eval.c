/**
 * @file      eval.c
 * @brief     yobd code to evaluate CAN frames according to a parsed schema.
 * @author    Martin Kelly <mkelly@xevo.com>
 * @copyright Copyright (C) 2017 Xevo Inc. All Rights Reserved.
 */

#include <float.h>
#include <stdbool.h>
#include <yobd_private/api.h>
#include <yobd_private/expr.h>
#include <yobd_private/parser.h>
#include <yobd/yobd.h>

#define OBD_II_REQUEST_ADDRESS (0x7df)
#define OBD_II_RESPONSE_ADDRESS (OBD_II_REQUEST_ADDRESS + 8)

/* ISO 15765-2:2016 page 43 suggests but does not require 0xcc for padding. */
#define OBD_II_PAD_VALUE (0xcc)

#define DEFINE_EVAL_FUNC(stack_type, enum_type, out_type, min_val, max_val) \
static \
out_type eval_expr_##out_type( \
    struct expr *expr, \
    const uint8_t *data) \
{ \
    size_t i; \
    struct expr_token tok1; \
    struct expr_token tok2; \
    struct expr_token result; \
    stack_type val; \
    \
    for (i = 0; i < expr->size; ++i) { \
        switch (expr->data[i].type) { \
            case EXPR_A: \
                result.type = enum_type; \
                result.as_##stack_type = data[0]; \
                PUSH_STACK(EXPR_STACK, &expr->stack, &result); \
                break; \
            case EXPR_B: \
                result.type = enum_type; \
                result.as_##stack_type = data[1]; \
                PUSH_STACK(EXPR_STACK, &expr->stack, &result); \
                break; \
            case EXPR_C: \
                result.type = enum_type; \
                result.as_##stack_type = data[2]; \
                PUSH_STACK(EXPR_STACK, &expr->stack, &result); \
                break; \
            case EXPR_D: \
                result.type = enum_type; \
                result.as_##stack_type = data[3]; \
                PUSH_STACK(EXPR_STACK, &expr->stack, &result); \
                break; \
            case EXPR_OP: \
                tok1 = POP_STACK(EXPR_STACK, &expr->stack); \
                tok2 = POP_STACK(EXPR_STACK, &expr->stack); \
                XASSERT_EQ(tok1.type, enum_type); \
                XASSERT_EQ(tok2.type, enum_type); \
                result.type = enum_type; \
                switch (expr->data[i].as_op) { \
                    case EXPR_OP_ADD: \
                        result.as_##stack_type = tok2.as_##stack_type + tok1.as_##stack_type; \
                        break; \
                    case EXPR_OP_SUB: \
                        result.as_##stack_type = tok2.as_##stack_type - tok1.as_##stack_type; \
                        break; \
                    case EXPR_OP_MUL: \
                        result.as_##stack_type = tok2.as_##stack_type * tok1.as_##stack_type; \
                        break; \
                    case EXPR_OP_DIV: \
                        result.as_##stack_type = tok2.as_##stack_type / tok1.as_##stack_type; \
                        break; \
                } \
                PUSH_STACK(EXPR_STACK, &expr->stack, &result); \
                break; \
            case enum_type: \
                PUSH_STACK(EXPR_STACK, &expr->stack, &expr->data[i]); \
                break; \
            default: \
                /*
                 * All tokens should be of the same numeric type (float or int),
                 * to avoid issues converting between types.
                 */ \
                XASSERT_ERROR; \
        } \
    } \
    \
    XASSERT_EQ(STACK_SIZE(EXPR_STACK, &expr->stack), 1); \
    result = POP_STACK(EXPR_STACK, &expr->stack); \
    XASSERT_EQ(result.type, enum_type); \
    \
    val = result.as_##stack_type; \
    XASSERT_GTE(val, min_val); \
    XASSERT_LTE(val, max_val); \
    \
    return (out_type) val; \
}

DEFINE_EVAL_FUNC(float, EXPR_FLOAT, float, FLT_MIN, FLT_MAX)
DEFINE_EVAL_FUNC(int32_t, EXPR_INT32, uint8_t, 0, UINT8_MAX)

static inline
bool mode_is_sae_standard(yobd_mode mode)
{
    return mode <= 0x0a;
}

PUBLIC_API
yobd_err yobd_make_can_request(
    struct yobd_ctx *ctx,
    yobd_mode mode,
    yobd_pid pid,
    struct can_frame *frame)
{
    const __u8 *data_end;

    if (ctx == NULL || frame == NULL) {
        return YOBD_INVALID_PARAMETER;
    }

    /* These are standard for all OBD II. */
    frame->can_id = OBD_II_REQUEST_ADDRESS;
    frame->can_dlc = 8;

    /* These vary per request. */
    if (mode_is_sae_standard(mode)) {
        frame->data[0] = 2;
        /*
         * Standard-mode PIDs must use only one byte, which the schema should
         * verify.
         */
        if (pid > 0x0f) {
            return YOBD_INVALID_PID;
        }
        frame->data[2] = pid;
        data_end = &frame->data[3];
    }
    else {
        frame->data[0] = 3;
        if (ctx->big_endian) {
            frame->data[2] = pid & 0xff00;
            frame->data[3] = pid & 0x00ff;
        }
        else {
            frame->data[2] = pid & 0x00ff;
            frame->data[3] = pid & 0xff00;
        }
        data_end = &frame->data[4];
    }
    frame->data[1] = mode;

    /* Pad the rest of the message. */
    memset(
        (void *) data_end,
        OBD_II_PAD_VALUE,
        frame->data + sizeof(frame->data) - data_end);

    return YOBD_OK;
}

static
void eval_expr(
    yobd_pid_data_type pid_type,
    struct expr *expr,
    const uint8_t *data,
    uint8_t *buf)
{
    /* Endianness of floats in buf will be host order. */
    float val;

    switch (pid_type) {
        case YOBD_PID_DATA_TYPE_FLOAT:
            val = eval_expr_float(expr, data);
            memcpy(buf, &val, sizeof(val));
            break;
        case YOBD_PID_DATA_TYPE_UINT8:
            *buf = eval_expr_uint8_t(expr, data);
            break;
    }
}

PUBLIC_API
yobd_err yobd_parse_can_response(
    struct yobd_ctx *ctx,
    const struct can_frame *frame,
    uint8_t *buf)
{
    const uint8_t *data_start;
    yobd_mode mode;
    yobd_pid pid;
    struct parse_pid_ctx *parse_ctx;

    if (ctx == NULL || frame == NULL || buf == NULL) {
        return YOBD_INVALID_PARAMETER;
    }

    if (frame->can_id != OBD_II_RESPONSE_ADDRESS) {
        return YOBD_UNKNOWN_ID;
    }

    if (frame->can_dlc != 8) {
        return YOBD_INVALID_DLC;
    }

    /*
     * The response mode is request mode + 0x40, so less than 0x41 implies a
     * response mode of less than 1, which is invalid.
     */
    mode = frame->data[1];
    if (mode < 0x41) {
        return YOBD_INVALID_MODE;
    }
    mode -= 0x40;

    if (mode_is_sae_standard(mode)) {
        pid = frame->data[2];
        data_start = &frame->data[3];
    }
    else {
        if (ctx->big_endian) {
            pid = (frame->data[2] << 8) | frame->data[3];
        }
        else {
            pid = (frame->data[3] << 8) | frame->data[2];
        }
        data_start = &frame->data[4];
    }

    parse_ctx = get_parse_ctx(ctx, mode, pid);
    if (parse_ctx == NULL) {
        /* We don't know about this mode-PID combination! */
        return YOBD_UNKNOWN_MODE_PID;
    }

    if (frame->data[0] != parse_ctx->can_bytes) {
        return YOBD_INVALID_DATA_BYTES;
    }

    if (parse_ctx->can_bytes > 7) {
        return YOBD_TOO_MANY_DATA_BYTES;
    }

    switch (parse_ctx->type) {
        case EVAL_TYPE_EXPR:
            if (parse_ctx->can_bytes > 4) {
                return YOBD_TOO_MANY_DATA_BYTES;
            }
            eval_expr(
                parse_ctx->pid_desc.type,
                &parse_ctx->expr,
                data_start,
                buf);
            break;
    }

    return YOBD_OK;
}

PUBLIC_API
yobd_err yobd_get_pid_descriptor(
    struct yobd_ctx *ctx,
    yobd_mode mode,
    yobd_pid pid,
    const struct yobd_pid_desc **pid_desc)
{
    const struct parse_pid_ctx *parse_ctx;

    if (ctx == NULL || pid_desc == NULL) {
        return YOBD_INVALID_PARAMETER;
    }

    parse_ctx = get_parse_ctx(ctx, mode, pid);
    if (parse_ctx == NULL) {
        return YOBD_UNKNOWN_MODE_PID;
    }

    *pid_desc = &parse_ctx->pid_desc;

    return YOBD_OK;
}
