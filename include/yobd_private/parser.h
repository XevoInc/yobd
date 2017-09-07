#ifndef YOBD_PRIVATE_PARSER_H_
#define YOBD_PRIVATE_PARSER_H_

#include <stdint.h>
#include <xlib/xhash.h>
#include <yobd/yobd.h>

typedef enum {
    EVAL_TYPE_EXPR
} eval_type;

struct parse_pid_ctx {
    struct yobd_pid_desc pid_desc;
    /* The byte count of the CAN response, not of the OBD II response. */
    uint_fast8_t can_bytes;
    eval_type type;
    union {
        struct expr expr;
    };
};

struct parse_pid_ctx *get_parse_ctx(
    const struct yobd_ctx *ctx,
    yobd_mode mode,
    yobd_pid pid);

XHASH_MAP_INIT_INT8(UNIT_MAP, const char *)
XHASH_MAP_INIT_INT(MODEPID_MAP, struct parse_pid_ctx)
XHASH_SET_INIT_STR(STR_SET)

struct yobd_ctx {
    yobd_unit next_unit_id;
    bool big_endian;
    xhash_t(UNIT_MAP) *unit_map;
    xhash_t(MODEPID_MAP) *modepid_map;
};

#endif /* YOBD_PRIVATE_PARSER_H_ */
