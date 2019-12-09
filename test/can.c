/**
 * @file      can.c
 * @brief     Unit test for the CAN schema parser and evaluator.
 * @author    Martin Kelly <mkelly@xevo.com>
 * @copyright Copyright (C) 2017 Xevo Inc. All Rights Reserved.
 */

#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <linux/limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yobd/yobd.h>
#include <yobd-test/assert.h>

#define FLOAT_THRESH 0.001

bool process_pids(const struct yobd_pid_desc *desc, void *data)
{
    size_t *pid_count;

    XASSERT_NOT_NULL(desc);

    pid_count = data;
    ++(*pid_count);

    return false;
}

int main(int argc, const char **argv)
{
    size_t api_pid_count;
    struct yobd_ctx *ctx;
    union {
        uint16_t as_uint16_t;
        uint8_t as_uint8_t;
    } maf_rate;
    yobd_err err;
    struct can_frame frame;
    struct can_frame frame2;
    size_t iter_pid_count;
    yobd_mode mode;
    yobd_mode mode2;
    yobd_pid pid;
    yobd_pid pid2;
    const struct yobd_pid_desc *pid_desc;
    const char *schema_file;
    const char *str;
    float val;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s SCHEMA-FILE\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    if (strnlen(argv[1], PATH_MAX) == PATH_MAX) {
        fprintf(stderr, "File argument is longer than PATH_MAX\n");
        exit(EXIT_FAILURE);
    }
    schema_file = argv[1];

    XASSERT_STREQ(yobd_strerror(YOBD_OOM), "out of memory!");
    XASSERT_STREQ(yobd_strerror(EIO), strerror(EIO));
    XASSERT_NULL(yobd_strerror(INT_MIN));

    ctx = NULL;
    err = yobd_parse_schema(schema_file, &ctx);
    XASSERT_OK(err);
    XASSERT_NOT_NULL(ctx);

    api_pid_count = 0;
    err = yobd_get_pid_count(ctx, &api_pid_count);
    XASSERT_OK(err);
    XASSERT_GT(api_pid_count, 0);

    iter_pid_count = 0;
    err = yobd_pid_foreach(ctx, process_pids, &iter_pid_count);
    XASSERT_EQ(iter_pid_count, api_pid_count);

    err = yobd_get_pid_descriptor(ctx, 0x1, 0x0f, &pid_desc);
    XASSERT_OK(err);
    XASSERT_NOT_NULL(pid_desc)
    XASSERT_STREQ(pid_desc->name, "Intake air temperature");
    XASSERT_EQ(pid_desc->can_bytes, 1);

    err = yobd_get_unit_str(ctx, pid_desc->unit, &str);
    XASSERT_OK(err);
    XASSERT_STREQ(str, "K");

    memset(&frame, 0, sizeof(frame));
    memset(&frame2, 0, sizeof(frame2));
    err = yobd_make_can_query(ctx, 0x1, 0x10, &frame);
    XASSERT_OK(err);
    err = yobd_make_can_query_noctx(true, 0x1, 0x10, &frame2);
    XASSERT_OK(err);
    XASSERT_EQ(memcmp(&frame, &frame2, sizeof(frame)), 0);

    XASSERT_EQ(frame.can_id, 0x7df);
    XASSERT_EQ(frame.can_dlc, 8);
    XASSERT_EQ(frame.data[0], 2);
    XASSERT_EQ(frame.data[1], 0x1);
    XASSERT_EQ(frame.data[2], 0x10);
    XASSERT_EQ(frame.data[3], 0xcc);
    XASSERT_EQ(frame.data[4], 0xcc);
    XASSERT_EQ(frame.data[5], 0xcc);
    XASSERT_EQ(frame.data[6], 0xcc);
    XASSERT_EQ(frame.data[7], 0xcc);

    memset(&frame, 0, sizeof(frame));
    memset(&frame2, 0, sizeof(frame2));
    maf_rate.as_uint16_t = 0xabcd;
    err = yobd_make_can_response(
        ctx,
        0x1,
        0x10,
        &maf_rate.as_uint8_t,
        sizeof(maf_rate),
        &frame);
    XASSERT_OK(err);
    err = yobd_make_can_response_noctx(
        true,
        0x1,
        0x10,
        &maf_rate.as_uint8_t,
        sizeof(maf_rate),
        &frame2);
    XASSERT_OK(err);
    XASSERT_EQ(memcmp(&frame, &frame2, sizeof(frame)), 0);

    XASSERT_EQ(frame.can_id, 0x7e8);
    XASSERT_EQ(frame.can_dlc, 8);
    XASSERT_EQ(frame.data[0], 4);
    XASSERT_EQ(frame.data[1], 0x1 + 0x40);
    XASSERT_EQ(frame.data[2], 0x10);
    XASSERT_EQ(frame.data[3], 0xcd);
    XASSERT_EQ(frame.data[4], 0xab);
    XASSERT_EQ(frame.data[5], 0xcc);
    XASSERT_EQ(frame.data[6], 0xcc);
    XASSERT_EQ(frame.data[7], 0xcc);

    /* Make sure we can parse the response we made. */
    err = yobd_parse_can_headers(ctx, &frame, &mode, &pid);
    XASSERT_OK(err);
    XASSERT_EQ(mode, 0x1);
    XASSERT_EQ(pid, 0x10);
    mode2 = pid2 = 0;
    err = yobd_parse_can_headers_noctx(ctx, &frame, &mode2, &pid2);
    XASSERT_OK(err);
    XASSERT_EQ(mode, mode2);
    XASSERT_EQ(pid, pid2);

    err = yobd_parse_can_response(ctx, &frame, &val);
    XASSERT_OK(err);
    /*
     * 0xab == 171
     * 0xcd == 205
     * (256*205 + 171) / 100 == 526.51
     * converting from g/s to kg/s, 526.51 --> 0.52651
     * Note that we flip byte order from 0xabcd as this is big-endian.
     */
    XASSERT_FLTEQ_THRESH(val, 0.526500000f, FLOAT_THRESH);

    memset(&frame, 0, sizeof(frame));
    frame.can_id = 0x7df;
    frame.can_dlc = 8;
    frame.data[0] = 1;
    frame.data[1] = 0x1;
    frame.data[2] = 0x0d;
    frame.data[3] = 60;
    err = yobd_parse_can_headers(ctx, &frame, &mode, &pid);
    XASSERT_OK(err);
    XASSERT_EQ(mode, 0x1);
    XASSERT_EQ(pid, 0x0d);

    memset(&frame, 0, sizeof(frame));
    frame.can_id = 0x7e8;
    frame.can_dlc = 8;
    /*
     * (256*77 + 130) / 4 == 4960.50 RPM
     * 4960.50 RPM --> 519.462345 rad/s
     */
    frame.data[0] = 4;
    frame.data[1] = 0x1 + 0x40;
    frame.data[2] = 0x0c;
    frame.data[3] = 77;
    frame.data[4] = 130;
    err = yobd_parse_can_response(ctx, &frame, &val);
    XASSERT_OK(err);
    XASSERT_FLTEQ_THRESH(val, 519.462345f, FLOAT_THRESH);

    memset(&frame, 0, sizeof(frame));
    frame.can_id = 0x7e8;
    frame.can_dlc = 8;
    frame.data[0] = 3;
    frame.data[1] = 0x1 + 0x40;
    frame.data[2] = 0x0d;
    frame.data[3] = 60;
    err = yobd_parse_can_response(ctx, &frame, &val);
    XASSERT_OK(err);
    XASSERT_FLTEQ(val, 16.666666f);

    yobd_free_ctx(ctx);

    return EXIT_SUCCESS;
}
