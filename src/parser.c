/**
 * @file      parser.c
 * @brief     yobd PID schema parser implementation.
 * @author    Martin Kelly <mkelly@xevo.com>
 * @copyright Copyright (C) 2017 Xevo Inc. All Rights Reserved.
 */

#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <yaml.h>
#include <yobd-private/api.h>
#include <yobd-private/assert.h>
#include <yobd-private/expr.h>
#include <yobd-private/parser.h>
#include <yobd-private/unit.h>

#include "config.h"

#define ARRAYLEN(a) (sizeof(a) / sizeof(a[0]))

typedef xhiter_t yobd_iter;

static uint32_t get_modepid(yobd_mode mode, yobd_pid pid)
{
    return (mode << 16) | pid;
}

static yobd_mode get_mode(uint32_t modepid)
{
    return modepid >> 16;
}

static yobd_pid get_pid(uint32_t modepid)
{
    return modepid & 0xff;
}

struct parse_pid_ctx *get_pid_ctx(
    const struct yobd_ctx *ctx,
    yobd_mode mode,
    yobd_pid pid)
{
    xhiter_t iter;

    iter = xh_get(MODEPID_MAP, ctx->modepid_map, get_modepid(mode, pid));
    if (iter == xh_end(ctx->modepid_map)) {
        return NULL;
    }

    return &xh_val(ctx->modepid_map, iter);
}

PUBLIC_API
yobd_err yobd_get_pid_count(struct yobd_ctx *ctx, size_t *count)
{
    if (ctx == NULL || count == NULL) {
        return YOBD_INVALID_PARAMETER;
    }

    *count = xh_size(ctx->modepid_map);

    return YOBD_OK;
}

PUBLIC_API
yobd_err yobd_pid_foreach(
    struct yobd_ctx *ctx,
    pid_process_func func,
    void *data)
{
    const struct yobd_pid_desc *desc;
    bool done;
    xhiter_t iter;
    yobd_mode mode;
    uint32_t modepid;
    yobd_pid pid;

    if (ctx == NULL) {
        return YOBD_INVALID_PARAMETER;
    }

    /* Find the first valid iter entry. */
    xh_iter(ctx->modepid_map, iter,
        modepid = xh_key(ctx->modepid_map, iter);
        mode = get_mode(modepid);
        pid = get_pid(modepid);
        desc = &xh_val(ctx->modepid_map, iter).desc;
        done = func(desc, mode, pid, data);
        if (done) {
            break;
        }
    );

    return YOBD_OK;
}

static
struct parse_pid_ctx *put_mode_pid(
    struct yobd_ctx *ctx,
    yobd_mode mode,
    yobd_pid pid)
{
    xhiter_t iter;
    int ret;

    iter = xh_put(MODEPID_MAP, ctx->modepid_map, (mode << 16) | pid, &ret);
    if (ret == -1) {
        return NULL;
    }

    return &xh_val(ctx->modepid_map, iter);
}

PUBLIC_API
void yobd_free_ctx(struct yobd_ctx *ctx)
{
    xhiter_t iter;
    struct parse_pid_ctx *pid_ctx;

    if (ctx == NULL) {
        return;
    }

    XASSERT_NOT_NULL(ctx->modepid_map);

    xh_iter(ctx->modepid_map, iter,
        pid_ctx = &xh_val(ctx->modepid_map, iter);

        free((char *) pid_ctx->desc.name);
        if (pid_ctx->expr.type == EXPR_STACK) {
            destroy_expr(&pid_ctx->expr);
        }
    );
    xh_destroy(MODEPID_MAP, ctx->modepid_map);

    free(ctx);
}

static
pid_data_type find_type(const char *str)
{
    size_t i;

    /* Make sure this stays in sync with the pid_data_type enum! */
    static const char *types[] = {
        [PID_DATA_TYPE_UINT8] = "uint8",
        [PID_DATA_TYPE_INT8] = "int8",
        [PID_DATA_TYPE_UINT16] = "uint16",
        [PID_DATA_TYPE_INT16] = "int16",
        [PID_DATA_TYPE_UINT32] = "uint32",
        [PID_DATA_TYPE_INT32] = "int32",
        [PID_DATA_TYPE_FLOAT] = "float"
    };

    for (i = 0; i < ARRAYLEN(types); ++i) {
        if (strcmp(types[i], str) == 0) {
            return i;
        }
    }

    XASSERT_ERROR;
}

static
yobd_unit find_unit(const char *val)
{
    if (strcmp(val, "degree") == 0) {
        return YOBD_UNIT_DEGREE;
    }
    else if (strcmp(val, "K") == 0) {
        return YOBD_UNIT_KELVIN;
    }
    else if (strcmp(val, "kg/s") == 0) {
        return YOBD_UNIT_KG_PER_S;
    }
    else if (strcmp(val, "lat") == 0) {
        return YOBD_UNIT_LATITUDE;
    }
    else if (strcmp(val, "lng") == 0) {
        return YOBD_UNIT_LONGITUDE;
    }
    else if (strcmp(val, "m") == 0) {
        return YOBD_UNIT_METER;
    }
    else if (strcmp(val, "m/s") == 0) {
        return YOBD_UNIT_METERS_PER_S;
    }
    else if (strcmp(val, "m/s^2") == 0) {
        return YOBD_UNIT_METERS_PER_S_2;
    }
    else if (strcmp(val, "Pa") == 0) {
        return YOBD_UNIT_PASCAL;
    }
    else if (strcmp(val, "percent") == 0) {
        return YOBD_UNIT_PERCENT;
    }
    else if (strcmp(val, "rad") == 0) {
        return YOBD_UNIT_RAD;
    }
    else if (strcmp(val, "rad/s") == 0) {
        return YOBD_UNIT_RAD_PER_S;
    }
    else if (strcmp(val, "ns") == 0) {
        return YOBD_UNIT_NANOSECOND;
    }
    else {
        /*
         * An unknown type was encountered. Either the schema validator failed,
         * or we need to add a new enum to yobd_unit and to the cases list here.
         */
        XASSERT_ERROR;
    }
}

static
bool parse_endianness(const char *val)
{
    if (strcmp(val, "big") == 0) {
        return true;
    }
    else {
        XASSERT_EQ(strcmp(val, "little"), 0);
        return false;
    }
}

static
yobd_err parse_expr(
    yaml_node_t *node,
    yaml_document_t *doc,
    pid_data_type *pid_type,
    struct expr *expr)
{
    const char *expr_str;
    yaml_node_t *key;
    const char *key_str;
    yaml_node_pair_t *pair;
    const char *pid_type_str;
    yaml_node_t *val;
    const char *val_str;

    /*
     * In order to parse the expression, we need the PID type first. So we first
     * scan through to find both strings, then we parse them in the correct
     * order.
     */
    XASSERT_EQ(node->type, YAML_MAPPING_NODE);
    expr_str = NULL;
    pid_type_str = NULL;
    for (pair = node->data.mapping.pairs.start;
         pair < node->data.mapping.pairs.top;
         ++pair) {
        key = yaml_document_get_node(doc, pair->key);
        XASSERT_NOT_NULL(key);
        XASSERT_EQ(key->type, YAML_SCALAR_NODE);
        key_str = (const char *) key->data.scalar.value;

        val = yaml_document_get_node(doc, pair->value);
        XASSERT_NOT_NULL(val);
        XASSERT_EQ(val->type, YAML_SCALAR_NODE);
        val_str = (const char *) val->data.scalar.value;

        if (strcmp(key_str, "type") == 0) {
            pid_type_str = val_str;
        }
        else if (strcmp(key_str, "val") == 0) {
            expr_str = val_str;
        }
        else {
        /* Unknown node. */
            XASSERT_ERROR;
        }
    }
    XASSERT_NOT_NULL(expr_str);
    XASSERT_NOT_NULL(pid_type_str);

    *pid_type = find_type(pid_type_str);
    return parse_expr_val(expr_str, expr, *pid_type);
}

static
yobd_err parse_desc(
    yaml_node_t *node,
    yaml_document_t *doc,
    struct parse_pid_ctx *pid_ctx)
{
    yaml_node_t *key;
    const char *key_str;
    yaml_node_pair_t *pair;
    yaml_node_t *val;
    const char *val_str;

    XASSERT_EQ(node->type, YAML_MAPPING_NODE);
    for (pair = node->data.mapping.pairs.start;
         pair < node->data.mapping.pairs.top;
         ++pair) {
        key = yaml_document_get_node(doc, pair->key);
        XASSERT_NOT_NULL(key);
        XASSERT_EQ(key->type, YAML_SCALAR_NODE);
        key_str = (const char *) key->data.scalar.value;

        val = yaml_document_get_node(doc, pair->value);
        XASSERT_NOT_NULL(val);

        if (strcmp(key_str, "name") == 0) {
            XASSERT_EQ(val->type, YAML_SCALAR_NODE);
            val_str = (const char *) val->data.scalar.value;
            pid_ctx->desc.name = strdup(val_str);
            if (pid_ctx->desc.name == NULL) {
                return YOBD_OOM;
            }
        }
        else if (strcmp(key_str, "bytes") == 0) {
            XASSERT_EQ(val->type, YAML_SCALAR_NODE);
            val_str = (const char *) val->data.scalar.value;
            errno = 0;
            pid_ctx->desc.can_bytes = strtol(val_str, NULL, 0);
            XASSERT_EQ(errno, 0);
        }
        else if (strcmp(key_str, "raw-unit") == 0) {
            XASSERT_EQ(val->type, YAML_SCALAR_NODE);
            val_str = (const char *) val->data.scalar.value;
            pid_ctx->convert_func = find_convert_func(val_str);
        }
        else if (strcmp(key_str, "si-unit") == 0) {
            XASSERT_EQ(val->type, YAML_SCALAR_NODE);
            val_str = (const char *) val->data.scalar.value;
            pid_ctx->desc.unit = find_unit(val_str);
        }
        else if (strcmp(key_str, "expr") == 0) {
            parse_expr(val, doc, &pid_ctx->pid_type, &pid_ctx->expr);
        }
    }

    switch (pid_ctx->pid_type) {
        case PID_DATA_TYPE_FLOAT:
            /* Passthrough floats must use 4 bytes, as we interpret them as a
             * raw IEEE 754 value, which requires 4 bytes to work with. Note
             * that this assertion should never fire, as the schema parser
             * should catch this condition.
             */
            if (pid_ctx->expr.type == EXPR_NOP) {
                XASSERT_EQ(pid_ctx->desc.can_bytes, 4);
            }
            break;
        case PID_DATA_TYPE_INT8:
        case PID_DATA_TYPE_UINT8:
            XASSERT_EQ(pid_ctx->desc.can_bytes, 1);
            break;
        case PID_DATA_TYPE_UINT16:
        case PID_DATA_TYPE_INT16:
            /*
             * Allow only widening casts (1 byte being output as 2 bytes),
             * and not narrowing, since that would lose information.
             */
            XASSERT_GTE(pid_ctx->desc.can_bytes, 1);
            XASSERT_LTE(pid_ctx->desc.can_bytes, 2);
            break;
        case PID_DATA_TYPE_UINT32:
        case PID_DATA_TYPE_INT32:
            /* See above; allow only widening casts. */
            XASSERT_GTE(pid_ctx->desc.can_bytes, 1);
            XASSERT_LTE(pid_ctx->desc.can_bytes, 4);
            break;
    }

    return YOBD_OK;
}

static
yobd_err parse_mode(
    yaml_node_t *node,
    yaml_document_t *doc,
    yobd_mode mode,
    struct yobd_ctx *ctx)
{
    yobd_err err;
    yaml_node_t *key;
    const char *key_str;
    yaml_node_pair_t *pair;
    yobd_pid pid;
    struct parse_pid_ctx *pid_ctx;
    yaml_node_t *val;

    XASSERT_EQ(node->type, YAML_MAPPING_NODE);
    for (pair = node->data.mapping.pairs.start;
         pair < node->data.mapping.pairs.top;
         ++pair) {
        key = yaml_document_get_node(doc, pair->key);
        XASSERT_NOT_NULL(key);
        XASSERT_EQ(key->type, YAML_SCALAR_NODE);
        key_str = (const char *) key->data.scalar.value;

        errno = 0;
        pid = strtol(key_str, NULL, 0);
        XASSERT_OK(errno);

        pid_ctx = put_mode_pid(ctx, mode, pid);
        if (pid_ctx == NULL) {
            return YOBD_OOM;
        }

        val = yaml_document_get_node(doc, pair->value);
        XASSERT_NOT_NULL(val);

        err = parse_desc(val, doc, pid_ctx);
        if (err != YOBD_OK) {
            /* NULL this out so it's safe to free at the top level. */
            pid_ctx->desc.name = NULL;
            return err;
        }
    }

    return YOBD_OK;
}

static
yobd_err parse_modepid(
    yaml_node_t *node,
    yaml_document_t *doc,
    struct yobd_ctx *ctx)
{
    yobd_err err;
    yaml_node_t *key;
    const char *key_str;
    yobd_mode mode;
    yaml_node_pair_t *pair;
    yaml_node_t *val;

    XASSERT_EQ(node->type, YAML_MAPPING_NODE);
    for (pair = node->data.mapping.pairs.start;
         pair < node->data.mapping.pairs.top;
         ++pair) {
        key = yaml_document_get_node(doc, pair->key);
        XASSERT_NOT_NULL(key);
        XASSERT_EQ(key->type, YAML_SCALAR_NODE);
        key_str = (const char *) key->data.scalar.value;

        errno = 0;
        mode = strtol(key_str, NULL, 0);
        XASSERT_OK(errno);

        val = yaml_document_get_node(doc, pair->value);
        XASSERT_NOT_NULL(val);

        err = parse_mode(val, doc, mode, ctx);
        if (err != YOBD_OK) {
            return err;
        }
    }

    return YOBD_OK;
}

static
yobd_err parse_doc(
    yaml_node_t *node,
    yaml_document_t *doc,
    struct yobd_ctx *ctx)
{
    yobd_err err;
    yaml_node_t *key;
    const char *key_str;
    yaml_node_pair_t *pair;
    yaml_node_t *val;
    const char *val_str;

    err = YOBD_OK;
    XASSERT_EQ(node->type, YAML_MAPPING_NODE);
    for (pair = node->data.mapping.pairs.start;
         pair < node->data.mapping.pairs.top;
         ++pair) {
        key = yaml_document_get_node(doc, pair->key);
        XASSERT_NOT_NULL(key);
        XASSERT_EQ(key->type, YAML_SCALAR_NODE);
        key_str = (const char *) key->data.scalar.value;

        val = yaml_document_get_node(doc, pair->value);
        XASSERT_NOT_NULL(val);

        if (strcmp(key_str, "endian") == 0) {
            XASSERT_EQ(key->type, YAML_SCALAR_NODE);
            val_str = (const char *) val->data.scalar.value;
            ctx->big_endian = parse_endianness(val_str);
        }
        else if (strcmp(key_str, "modepid") == 0) {
            err = parse_modepid(val, doc, ctx);
            if (err != YOBD_OK) {
                return err;
            }
        }
        else {
            XASSERT_ERROR;
        }
    }

    return YOBD_OK;
}

static
yobd_err parse(FILE *file, struct yobd_ctx *ctx)
{
    yaml_document_t doc;
    yobd_err err;
    yaml_node_t *node;
    yaml_parser_t parser;
    int ret;

    ret = yaml_parser_initialize(&parser);
    if (ret == 0) {
        return YOBD_OOM;
    }
    yaml_parser_set_input_file(&parser, file);

    ret = yaml_parser_load(&parser, &doc);
    XASSERT_NEQ(ret, 0);

    node = yaml_document_get_root_node(&doc);
    XASSERT_NOT_NULL(node);

    err = parse_doc(node, &doc, ctx);

    yaml_document_delete(&doc);
    yaml_parser_delete(&parser);

    return err;
}

PUBLIC_API
yobd_err yobd_parse_schema(const char *schema, struct yobd_ctx **out_ctx)
{
    char abspath[PATH_MAX];
    struct yobd_ctx *ctx;
    yobd_err err;
    FILE *file;
    size_t i;
    bool is_path;
    size_t len;
    int ret;

    if (schema == NULL) {
        err = YOBD_INVALID_PARAMETER;
        goto out;
    }

    /* Determine if path is absolute or relative. */
    is_path = false;
    for (i = 0; i < PATH_MAX; ++i) {
        if (schema[i] == '\0') {
            break;
        }

        if (schema[i] == '/') {
            is_path = true;
        }
    }
    len = i;
    if (len == PATH_MAX) {
        err = YOBD_INVALID_PATH;
        goto out;
    }

    if (is_path) {
        file = fopen(schema, "r");
    }
    else {
        sprintf(abspath, "%s/%s", CONFIG_YOBD_PID_DIR, schema);
        file = fopen(abspath, "r");
    }
    if (file == NULL) {
        err = YOBD_CANNOT_OPEN_FILE;
        goto out;
    }

    ctx = malloc(sizeof(*ctx));
    if (ctx == NULL) {
        err = YOBD_OOM;
        goto error_malloc;
    }

    ctx->modepid_map = xh_init(MODEPID_MAP);
    if (ctx->modepid_map == NULL) {
        err = YOBD_OOM;
        goto error_modepid_map_init;
    }

    err = parse(file, ctx);
    if (err != YOBD_OK) {
        goto error_parse;
    }

    fclose(file);

    ret = xh_trim(MODEPID_MAP, ctx->modepid_map);
    if (ret == -1) {
        err = YOBD_OOM;
        goto error_trim;
    }

    *out_ctx = ctx;

    goto out;

error_trim:
error_parse:
    xh_destroy(MODEPID_MAP, ctx->modepid_map);
error_modepid_map_init:
    yobd_free_ctx(ctx);
error_malloc:
    fclose(file);
out:
    return err;
}
