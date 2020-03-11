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
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** The CAN address to use to query a vehicle. */
#define YOBD_OBD_II_QUERY_ADDRESS (0x7df)

/*
 * OBD-II responses will be on addresses between
 * OBD_II_RESPONSE_BASE and OBD_II_RESPONSE_END, inclusive.
 */

/** The first OBD-II response address. */
#define YOBD_OBD_II_RESPONSE_BASE (0x7e8)

/** The last OBD-II response address. */
#define YOBD_OBD_II_RESPONSE_END (0x7ef)

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
    YOBD_INVALID_DATA_BYTES = -12,
    YOBD_PARSE_FAIL = -13
} yobd_err;

/**
 * Units for PID descriptors. These are SI units as much as possible. Time is an
 * exception, as specifying everything in seconds will result in floating point
 * issues.
 */
typedef enum {
    YOBD_UNIT_DEGREE = 0,
    YOBD_UNIT_KELVIN,
    YOBD_UNIT_KG_PER_S,
    YOBD_UNIT_METER,
    YOBD_UNIT_METERS_PER_S,
    YOBD_UNIT_METERS_PER_S_SQUARED,
    YOBD_UNIT_NONE,
    YOBD_UNIT_PASCAL,
    YOBD_UNIT_PERCENT,
    YOBD_UNIT_RAD,
    YOBD_UNIT_RAD_PER_S,
    YOBD_UNIT_NANOSECOND
} yobd_unit;

/**
 * Returns a human-readable error string. The string must not be modified or
 * freed, but it may be modified by subsequent calls to yobd_strerror or the
 * libc strerror class of functions.
 */
const char *yobd_strerror(yobd_err err);

/** OBD II mode. */
typedef uint_fast8_t yobd_mode;

/** OBD II PID. */
typedef uint_fast16_t yobd_pid;

/** The details about how to interpret a PID from bitpacked yobd output. */
struct yobd_pid_desc {
    const char *name;
    uint_fast8_t can_bytes;
    yobd_unit unit;
};

/**
 * Parses a schema, returning a context.
 *
 * @param file a schema file (either by name, in which case it must be in the
 *             yobd schema directory, or by path, either relative or absolute)
 * @param ctx a yobd context, to be filled in
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
 * Gets the number of PIDs known to this context.
 *
 * @param ctx a yobd context
 * @param count to be filled in with the PID count
 *
 * @return an error code
 */
yobd_err yobd_get_pid_count(struct yobd_ctx *ctx, size_t *count);

/**
 * Gets the descriptor corresponding to the given mode and PID, containing
 * information about how to interpret the bitpacked data for this PID.
 *
 * Note that this call is relatively cheap (it amounts to a hash table lookup),
 * so it is OK for callers to call it for every incoming CAN frame. In other
 * words, there is no need to separately cache PID descriptors, and the caller
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
 * PID processing function, used in yobd_pid_foreach.
 *
 * @param desc a PID descriptor provided by the iteration function
 * @param data user-specific data context passed into each function call
 * @param mode the mode of the passed-in PID
 * @param pid the PID being processed
 *
 * @return true boolean indicating "done status". if done processing
 *              descriptors, false otherwise. If this function returns true,
 *              iteration will terminate.
 */
typedef bool (*pid_process_func)(
    const struct yobd_pid_desc *desc,
    yobd_mode mode,
    yobd_pid pid, void *data);

/**
 * Iterates through the PID descriptors in the given yobd context.
 *
 * @param ctx a yobd context
 * @param func a function called once per PID descriptor
 * @param data user-specific data context passed into each function call
 * descriptors left
 *
 * @return an error code
 */
yobd_err yobd_pid_foreach(
    struct yobd_ctx *ctx,
    pid_process_func func,
    void *data);

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
yobd_err yobd_parse_can_headers(
    struct yobd_ctx *ctx,
    const struct can_frame *frame,
    yobd_mode *mode,
    yobd_pid *pid);

/**
 * Parses a given CAN frame, returning basic header information about the
 * frame, without requiring a yobd context.
 *
 * @param big_endian true if parsing big endian CAN, false otherwise
 * @param frame a CAN frame to be parsed
 * @param mode filled in with the mode of the given CAN frame
 * @param pid filled in with the pid of the given CAN frame
 *
 * @return an error code
 */
yobd_err yobd_parse_can_headers_noctx(
    bool big_endian,
    const struct can_frame *frame,
    yobd_mode *mode,
    yobd_pid *pid);

/**
 * Interprets a CAN frame, yielding an output buffer containing resource IDs.
 *
 * @param ctx a yobd context
 * @param frame a CAN frame to be interpreted
 * @param val a float value in SI units
 * @return an error code
 */
yobd_err yobd_parse_can_response(
    struct yobd_ctx *ctx,
    const struct can_frame *frame,
    float *val);

#ifdef __cplusplus
}
#endif

#endif /* YOBD_H_ */
