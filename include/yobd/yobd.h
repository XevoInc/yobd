/**
 * @file      yobd.h
 * @brief     yobd public header.
 * @author    Martin Kelly <mkelly@xevo.com>
 * @copyright Copyright (C) 2017 Xevo Inc. All Rights Reserved.
 */

#ifndef YOBD_H_
#define YOBD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/** Forward declaration for opaque pointer. */
struct yobd_ctx;

typedef enum {
    YOBD_OK = 0,
    YOBD_OOM = -1,
    YOBD_PID_DOES_NOT_EXIST = -2,
    YOBD_INVALID_PARAMETER = -3,
    YOBD_CANNOT_OPEN_FILE = -4
} yobd_err;

/** OBD II mode. */
typedef uint_fast8_t yobd_mode;

/** OBD II PID. */
typedef uint_fast16_t yobd_pid;

/** An ID for a unit, used to lookup a corresponding string. */
typedef uint_fast8_t yobd_unit;

/** A memory allocation function, with the same prototype as malloc. */
typedef void *(yobd_alloc)(size_t bytes);

typedef enum {
    YOBD_PID_DATA_TYPE_UINT8 = 0,
    YOBD_PID_DATA_TYPE_FLOAT = 1,
    YOBD_PID_DATA_TYPE_MAX
} yobd_pid_data_type;

/**
 * Parses a schema, returning a context.
 *
 * @param filepath the path to a schema file
 * @param alloc a memory allocator function
 *
 * @return a yobd context
 */
yobd_err yobd_get_ctx(
    const char *filepath,
    yobd_alloc *alloc,
    struct yobd_ctx **ctx);

/**
 * Frees a yobd context.
 *
 * @param ctx a yobd context
 */
void yobd_free_ctx(struct yobd_ctx *ctx);

/**
 * Translates a unit to a unit string.
 *
 * @param ctx a yobd context
 * @param id a unit
 *
 * @return a unit string, with memory owned by yobd
 */
const char *yobd_get_unit_str(const struct yobd_ctx *ctx, yobd_unit unit);

#ifdef __cplusplus
}
#endif

#endif /* YOBD_H_ */
