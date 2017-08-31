/**
 * @file      parser.c
 * @brief     Unit test for the schema parser.
 * @author    Martin Kelly <mkelly@xevo.com>
 * @copyright Copyright (C) 2017 Xevo Inc. All Rights Reserved.
 */

#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <yobd/yobd.h>

int main(int argc, const char **argv) {
    struct yobd_ctx *ctx;
    yobd_err err;
    const char *schema_file;

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
    err = yobd_get_ctx(schema_file, malloc, &ctx);
    assert(err == YOBD_OK);
    assert(ctx != NULL);

    yobd_free_ctx(ctx);

    return 0;
}
