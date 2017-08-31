#ifndef YOBD_PRIVATE_PARSER_H_
#define YOBD_PRIVATE_PARSER_H_

#include <stdint.h>
#include <yobd/yobd.h>

struct yobd_pid_ctx *get_mode_pid(
    const struct yobd_ctx *ctx,
    yobd_mode mode,
    yobd_pid pid);

#endif /* YOBD_PRIVATE_PARSER_H_ */
