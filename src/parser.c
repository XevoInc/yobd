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
#include <yobd_private/api.h>
#include <yobd_private/assert.h>
#include <yobd_private/expr.h>
#include <yobd_private/parser.h>

#define ARRAYLEN(a) (sizeof(a) / sizeof(a[0]))

struct parse_pid_ctx *get_parse_ctx(
    const struct yobd_ctx *ctx,
    yobd_mode mode,
    yobd_pid pid)
{
    xhiter_t iter;

    iter = xh_get(MODEPID_MAP, ctx->modepid_map, (mode << 16) | pid);
    if (iter == xh_end(ctx->modepid_map)) {
        return NULL;
    }

    return &xh_val(ctx->modepid_map, iter);
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
    struct parse_pid_ctx *parse_ctx;
    char *unit_str;

    if (ctx == NULL) {
        return;
    }

    XASSERT_NOT_NULL(ctx->unit_map);
    XASSERT_NOT_NULL(ctx->modepid_map);

    xh_iter(ctx->unit_map, iter,
        unit_str = (char *) xh_val(ctx->unit_map, iter);
        free(unit_str);
    );
    xh_destroy(UNIT_MAP, ctx->unit_map);

    xh_iter(ctx->modepid_map, iter,
        parse_ctx = &xh_val(ctx->modepid_map, iter);

        free((char *) parse_ctx->pid_desc.name);
        switch (parse_ctx->type) {
            case EVAL_TYPE_EXPR:
                destroy_expr(&parse_ctx->expr);
        }
    );
    xh_destroy(MODEPID_MAP, ctx->modepid_map);

    free(ctx);
}

PUBLIC_API
yobd_err yobd_get_unit_str(
    const struct yobd_ctx *ctx,
    yobd_unit id,
    const char **unit_str)
{
    xhiter_t iter;

    if (ctx == NULL || unit_str == NULL) {
        return YOBD_INVALID_PARAMETER;
    }

    iter = xh_get(UNIT_MAP, ctx->unit_map, id);
    if (iter == xh_end(ctx->unit_map)) {
        return YOBD_UNKNOWN_UNIT;
    }
    *unit_str = xh_val(ctx->unit_map, iter);

    return YOBD_OK;
}

typedef enum {
    MAP_NONE,
    MAP_ROOT,
    MAP_PIDS,
    MAP_SPECIFIC_PID,
    MAP_EXPR
} parse_map;

typedef enum {
    KEY_MODE,
    KEY_ENDIAN,
    KEY_PIDS,
    KEY_NAME,
    KEY_BYTES,
    KEY_UNIT,
    KEY_TYPE,
    KEY_EXPR,
    KEY_VAL,
    KEY_NONE
} parse_key;

struct parse_state {
    parse_map map;
    parse_key key;
};

static
parse_key find_key(const char *str)
{
    size_t i;
    /* Make sure this stays in sync with the parse_key enum! */
    static const char *keys[] = {
        [KEY_MODE] = "mode",
        [KEY_ENDIAN] = "endian",
        [KEY_PIDS] = "pids",
        [KEY_NAME] = "name",
        [KEY_BYTES] = "bytes",
        [KEY_UNIT] = "unit",
        [KEY_TYPE] = "type",
        [KEY_EXPR] = "expr",
        [KEY_VAL] = "val"
    };

    for (i = 0; i < ARRAYLEN(keys); ++i) {
        if (strcmp(keys[i], str) == 0) {
            return i;
        }
    }

    XASSERT_ERROR;
}

struct type_desc {
    const char *name;
    uint_fast8_t bytes;
};

static
void find_type(const char *str, uint_fast8_t *bytes, yobd_pid_data_type *type)
{
    size_t i;

    /* Make sure this stays in sync with the yobd_pid_data_type enum! */
    static struct type_desc types[] = {
        [YOBD_PID_DATA_TYPE_UINT8] =
            { .name = "uint8", .bytes = sizeof(uint8_t) },
        [YOBD_PID_DATA_TYPE_FLOAT] =
            { .name = "float", .bytes = sizeof(float) }
    };

    for (i = 0; i < ARRAYLEN(types); ++i) {
        if (strcmp(types[i].name, str) == 0) {
            *bytes = types[i].bytes;
            *type = i;
            return;
        }
    }

    XASSERT_ERROR;
}

static
yobd_err parse(struct yobd_ctx *ctx, FILE *file)
{
    bool done;
    yobd_err err;
    yaml_event_t event;
    xhiter_t iter;
    yobd_mode mode;
    yaml_parser_t parser;
    yobd_pid pid;
    struct parse_pid_ctx *parse_ctx;
    int ret;
    struct parse_state state;
    xhash_t(STR_SET) *unit_set;
    const char *unit_str;
    const char *val;

    err = YOBD_OK;

    unit_set = xh_init(STR_SET);
    if (unit_set == NULL) {
        return YOBD_OOM;
    }

    ret = yaml_parser_initialize(&parser);
    if (ret == 0) {
        return YOBD_OOM;
    }
    yaml_parser_set_input_file(&parser, file);

    state.map = MAP_NONE;
    state.key = KEY_NONE;
    done = false;
    do {
        ret = yaml_parser_parse(&parser, &event);
        if (ret == 0) {
            err = YOBD_OOM;
            break;
        }

        switch (event.type) {
            /* We don't handle these. */
            case YAML_NO_EVENT:
            case YAML_ALIAS_EVENT:
            case YAML_SEQUENCE_START_EVENT:
            case YAML_SEQUENCE_END_EVENT:
                XASSERT_ERROR;
                break;

            case YAML_STREAM_START_EVENT:
            case YAML_DOCUMENT_START_EVENT:
                break;
            case YAML_DOCUMENT_END_EVENT:
            case YAML_STREAM_END_EVENT:
                err = YOBD_OK;
                done = true;
                break;

            /* Start events push us one level down the map stack, while end
             * events pop us one level up the map stack.
             */
            case YAML_MAPPING_START_EVENT:
                switch (state.map) {
                    case MAP_NONE:
                        state.map = MAP_ROOT;
                        break;
                    case MAP_ROOT:
                        state.map = MAP_PIDS;
                        break;
                    case MAP_PIDS:
                        state.map = MAP_SPECIFIC_PID;
                        parse_ctx = put_mode_pid(ctx, mode, pid);
                        if (parse_ctx == NULL) {
                            err = YOBD_OOM;
                            done = true;
                        }
                        parse_ctx->pid_desc.name = NULL;

                        break;
                    case MAP_SPECIFIC_PID:
                        switch (state.key) {
                            case KEY_EXPR:
                                state.map = MAP_EXPR;
                                break;
                            default:
                                XASSERT_ERROR;
                        }
                        state.key = KEY_NONE;
                        break;
                    case MAP_EXPR:
                        XASSERT_ERROR;
                        break;
                }
                break;
            case YAML_MAPPING_END_EVENT:
                switch (state.map) {
                    case MAP_NONE:
                        XASSERT_ERROR;
                        break;
                    case MAP_ROOT:
                        break;
                    case MAP_PIDS:
                        state.map = MAP_ROOT;
                        break;
                    case MAP_SPECIFIC_PID:
                        state.map = MAP_PIDS;
                        state.key = KEY_PIDS;
                        break;
                    case MAP_EXPR:
                        state.map = MAP_SPECIFIC_PID;
                        break;
                }
                break;

            case YAML_SCALAR_EVENT:
                val = (const char *) event.data.scalar.value;
                switch (state.key) {
                    case KEY_NONE:
                        state.key = find_key(val);
                        break;

                    case KEY_MODE:
                        XASSERT_EQ(state.map, MAP_ROOT);
                        state.key = KEY_NONE;

                        errno = 0;
                        mode = strtol(val, NULL, 0);
                        XASSERT_EQ(errno, 0);
                        break;

                    case KEY_ENDIAN:
                        XASSERT_EQ(state.map, MAP_ROOT);
                        state.key = KEY_NONE;

                        if (strcmp(val, "big") == 0) {
                            ctx->big_endian = true;
                        }
                        else {
                            XASSERT_EQ(strcmp(val, "little"), 0);
                            ctx->big_endian = false;
                        }
                        break;

                    case KEY_PIDS:
                        XASSERT_EQ(state.map, MAP_PIDS);
                        state.key = KEY_NONE;

                        errno = 0;
                        pid = strtol(val, NULL, 0);
                        XASSERT_EQ(errno, 0);
                        break;

                    case KEY_NAME:
                        XASSERT_EQ(state.map, MAP_SPECIFIC_PID);
                        state.key = KEY_NONE;

                        XASSERT_NOT_NULL(parse_ctx);
                        XASSERT_NULL(parse_ctx->pid_desc.name);
                        parse_ctx->pid_desc.name = strdup(val);
                        if (parse_ctx->pid_desc.name == NULL) {
                            err = YOBD_OOM;
                            done = true;
                        }

                        break;

                    case KEY_BYTES:
                        XASSERT_EQ(state.map, MAP_SPECIFIC_PID);
                        state.key = KEY_NONE;

                        XASSERT_NOT_NULL(parse_ctx);
                        errno = 0;
                        parse_ctx->can_bytes = strtol(val, NULL, 0);
                        XASSERT_EQ(errno, 0);
                        break;

                    case KEY_UNIT:
                        XASSERT_EQ(state.map, MAP_SPECIFIC_PID);
                        state.key = KEY_NONE;

                        XASSERT_NOT_NULL(parse_ctx);

                        /* Check if we have seen this unit before. */
                        iter = xh_get(STR_SET, unit_set, val);
                        if (iter != xh_end(unit_set)) {
                            break;
                        }

                        /* This unit is new. */
                        unit_str = strdup(val);
                        if (unit_str == NULL) {
                            err = YOBD_OOM;
                            done = true;
                            break;
                        }

                        /* Get a new unit ID. */
                        parse_ctx->pid_desc.unit = ctx->next_unit_id;
                        ++ctx->next_unit_id;

                        /* Finally put the unit into our maps. */
                        iter = xh_put(
                            UNIT_MAP,
                            ctx->unit_map,
                            parse_ctx->pid_desc.unit,
                            &ret);
                        if (ret == -1) {
                            err = YOBD_OOM;
                            done = true;
                            break;
                        }
                        xh_val(ctx->unit_map, iter) = unit_str;

                        xh_put(STR_SET, unit_set, unit_str, &ret);
                        if (ret == -1) {
                            err = YOBD_OOM;
                            done = true;
                            break;
                        }

                        break;

                    case KEY_TYPE:
                        XASSERT_EQ(state.map, MAP_EXPR);
                        state.key = KEY_NONE;

                        XASSERT_NOT_NULL(parse_ctx);
                        find_type(
                            val,
                            &parse_ctx->pid_desc.bytes,
                            &parse_ctx->pid_desc.type);

                        break;

                    case KEY_EXPR:
                        XASSERT_EQ(state.map, MAP_SPECIFIC_PID);
                        state.map = MAP_EXPR;
                        state.key = KEY_NONE;
                        break;

                    case KEY_VAL:
                        XASSERT_EQ(state.map, MAP_EXPR);
                        state.key = KEY_NONE;

                        XASSERT_NOT_NULL(parse_ctx);
                        parse_ctx->type = EVAL_TYPE_EXPR;
                        err = parse_expr(
                            val,
                            &parse_ctx->expr,
                            parse_ctx->pid_desc.type);
                        if (err != YOBD_OK) {
                            done = true;
                        }

                        break;
                }
                break;
        }
        yaml_event_delete(&event);

    } while (!done);

    xh_destroy(STR_SET, unit_set);
    yaml_parser_delete(&parser);

    ret = xh_trim(UNIT_MAP, ctx->unit_map);
    if (ret == -1 && err == YOBD_OK) {
        err = YOBD_OOM;
    }

    ret = xh_trim(MODEPID_MAP, ctx->modepid_map);
    if (ret == -1 && err == YOBD_OK) {
        err = YOBD_OOM;
    }

    return err;
}

PUBLIC_API
yobd_err yobd_parse_schema(const char *filepath, struct yobd_ctx **out_ctx)
{
    struct yobd_ctx *ctx;
    yobd_err err;
    FILE *file;

    if (filepath == NULL) {
        err = YOBD_INVALID_PARAMETER;
        goto out;
    }

    file = fopen(filepath, "r");
    if (file == NULL) {
        err = YOBD_CANNOT_OPEN_FILE;
        goto out;
    }

    ctx = malloc(sizeof(*ctx));
    if (ctx == NULL) {
        err = YOBD_OOM;
        goto error_malloc;
    }

    ctx->unit_map = xh_init(UNIT_MAP);
    if (ctx->unit_map == NULL) {
        err = YOBD_OOM;
        goto error_unit_map_init;
    }

    ctx->modepid_map = xh_init(MODEPID_MAP);
    if (ctx->modepid_map == NULL) {
        err = YOBD_OOM;
        goto error_modepid_map_init;
    }

    ctx->next_unit_id = 0;

    err = parse(ctx, file);
    if (err != YOBD_OK) {
        goto error_parse;
    }

    fclose(file);
    *out_ctx = ctx;
    err = YOBD_OK;

    goto out;

error_parse:
    xh_destroy(MODEPID_MAP, ctx->modepid_map);
error_modepid_map_init:
    xh_destroy(UNIT_MAP, ctx->unit_map);
error_unit_map_init:
    free(ctx);
error_malloc:
    fclose(file);
out:
    return err;
}
