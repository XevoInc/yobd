/**
 * @file      types.h
 * @brief     yobd data types header.
 * @author    Martin Kelly <mkelly@xevo.com>
 * @copyright Copyright (C) 2017 Xevo Inc. All Rights Reserved.
 */

#ifndef YOBD_PRIVATE_TYPES_H_
#define YOBD_PRIVATE_TYPES_H_

typedef enum {
    PID_DATA_TYPE_UINT8 = 0,
    PID_DATA_TYPE_UINT16 = 1,
    PID_DATA_TYPE_INT8 = 2,
    PID_DATA_TYPE_INT16 = 3,
    PID_DATA_TYPE_FLOAT = 4
} pid_data_type;

#endif /* YOBD_PRIVATE_TYPES_H_ */
