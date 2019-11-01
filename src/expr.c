/**
 * @file      expr.c
 * @brief     yobd expression parser implementation.
 * @author    Martin Kelly <mkelly@xevo.com>
 * @copyright Copyright (C) 2017 Xevo Inc. All Rights Reserved.
 */

#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <yobd/yobd.h>
#include <yobd-private/assert.h>
#include <yobd-private/expr.h>
#include <yobd-private/stack.h>

#define ARRAYLEN(a) (sizeof(a) / sizeof(a[0]))
#define OP_STACK_SIZE (20)
#define OUT_STACK_SIZE (50)

typedef enum {
    TOK_A,
    TOK_B,
    TOK_C,
    TOK_D,
    TOK_NUMERIC,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_OP_ADD,
    TOK_OP_SUB,
    TOK_OP_MUL,
    TOK_OP_DIV
} parse_token;

DEFINE_STACK(OP_STACK, parse_token)

static
void next_token(
    const char *str,
    const char **start,
    const char **end,
    parse_token *type)
{
    const char *pos;

    /* Consume whitespace. */
    for (pos = str; isspace(*pos); ++pos);

    if (*pos == '-' && *(pos+1) != '\0' && isdigit(*(pos+1))) {
        *start = pos;
        ++pos;
    }
    else {
        *start = NULL;
    }

    /* Numbers are the only multi-character tokens. */
    if (isdigit(*pos)) {
        *type = TOK_NUMERIC;
        if (*start == NULL) {
            *start = pos;
        }

        /* Read the rest of the number. */
        do {
            ++pos;
        } while (isdigit(*pos));

        /* Check for float. */
        if (*pos == '.') {
            /* Read the fractional part. */
            do {
                ++pos;
            } while (isdigit(*pos));
        }

        *end = pos;
    }
    else if (*pos == '\0') {
        *end = NULL;
    }
    else {
        switch (*pos) {
            case '(':
                *type = TOK_LPAREN;
                break;
            case ')':
                *type = TOK_RPAREN;
                break;
            case 'A':
                *type = TOK_A;
                break;
            case 'B':
                *type = TOK_B;
                break;
            case 'C':
                *type = TOK_C;
                break;
            case 'D':
                *type = TOK_D;
                break;
            case '+':
                *type = TOK_OP_ADD;
                break;
            case '-':
                *type = TOK_OP_SUB;
                break;
            case '*':
                *type = TOK_OP_MUL;
                break;
            case '/':
                *type = TOK_OP_DIV;
                break;
            default:
                /* Unrecognized token, should have failed schema checking. */
                XASSERT_ERROR;
        }

        *start = pos;
        *end = pos + 1;
    }
}

static
void op_to_expr(parse_token tok, struct expr_token *expr_tok)
{
    expr_tok->type = EXPR_OP;
    switch (tok) {
        case TOK_OP_ADD:
            expr_tok->as_op = EXPR_OP_ADD;
            break;
        case TOK_OP_SUB:
            expr_tok->as_op = EXPR_OP_SUB;
            break;
        case TOK_OP_MUL:
            expr_tok->as_op = EXPR_OP_MUL;
            break;
        case TOK_OP_DIV:
            expr_tok->as_op = EXPR_OP_DIV;
            break;
        default:
            XASSERT_ERROR;
    }
}

void handle_op(
    parse_token tok,
    struct OP_STACK *op_stack,
    struct EXPR_STACK *out_stack)
{
    struct expr_token expr_tok;
    parse_token op_tok;
    int ret;

    /* Pop all higher precedence operators onto the output stack. */
    while (true) {
        ret = PEEK_STACK(OP_STACK, op_stack, &op_tok);
        if (ret == -1 || (op_tok != TOK_OP_MUL && op_tok != TOK_OP_DIV)) {
            break;
        }
        op_tok = POP_STACK(OP_STACK, op_stack);
        op_to_expr(op_tok, &expr_tok);
        PUSH_STACK(EXPR_STACK, out_stack, &expr_tok);
    }
    /* Now push the lower precedence operator. */
    PUSH_STACK(OP_STACK, op_stack, &tok);
}

void shunting_yard(
    const char *str,
    pid_data_type type,
    struct OP_STACK *op_stack,
    struct EXPR_STACK *out_stack)
{
    const char *end;
    struct expr_token expr_tok;
    const char *pos;
    parse_token op_tok;
    int ret;
    const char *start;
    parse_token tok;

    /*
     * Dijkstra's Shunting Yard algorithm:
     * https://en.wikipedia.org/wiki/Shunting-yard_algorithm
     * Thanks Dijkstra!
     */

    pos = str;
    while (true) {
        next_token(pos, &start, &end, &tok);
        if (end == NULL) {
            /* No more tokens. */
            break;
        }

        pos = end;
        switch (tok) {
            case TOK_NUMERIC:
                errno = 0;
                switch (type) {
                    case PID_DATA_TYPE_UINT8:
                    case PID_DATA_TYPE_UINT16:
                    case PID_DATA_TYPE_INT8:
                    case PID_DATA_TYPE_INT16:
                        expr_tok.as_int32_t = strtol((const char *) start, NULL, 10);
                        expr_tok.type = EXPR_INT32;
                        break;
                    case PID_DATA_TYPE_FLOAT:
                        expr_tok.as_float = strtof((const char *) start, NULL);
                        expr_tok.type = EXPR_FLOAT;
                        break;
                }
                XASSERT_EQ(errno, 0);
                PUSH_STACK(EXPR_STACK, out_stack, &expr_tok);
                break;

            case TOK_A:
                expr_tok.type = EXPR_A;
                PUSH_STACK(EXPR_STACK, out_stack, &expr_tok);
                break;
            case TOK_B:
                expr_tok.type = EXPR_B;
                PUSH_STACK(EXPR_STACK, out_stack, &expr_tok);
                break;
            case TOK_C:
                expr_tok.type = EXPR_C;
                PUSH_STACK(EXPR_STACK, out_stack, &expr_tok);
                break;
            case TOK_D:
                expr_tok.type = EXPR_D;
                PUSH_STACK(EXPR_STACK, out_stack, &expr_tok);
                break;

            case TOK_OP_ADD:
            case TOK_OP_SUB:
                handle_op(tok, op_stack, out_stack);
                break;

            case TOK_OP_MUL:
            case TOK_OP_DIV:
                /* This operator is high precedence, so just push it. */
                PUSH_STACK(OP_STACK, op_stack, &tok);
                break;

            case TOK_LPAREN:
                PUSH_STACK(OP_STACK, op_stack, &tok);
                break;

            case TOK_RPAREN:
                while (true) {
                    ret = PEEK_STACK(OP_STACK, op_stack, &tok);
                    /* We should not see a ) without a ( before it. */
                    XASSERT_NEQ(ret, -1);
                    if (tok == TOK_LPAREN) {
                        break;
                    }
                    op_tok = POP_STACK(OP_STACK, op_stack);
                    op_to_expr(op_tok, &expr_tok);
                    PUSH_STACK(EXPR_STACK, out_stack, &expr_tok);
                }
                /* Pop the left bracket. */
                POP_STACK(OP_STACK, op_stack);
                break;
        }
    }

    /* Push all remaining operators onto the output. */
    while (STACK_SIZE(OP_STACK, op_stack) > 0) {
        op_tok = POP_STACK(OP_STACK, op_stack);
        op_to_expr(op_tok, &expr_tok);
        PUSH_STACK(EXPR_STACK, out_stack, &expr_tok);
    }

    XASSERT_GT(STACK_SIZE(EXPR_STACK, out_stack), 0);
}

yobd_err parse_expr(
    const char *str,
    struct expr *expr,
    pid_data_type type)
{
    struct expr_token *data;
    size_t expr_bytes;
    parse_token op_data[OP_STACK_SIZE];
    struct OP_STACK op_stack;
    struct expr_token out_data[OUT_STACK_SIZE];
    struct EXPR_STACK out_stack;

    INIT_STACK(OP_STACK, &op_stack, op_data, ARRAYLEN(op_data));
    INIT_STACK(EXPR_STACK, &out_stack, out_data, ARRAYLEN(out_data));

    shunting_yard(str, type, &op_stack, &out_stack);

    /* Make our expression data the right size. */
    expr->size = STACK_SIZE(EXPR_STACK, &out_stack);
    expr_bytes = expr->size * sizeof(*expr->data);
    expr->data = malloc(expr_bytes);
    if (expr->data == NULL) {
        return YOBD_OOM;
    }

    /*
     * Note that in the original Shunting Yard algorithm, we use an output queue
     * instead of an output stack. By copying from the stack bottom to the stack
     * top, we effectively reverse the stack order and turn it into a queue so
     * that it will be correctly evaluated.
     */
    memcpy(expr->data, STACK_DATA(EXPR_STACK, &out_stack), expr_bytes);

    /*
     * Alloc an evaluation stack for the expression. Note that we give the
     * evaluation stack the same size as the expression data. This is an
     * upper-bound, and we could likely do better with a bit of cleverness.
     */
    data = malloc(expr_bytes);
    if (data == NULL) {
        return YOBD_OOM;
    }
    INIT_STACK(EXPR_STACK, &expr->stack, data, expr->size);

    return YOBD_OK;
}

void destroy_expr(struct expr *expr)
{
    free(expr->data);
    free(STACK_DATA(EXPR_STACK, &expr->stack));
}
