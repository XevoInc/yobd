/**
 * @file      unit.h
 * @brief     yobd unit handling code.
 * @author    Martin Kelly <mkelly@xevo.com>
 * @copyright Copyright (C) 2018 Xevo Inc. All Rights Reserved.
 */

#ifndef YOBD_PRIVATE_UNIT_H_
#define YOBD_PRIVATE_UNIT_H_

#include <yobd/yobd.h>

typedef float (*to_si)(float val);

struct unit_desc {
    const char *name;
    to_si convert;
};

/**
 * Returns the conversion function (converting to SI units) for a given unit.
 * @param unit a unit ID
 * @return conversion function pointer
 */
to_si get_conversion(const char *unit);

#endif /* YOBD_PRIVATE_UNIT_H_ */
