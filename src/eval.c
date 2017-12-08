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

#define OBD_II_QUERY_ADDRESS (0x7df)
#define OBD_II_RESPONSE_ADDRESS (0x7e8)

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
    /*
     * TODO: Ideally we would assert here that everything is in range. We can do
     * this once ranges are added to the schema.
     */ \
    return (out_type) val; \
}

DEFINE_EVAL_FUNC(int32_t, EXPR_INT32, uint8_t, 0, UINT8_MAX)
DEFINE_EVAL_FUNC(int32_t, EXPR_INT32, uint16_t, 0, UINT16_MAX)
DEFINE_EVAL_FUNC(int32_t, EXPR_INT32, int8_t, 0, INT8_MAX)
DEFINE_EVAL_FUNC(int32_t, EXPR_INT32, int16_t, 0, INT16_MAX)
DEFINE_EVAL_FUNC(float, EXPR_FLOAT, float, FLT_MIN, FLT_MAX)

static inline
bool mode_is_sae_standard(yobd_mode mode)
{
    return mode <= 0x0a;
}

static inline
size_t mode_data_offset(yobd_mode mode)
{
    if (mode_is_sae_standard(mode)) {
        return 2;
    }
    else {
        return 3;
    }
}

PUBLIC_API
yobd_err yobd_make_can_query_noctx(
    bool big_endian,
    yobd_mode mode,
    yobd_pid pid,
    struct can_frame *frame)
{
    const uint8_t *data_start;

    if (frame == NULL) {
        return YOBD_INVALID_PARAMETER;
    }

    /* These are standard for all OBD II. */
    frame->can_id = OBD_II_QUERY_ADDRESS;
    frame->can_dlc = 8;

    /* These vary per query. */
    if (mode_is_sae_standard(mode)) {
        frame->data[0] = 2;
        /* Standard-mode PIDs must use only one byte. */
        if (pid > 0xff) {
            return YOBD_INVALID_PID;
        }
        frame->data[2] = pid;
        data_start = &frame->data[3];
    }
    else {
        frame->data[0] = 3;
        if (big_endian) {
            frame->data[2] = pid & 0xff00;
            frame->data[3] = pid & 0x00ff;
        }
        else {
            frame->data[2] = pid & 0x00ff;
            frame->data[3] = pid & 0xff00;
        }
        data_start = &frame->data[4];
    }
    frame->data[1] = mode;

    /* Pad the rest of the message. */
    memset(
        (void *) data_start,
        OBD_II_PAD_VALUE,
        sizeof(frame->data) - (data_start - frame->data));

    return YOBD_OK;
}

PUBLIC_API
yobd_err yobd_make_can_query(
    struct yobd_ctx *ctx,
    yobd_mode mode,
    yobd_pid pid,
    struct can_frame *frame)
{
    if (ctx == NULL) {
        return YOBD_INVALID_PARAMETER;
    }

    return yobd_make_can_query_noctx(ctx->big_endian, mode, pid, frame);
}

PUBLIC_API
yobd_err yobd_make_can_response_noctx(
    bool big_endian,
    yobd_mode mode,
    yobd_pid pid,
    const uint8_t *data,
    uint8_t data_size,
    struct can_frame *frame)
{
    const uint8_t *data_start;

    if (data == NULL || frame == NULL) {
        return YOBD_INVALID_PARAMETER;
    }

    if (data_size < 1 || data_size > 5) {
        return YOBD_INVALID_PARAMETER;
    }

    /* These are standard for all OBD II. */
    frame->can_id = OBD_II_RESPONSE_ADDRESS;
    frame->can_dlc = 8;

    /* These vary per query. */
    if (mode_is_sae_standard(mode)) {
        if (pid > 0xff) {
            /* Standard-mode PIDs must use only one byte. */
            return YOBD_INVALID_PID;
        }
        frame->data[2] = pid;
        data_start = &frame->data[3];
    }
    else {
        if (big_endian) {
            frame->data[2] = pid & 0xff00;
            frame->data[3] = pid & 0x00ff;
        }
        else {
            frame->data[2] = pid & 0x00ff;
            frame->data[3] = pid & 0xff00;
        }
        data_start = &frame->data[4];
    }
    frame->data[0] = mode_data_offset(mode) + data_size;
    frame->data[1] = 0x40 + mode;

    memcpy((void *) data_start, data, data_size);

    /* Pad the rest of the message. */
    memset(
        (void *) (data_start + data_size),
        OBD_II_PAD_VALUE,
        sizeof(frame->data) - ((data_start - frame->data) + data_size));

    return YOBD_OK;
}

PUBLIC_API
yobd_err yobd_make_can_response(
    struct yobd_ctx *ctx,
    yobd_mode mode,
    yobd_pid pid,
    const uint8_t *data,
    uint8_t data_size,
    struct can_frame *frame)
{
    if (ctx == NULL) {
        return YOBD_INVALID_PARAMETER;
    }

    return yobd_make_can_response_noctx(
        ctx->big_endian,
        mode,
        pid,
        data,
        data_size,
        frame);
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
        case YOBD_PID_DATA_TYPE_UINT16:
            *buf = eval_expr_uint16_t(expr, data);
            break;
        case YOBD_PID_DATA_TYPE_INT8:
            *buf = eval_expr_int8_t(expr, data);
            break;
        case YOBD_PID_DATA_TYPE_INT16:
            *buf = eval_expr_int16_t(expr, data);
            break;
    }
}

static
yobd_err parse_mode_pid(
    bool big_endian,
    const struct can_frame *frame,
    yobd_mode *mode,
    yobd_pid *pid,
    const uint8_t **data_start)
{
    /*
     * The response mode is query mode + 0x40, so less than 0x41 implies a
     * response mode of less than 1, which is invalid.
     */
    *mode = frame->data[1];
    if (frame->can_id == OBD_II_RESPONSE_ADDRESS) {
        if (*mode < 0x41) {
            return YOBD_INVALID_MODE;
        }
        *mode -= 0x40;
    }

    if (mode_is_sae_standard(*mode)) {
        *pid = frame->data[2];
        *data_start = &frame->data[3];
    }
    else {
        if (big_endian) {
            *pid = (frame->data[2] << 8) | frame->data[3];
        }
        else {
            *pid = (frame->data[3] << 8) | frame->data[2];
        }
        *data_start = &frame->data[4];
    }

    return YOBD_OK;
}

PUBLIC_API
yobd_err yobd_parse_headers_noctx(
    bool big_endian,
    const struct can_frame *frame,
    yobd_mode *mode,
    yobd_pid *pid)
{
    const uint8_t *data_start;
    yobd_err err;

    if (frame == NULL || mode == NULL || pid == NULL) {
        return YOBD_INVALID_PARAMETER;
    }

    if (frame->can_id != OBD_II_QUERY_ADDRESS &&
        frame->can_id != OBD_II_RESPONSE_ADDRESS) {
        return YOBD_UNKNOWN_ID;
    }

    if (frame->can_dlc != 8) {
        return YOBD_INVALID_DLC;
    }

    err = parse_mode_pid(big_endian, frame, mode, pid, &data_start);
    if (err != YOBD_OK) {
        return err;
    }

    return YOBD_OK;
}

PUBLIC_API
yobd_err yobd_parse_headers(
    struct yobd_ctx *ctx,
    const struct can_frame *frame,
    yobd_mode *mode,
    yobd_pid *pid)
{
    if (ctx == NULL) {
        return YOBD_INVALID_PARAMETER;
    }

    return yobd_parse_headers_noctx(ctx->big_endian, frame, mode, pid);
}

PUBLIC_API
yobd_err yobd_parse_can_response(
    struct yobd_ctx *ctx,
    const struct can_frame *frame,
    uint8_t *buf)
{
    const uint8_t *data_start;
    yobd_err err;
    size_t expected_bytes;
    yobd_mode mode;
    yobd_pid pid;
    size_t offset;
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

    err = parse_mode_pid(ctx->big_endian, frame, &mode, &pid, &data_start);
    if (err != YOBD_OK) {
        return err;
    }

    parse_ctx = get_parse_ctx(ctx, mode, pid);
    if (parse_ctx == NULL) {
        /* We don't know about this mode-PID combination! */
        return YOBD_UNKNOWN_MODE_PID;
    }

    if (mode_is_sae_standard(mode)) {
        /* One byte for mode, one byte for PID. */
        offset = 2;
    }
    else {
        /* One byte for mode, two bytes for PID. */
        offset = 3;
    }
    expected_bytes = offset + parse_ctx->can_bytes;

    if (frame->data[0] != expected_bytes) {
        return YOBD_INVALID_DATA_BYTES;
    }

    switch (parse_ctx->type) {
        case EVAL_TYPE_EXPR:
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
