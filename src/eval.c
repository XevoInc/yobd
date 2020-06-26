/**
 * @file      eval.c
 * @brief     yobd code to evaluate CAN frames according to a parsed schema.
 * @author    Martin Kelly <mkelly@xevo.com>
 * @copyright Copyright (C) 2017 Xevo Inc. All Rights Reserved.
 */

#define _DEFAULT_SOURCE
#include <endian.h>
#include <float.h>
#include <stdbool.h>
#include <yobd-private/api.h>
#include <yobd-private/expr.h>
#include <yobd-private/parser.h>
#include <yobd/yobd.h>

#include <stdio.h>

/**
 * The value to use to pad OBD II messages. ISO 15765-2:2016 page 43 suggests
 * but does not require 0xcc for padding.
 */
#define OBD_II_PAD_VALUE (0xcc)

/**
 * The CAN data length code that should be used for OBD-II frames.
 */
#define OBD_II_DLC 8

/**
 * Macro to define stack evaluation functions.
 *
 * @param stack_type the type of values the stack can handle
 * @param enum_type the enum type corresponding to the given stack_type
 */
#define DEFINE_EVAL_FUNC(stack_type, enum_type) \
static \
float eval_expr_##stack_type( \
    const struct expr *expr, \
    struct EXPR_STACK *stack, \
    const unsigned char *data) \
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
                PUSH_STACK(EXPR_STACK, stack, &result); \
                break; \
            case EXPR_B: \
                result.type = enum_type; \
                result.as_##stack_type = data[1]; \
                PUSH_STACK(EXPR_STACK, stack, &result); \
                break; \
            case EXPR_C: \
                result.type = enum_type; \
                result.as_##stack_type = data[2]; \
                PUSH_STACK(EXPR_STACK, stack, &result); \
                break; \
            case EXPR_D: \
                result.type = enum_type; \
                result.as_##stack_type = data[3]; \
                PUSH_STACK(EXPR_STACK, stack, &result); \
                break; \
            case EXPR_OP: \
                tok1 = POP_STACK(EXPR_STACK, stack); \
                tok2 = POP_STACK(EXPR_STACK, stack); \
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
                PUSH_STACK(EXPR_STACK, stack, &result); \
                break; \
            case enum_type: \
                PUSH_STACK(EXPR_STACK, stack, &expr->data[i]); \
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
    XASSERT_EQ(STACK_SIZE(EXPR_STACK, stack), 1); \
    result = POP_STACK(EXPR_STACK, stack); \
    XASSERT_EQ(result.type, enum_type); \
    \
    val = result.as_##stack_type; \
    return (float) val; \
}

DEFINE_EVAL_FUNC(int32_t, EXPR_INT32)
DEFINE_EVAL_FUNC(float, EXPR_FLOAT)

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
    const unsigned char *data_start;

    if (frame == NULL) {
        return YOBD_INVALID_PARAMETER;
    }

    /* These are standard for all OBD II. */
    frame->can_id = YOBD_OBD_II_QUERY_ADDRESS;
    frame->can_dlc = OBD_II_DLC;

    /* These vary per query. */
    if (mode_is_sae_standard(mode)) {
        frame->data[0] = 2;
        /* Standard-mode PIDs must use only one byte. */
        if (pid > 0xff) {
            return YOBD_INVALID_PID;
        }
        frame->data[1] = mode;
        frame->data[2] = pid;
        data_start = &frame->data[3];
    }
    else {
        frame->data[0] = 3;
        if (big_endian) {
            frame->data[1] = mode;
            frame->data[2] = pid & 0xff00;
            frame->data[3] = pid & 0x00ff;
        }
        else {
            frame->data[1] = mode;
            frame->data[2] = pid & 0x00ff;
            frame->data[3] = pid & 0xff00;
        }
        data_start = &frame->data[4];
    }

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
    const unsigned char *data,
    uint8_t data_size,
    struct can_frame *frame)
{
    const unsigned char *data_start;

    if (data == NULL || frame == NULL) {
        return YOBD_INVALID_PARAMETER;
    }

    if (data_size < 1 || data_size > 5) {
        return YOBD_INVALID_PARAMETER;
    }

    /* These are standard for all OBD II. */
    frame->can_id = YOBD_OBD_II_RESPONSE_BASE;
    frame->can_dlc = OBD_II_DLC;

    /* These vary per query. */
    frame->data[0] = mode_data_offset(mode) + data_size;
    frame->data[1] = 0x40 + mode;
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
    const unsigned char *data,
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
bool is_query(const struct can_frame *frame)
{
    return frame->can_id == YOBD_OBD_II_QUERY_ADDRESS;
}

static
bool is_response(const struct can_frame *frame)
{
    return (frame->can_id >= YOBD_OBD_II_RESPONSE_BASE &&
            frame->can_id <= YOBD_OBD_II_RESPONSE_END);
}

float nop_eval(
    bool big_endian,
    uint_fast8_t can_bytes,
    pid_data_type pid_type,
    const unsigned char *data)
{
    union {
        float float_val;
        uint32_t uint_val;
    } nop;
    uint32_t num;
    float val;

    switch (can_bytes) {
        case 1:
            val = (float) *data;
            break;
        case 2:
            if (big_endian) {
                val = (float) be16toh(*((uint16_t *) data));
            }
            else {
                val = (float) le16toh(*((uint16_t *) data));
            }
            break;
        case 3:
            if (big_endian) {
                num = (data[0] << 16) | (data[1] << 8) | data[2];
            }
            else {
                num = (data[2] << 16) | (data[0] << 8) | data[0];
            }
            val = (float) num;
            break;
        case 4:
            if (pid_type != PID_DATA_TYPE_FLOAT) {
                if (big_endian) {
                    val = (float) be32toh(*((uint32_t *) data));
                }
                else {
                    val = (float) le32toh(*((uint32_t *) data));
                }
            }
            else {
                /* Reinterpret the bits as an IEEE 754 float. */
                if (big_endian) {
                    nop.uint_val = be32toh(*((uint32_t *) data));
                }
                else {
                    /* Little endian. */
                    nop.uint_val = le32toh(*((uint32_t *) data));
                }
                val = nop.float_val;
            }
            break;
        default:
            /* We should not have passed the parsing step. */
            XASSERT_ERROR;
    }

    return val;
}

static
float stack_eval(pid_data_type pid_type, const struct expr *expr, const unsigned char *data)
{
    struct EXPR_STACK eval_stack;
    struct expr_token stack_data[expr->size * sizeof(*expr->data)];
    float val;

    /* Put the data for the evaluation stack on the stack. Haha. */
    INIT_STACK(EXPR_STACK, &eval_stack, stack_data, expr->size);

    switch (pid_type) {
        case PID_DATA_TYPE_FLOAT:
            val = eval_expr_float(expr, &eval_stack, data);
            break;
        case PID_DATA_TYPE_INT8:
        case PID_DATA_TYPE_UINT8:
        case PID_DATA_TYPE_UINT16:
        case PID_DATA_TYPE_INT16:
        case PID_DATA_TYPE_UINT32:
        case PID_DATA_TYPE_INT32:
            val = eval_expr_int32_t(expr, &eval_stack, data);
            break;
    }

    return val;
}

static
float eval_expr(
    bool big_endian,
    uint_fast8_t can_bytes,
    pid_data_type pid_type,
    const struct expr *expr,
    const unsigned char *data,
    convert_func convert)
{
    float val;

    switch (expr->type) {
        case EXPR_NOP:
            val = nop_eval(big_endian, can_bytes, pid_type, data);
            break;
        case EXPR_STACK:
            val = stack_eval(pid_type, expr, data);
            break;
    }

    return convert(val);
}

static
yobd_err parse_mode_pid(
    bool big_endian,
    const struct can_frame *frame,
    yobd_mode *mode,
    yobd_pid *pid,
    const unsigned char **data_start)
{
    /*
     * The response mode is query mode + 0x40, so less than 0x41 implies a
     * response mode of less than 1, which is invalid.
     */
    *mode = frame->data[1];
    if (is_response(frame)) {
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
yobd_err yobd_parse_can_headers_noctx(
    bool big_endian,
    const struct can_frame *frame,
    yobd_mode *mode,
    yobd_pid *pid)
{
    const unsigned char *data_start;
    yobd_err err;

    if (frame == NULL || mode == NULL || pid == NULL) {
        return YOBD_INVALID_PARAMETER;
    }

    if (!is_query(frame) && !is_response(frame)) {
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
yobd_err yobd_parse_can_headers(
    struct yobd_ctx *ctx,
    const struct can_frame *frame,
    yobd_mode *mode,
    yobd_pid *pid)
{
    if (ctx == NULL) {
        return YOBD_INVALID_PARAMETER;
    }

    return yobd_parse_can_headers_noctx(ctx->big_endian, frame, mode, pid);
}

PUBLIC_API
yobd_err yobd_parse_can_response(
    struct yobd_ctx *ctx,
    const struct can_frame *frame,
    float *val)
{
    const unsigned char *data_start;
    yobd_err err;
    size_t expected_bytes;
    yobd_mode mode;
    yobd_pid pid;
    size_t offset;
    const struct parse_pid_ctx *pid_ctx;

    if (ctx == NULL || frame == NULL || val == NULL) {
        return YOBD_INVALID_PARAMETER;
    }

    if (!is_response(frame)) {
        return YOBD_UNKNOWN_ID;
    }

    if (frame->can_dlc != 8) {
        return YOBD_INVALID_DLC;
    }

    err = parse_mode_pid(ctx->big_endian, frame, &mode, &pid, &data_start);
    if (err != YOBD_OK) {
        return err;
    }

    pid_ctx = get_pid_ctx(ctx, mode, pid);
    if (pid_ctx == NULL) {
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
    expected_bytes = offset + pid_ctx->desc.can_bytes;

    if (frame->data[0] != expected_bytes) {
        return YOBD_INVALID_DATA_BYTES;
    }

    *val = eval_expr(
        ctx->big_endian,
        pid_ctx->desc.can_bytes,
        pid_ctx->pid_type,
        &pid_ctx->expr,
        data_start,
        pid_ctx->convert_func);

    return YOBD_OK;
}

PUBLIC_API
yobd_err yobd_get_pid_descriptor(
    struct yobd_ctx *ctx,
    yobd_mode mode,
    yobd_pid pid,
    const struct yobd_pid_desc **pid_desc)
{
    const struct parse_pid_ctx *pid_ctx;

    if (ctx == NULL || pid_desc == NULL) {
        return YOBD_INVALID_PARAMETER;
    }

    pid_ctx = get_pid_ctx(ctx, mode, pid);
    if (pid_ctx == NULL) {
        return YOBD_UNKNOWN_MODE_PID;
    }

    *pid_desc = &pid_ctx->desc;

    return YOBD_OK;
}
