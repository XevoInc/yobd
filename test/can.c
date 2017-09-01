/**
 * @file      can.c
 * @brief     Unit test for the CAN schema parser and evaluator.
 * @author    Martin Kelly <mkelly@xevo.com>
 * @copyright Copyright (C) 2017 Xevo Inc. All Rights Reserved.
 */

#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <float.h>
#include <linux/limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yobd/yobd.h>

bool float_eq(float a, float b)
{
    return fabs(a - b) < DBL_EPSILON;
}

int main(int argc, const char **argv) {
    struct yobd_ctx *ctx;
    yobd_err err;
    struct can_frame frame;
    const struct yobd_pid_desc *pid_desc;
    const char *schema_file;
    const char *str;
    union {
        float as_float;
        uint8_t as_uint8_t;
        uint8_t as_bytes[sizeof(float)];
    } u;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s SCHEMA-FILE\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    if (strnlen(argv[1], PATH_MAX) == PATH_MAX) {
        fprintf(stderr, "File argument is longer than PATH_MAX\n");
        exit(EXIT_FAILURE);
    }
    schema_file = argv[1];

    ctx = NULL;
    err = yobd_parse_schema(schema_file, &ctx);
    assert(err == YOBD_OK);
    assert(ctx != NULL);

    err = yobd_get_pid_descriptor(ctx, 0x1, 0x0c, &pid_desc);
    assert(err == YOBD_OK);
    assert(pid_desc != NULL);
    assert(strcmp(pid_desc->name, "Engine RPM") == 0);
    assert(pid_desc->bytes == sizeof(float));
    assert(pid_desc->type == YOBD_PID_DATA_TYPE_FLOAT);

    err = yobd_get_unit_str(ctx, pid_desc->unit, &str);
    assert(err == YOBD_OK);
    assert(strcmp(str, "rpm") == 0);

    err = yobd_make_can_request(ctx, 0x1, 0x0c, &frame);
    assert(err == YOBD_OK);
    assert(frame.can_id == 0x7df);
    assert(frame.can_dlc == 8);
    assert(frame.data[0] == 2);
    assert(frame.data[1] == 0x1);
    assert(frame.data[2] == 0x0c);
    assert(frame.data[3] == 0xcc);
    assert(frame.data[4] == 0xcc);
    assert(frame.data[5] == 0xcc);
    assert(frame.data[6] == 0xcc);
    assert(frame.data[7] == 0xcc);

    frame.can_id = 0x7df + 8;
    frame.can_dlc = 8;
    /* (256*77 + 130) / 4 == 4960.50 RPM */
    frame.data[0] = 2;
    frame.data[1] = 0x1 + 0x40;
    frame.data[2] = 0x0c;
    frame.data[3] = 77;
    frame.data[4] = 130;
    err = yobd_parse_can_response(ctx, &frame, u.as_bytes);
    assert(err == YOBD_OK);
    assert(float_eq(u.as_float, 4960.500));

    frame.can_id = 0x7df + 8;
    frame.can_dlc = 8;
    frame.data[0] = 1;
    frame.data[1] = 0x1 + 0x40;
    frame.data[2] = 0x0d;
    frame.data[3] = 60;
    err = yobd_parse_can_response(ctx, &frame, u.as_bytes);
    assert(err == YOBD_OK);
    assert(u.as_uint8_t == 60);

    yobd_free_ctx(ctx);

    return 0;
}
