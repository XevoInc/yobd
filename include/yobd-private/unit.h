/**
 * @file      unit.h
 * @brief     yobd unit handling code.
 * @author    Martin Kelly <mkelly@xevo.com>
 * @copyright Copyright (C) 2018 Xevo Inc. All Rights Reserved.
 */

#ifndef YOBD_PRIVATE_UNIT_H_
#define YOBD_PRIVATE_UNIT_H_

#include <yobd/yobd.h>

typedef float (*convert_func)(float val);

/**
 * Returns the conversion function (converting to SI units) for a given unit.
 * @param unit a unit ID
 * @return conversion function pointer
 */
convert_func find_convert_func(const char *raw_unit);

#endif /* YOBD_PRIVATE_UNIT_H_ */
