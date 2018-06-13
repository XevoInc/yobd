/**
 * @file      unit.c
 * @brief     yobd unit handling code.
 * @author    Martin Kelly <mkelly@xevo.com>
 * @copyright Copyright (C) 2017 Xevo Inc. All Rights Reserved.
 */

#include <string.h>
#include <yobd/yobd.h>
#include <yobd_private/assert.h>
#include <yobd_private/unit.h>

#define PI 3.141593f

static
float nop(float val)
{
    return val;
}

static
float celsius_to_k(float val)
{
    return val + 273.15;
}

static
float gs_to_kgs(float val)
{
    return val / 1000.0;
}

static
float km_to_m(float val)
{
    return val * 1000.0;
}

static
float kmh_to_ms(float val)
{
    return km_to_m(val) / (60.0*60.0);
}

static
float kpa_to_pa(float val)
{
    return val * 1000.0;
}

static
float nm_to_m(float val)
{
    return val / ((float) 1e-9);
}

static
float rpm_to_rads(float val)
{
    /* rad/s = 2*pi/60 * rpm */
    return val * PI / 30.0;
}

to_si get_conversion(const char *unit)
{
    to_si convert;

    /*
     * Please keep this list sorted to prevent duplicates. If the list gets
     * large enough, we could consider switching to a hash map.
     */
    if (strcmp(unit, "celsius") == 0) {
        convert = celsius_to_k;
    }
    else if (strcmp(unit, "g/s") == 0) {
        convert = gs_to_kgs;
    }
    else if (strcmp(unit, "K") == 0) {
        convert = nop;
    }
    else if (strcmp(unit, "kg/s") == 0) {
        convert = nop;
    }
    else if (strcmp(unit, "km") == 0) {
        convert = km_to_m;
    }
    else if (strcmp(unit, "km/h") == 0) {
        convert = kmh_to_ms;
    }
    else if (strcmp(unit, "kPa") == 0) {
        convert = kpa_to_pa;
    }
    else if (strcmp(unit, "m") == 0) {
        convert = nop;
    }
    else if (strcmp(unit, "m/s") == 0) {
        convert = nop;
    }
    else if (strcmp(unit, "nm") == 0) {
        convert = nm_to_m;
    }
    else if (strcmp(unit, "Pa") == 0) {
        convert = nop;
    }
    else if (strcmp(unit, "percent") == 0) {
        convert = nop;
    }
    else if (strcmp(unit, "rad/s") == 0) {
        convert = nop;
    }
    else if (strcmp(unit, "rpm") == 0) {
        convert = rpm_to_rads;
    }
    else if (strcmp(unit, "s") == 0) {
        convert = nop;
    }
    else {
        /* We need to add a new conversion function. */
        XASSERT_ERROR;
    }

    return convert;
}
