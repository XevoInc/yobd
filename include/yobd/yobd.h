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

#include <linux/can.h>
#include <stdint.h>

/** Forward declaration for opaque pointer. */
struct yobd_ctx;

typedef enum {
    YOBD_OK = 0,
    YOBD_OOM = -1,
    YOBD_PID_DOES_NOT_EXIST = -2,
    YOBD_INVALID_PARAMETER = -3,
    YOBD_INVALID_PATH = -4,
    YOBD_CANNOT_OPEN_FILE = -5,
    YOBD_UNKNOWN_ID = -6,
    YOBD_INVALID_DLC = -7,
    YOBD_INVALID_MODE = -8,
    YOBD_INVALID_PID = -9,
    YOBD_UNKNOWN_MODE_PID = -10,
    YOBD_UNKNOWN_UNIT = -11,
    YOBD_INVALID_DATA_BYTES = -12
} yobd_err;

/** OBD II mode. */
typedef uint_fast8_t yobd_mode;

/** OBD II PID. */
typedef uint_fast16_t yobd_pid;

/** An ID for a unit, used to lookup a corresponding string. */
typedef uint_fast8_t yobd_unit;

typedef enum {
    YOBD_PID_DATA_TYPE_UINT8 = 0,
    YOBD_PID_DATA_TYPE_FLOAT = 1
} yobd_pid_data_type;

/** The details about how to interpret a PID from bitpacked yobd output. */
struct yobd_pid_desc {
    const char *name;
    yobd_unit unit;
    uint_fast8_t can_bytes;
    uint_fast8_t interpreted_bytes;
    yobd_pid_data_type type;
};

/**
 * Parses a schema, returning a context.
 *
 * @param file a schema file (either by name, in which case it must be in the
 *             yobd schema directory, or by path, either relative or absolute)
 *
 * @return a yobd context
 */
yobd_err yobd_parse_schema(const char *file, struct yobd_ctx **ctx);

/**
 * Frees a yobd context.
 *
 * @param ctx a yobd context
 */
void yobd_free_ctx(struct yobd_ctx *ctx);

/**
 * Gets the descriptor corresponding to the given mode and PID, containing
 * information about how to interpret the bitpacked data for this PID.
 *
 * Note that this call is relatively cheap (it amounts to a hash table lookup),
 * so it is OK for callers to call it for every incoming CAN frame. In other
 * words, there is no need to separately cache PID descriptors, and caller
 * should just look them up on-demand.
 *
 * @param ctx a yobd context
 * @param mode an OBD II mode
 * @param pid an OBD II PID
 * @param desc filled in with a pointer to a PID descriptor describing the
 *             bitpacked data, with memory owned by yobd
 *
 * @return an error code
 */
yobd_err yobd_get_pid_descriptor(
    struct yobd_ctx *ctx,
    yobd_mode mode,
    yobd_pid pid,
    const struct yobd_pid_desc **desc);

/**
 * Translates a unit to a unit string.
 *
 * @param ctx a yobd context
 * @param id a unit
 * @param unit_str filled in with a unit string, with memory owned by yobd
 *
 * @return an error code
 * found
 */
yobd_err yobd_get_unit_str(
    const struct yobd_ctx *ctx,
    yobd_unit unit,
    const char **unit_str);

/**
 * Creates a CAN frame representing a given OBD II query.
 *
 * @param ctx a yobd context
 * @param mode an OBD II mode
 * @param pid an OBD II PID
 * @param frame the CAN frame to be filled in
 *
 * @return an error code
 */
yobd_err yobd_make_can_query(
    struct yobd_ctx *ctx,
    yobd_mode mode,
    yobd_pid pid,
    struct can_frame *frame);

/**
 * Creates a CAN frame representing a given OBD II query without requiring a
 * yobd context.
 *
 * @param big_endian whether or not the CAN bus is big-endian
 * @param mode an OBD II mode
 * @param pid an OBD II PID
 * @param frame the CAN frame to be filled in
 *
 * @return an error code
 */
yobd_err yobd_make_can_query_noctx(
    bool big_endian,
    yobd_mode mode,
    yobd_pid pid,
    struct can_frame *frame);

/**
 * Creates a CAN frame representing a given OBD II response.
 *
 * @param ctx a yobd context
 * @param mode an OBD II mode
 * @param pid an OBD II PID
 * @param data the data payload for the CAN frame
 * @param data_size the size of the data payload (must be between 1 and 5 by the
 *                  OBD II spec). This is specified so that yobd does not have
 *                  to know about the mode-pid in its schema or incur the
 *                  overhead of a schema lookup.
 * @param frame the CAN frame to be filled in
 *
 * @return an error code
 */
yobd_err yobd_make_can_response(
    struct yobd_ctx *ctx,
    yobd_mode mode,
    yobd_pid pid,
    const uint8_t *data,
    uint8_t data_size,
    struct can_frame *frame);

/**
 * Creates a CAN frame representing a given OBD II response, without requiring a
 * yobd context.
 *
 * @param big_endian whether or not the CAN bus is big-endian
 * @param mode an OBD II mode
 * @param pid an OBD II PID
 * @param data the data payload for the CAN frame
 * @param data_size the size of the data payload (must be between 1 and 5 by the
 *                  OBD II spec). This is specified so that yobd does not have
 *                  to know about the mode-pid in its schema or incur the
 *                  overhead of a schema lookup.
 * @param frame the CAN frame to be filled in
 *
 * @return an error code
 */
yobd_err yobd_make_can_response_noctx(
    bool big_endian,
    yobd_mode mode,
    yobd_pid pid,
    const uint8_t *data,
    uint8_t data_size,
    struct can_frame *frame);

/**
 * Parses a given CAN frame, returning basic header information about the frame.
 *
 * @param ctx a yobd context
 * @param frame a CAN frame to be parsed
 * @param mode filled in with the mode of the given CAN frame
 * @param pid filled in with the pid of the given CAN frame
 *
 * @return an error code
 */
yobd_err yobd_parse_headers(
    struct yobd_ctx *ctx,
    const struct can_frame *frame,
    yobd_mode *mode,
    yobd_pid *pid);

/**
 * Parses a given CAN frame, returning basic header information about the
 * frame, without requiring a yobd context.
 *
 * @param ctx a yobd context
 * @param frame a CAN frame to be parsed
 * @param mode filled in with the mode of the given CAN frame
 * @param pid filled in with the pid of the given CAN frame
 *
 * @return an error code
 */
yobd_err yobd_parse_headers_noctx(
    bool big_endian,
    const struct can_frame *frame,
    yobd_mode *mode,
    yobd_pid *pid);

/**
 * Interprets a CAN frame, yielding an output buffer containing resource IDs.
 *
 * @param ctx a yobd context
 * @param frame a CAN frame to be interpreted
 * @param buf a buffer allocated to the caller, which will be filled in with
 *            bitpacked OBD II data. The buffer must be large enough to hold the
 *            data, according to the size returned via yobd_get_pid_descriptor.
 *            The buffer being too small will result in "undefined behavior" :).
 *
 * @return an error code
 */
yobd_err yobd_parse_can_response(
    struct yobd_ctx *ctx,
    const struct can_frame *frame,
    uint8_t *buf);

#ifdef __cplusplus
}
#endif

#endif /* YOBD_H_ */
