/**
 * @file      stack.h
 * @brief     Generic macro-based stack implementation.
 * @author    Martin Kelly <mkelly@xevo.com>
 * @copyright Copyright (C) 2017 Xevo Inc. All Rights Reserved.
 */

#ifndef YOBD_PRIVATE_STACK_H_
#define YOBD_PRIVATE_STACK_H_

#include <string.h>
#include <yobd_private/assert.h>

#define DEFINE_STACK(stack_type, item_type) \
    struct stack_type { \
        size_t max_size; \
        item_type *top; \
        item_type *bottom; \
    }; \
    \
    static inline __attribute__ ((__unused__)) \
    void stack_type##_init( \
        struct stack_type *stack, \
        item_type *data, \
        size_t max_size) \
    { \
        stack->max_size = max_size; \
        stack->top = data; \
        stack->bottom = data; \
    } \
    \
    static inline __attribute__ ((__unused__)) \
    size_t stack_type##_max_size(const struct stack_type *stack) \
    { \
        return stack->max_size; \
    } \
    \
    static inline __attribute__ ((__unused__)) \
    item_type *stack_type##_data(const struct stack_type *stack) \
    { \
        return stack->bottom; \
    } \
    \
    static inline __attribute__ ((__unused__)) \
    size_t stack_type##_size(const struct stack_type *stack) \
    { \
        return stack->top - stack->bottom; \
    } \
    \
    static inline __attribute__ ((__unused__)) \
    void stack_type##_push(struct stack_type *stack, item_type *item) \
    { \
        XASSERT_LT(stack_type##_size(stack), stack_type##_max_size(stack)); \
        \
        memcpy(stack->top, item, sizeof(*stack->top)); \
        ++stack->top; \
    } \
    static inline __attribute__ ((__unused__)) \
    item_type stack_type##_pop(struct stack_type *stack) \
    { \
        item_type item; \
        \
        XASSERT_GT(stack_type##_size(stack), 0); \
        \
        --stack->top; \
        memcpy(&item, stack->top, sizeof(item)); \
        return item; \
    } \
    static inline __attribute__ ((__unused__)) \
    int stack_type##_peek(const struct stack_type *stack, item_type *item) \
    { \
        if (stack_type##_size(stack) == 0) { \
            return -1; \
        } \
        memcpy(item, stack->top-1, sizeof(*item)); \
        return 0; \
    }

#define INIT_STACK(name, stack, data, max_size) name##_init(stack, data, max_size)
#define PUSH_STACK(name, stack, item) name##_push(stack, item)
#define POP_STACK(name, stack) name##_pop(stack)
#define PEEK_STACK(name, stack, ret) name##_peek(stack, ret)
#define STACK_DATA(name, stack) name##_data(stack)
#define STACK_SIZE(name, stack) name##_size(stack)
#define STACK_MAX_SIZE(name, stack) name##_max_size(stack)

#endif /* YOBD_PRIVATE_STACK_H_ */
