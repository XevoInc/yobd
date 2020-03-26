/**
 * @file      unit.c
 * @brief     yobd unit handling code.
 * @author    Martin Kelly <mkelly@xevo.com>
 * @copyright Copyright (C) 2017 Xevo Inc. All Rights Reserved.
 */

#include <math.h>
#include <string.h>
#include <yobd/yobd.h>
#include <yobd-private/assert.h>
#include <yobd-private/unit.h>

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
float degree_to_rad(float val)
{
    return val * (PI / 180.0);
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

static
float s_to_ns(float val)
{
    return val * ((float) 1e9);
}

convert_func find_convert_func(const char *raw_unit)
{
    /*
     * Please keep this list sorted to prevent duplicates. If the list gets
     * large enough, we could consider switching to a hash map.
     */
    if (strcmp(raw_unit, "celsius") == 0) {
        return celsius_to_k;
    }
    if (strcmp(raw_unit, "degree") == 0) {
        return degree_to_rad;
    }
    else if (strcmp(raw_unit, "g/s") == 0) {
        return gs_to_kgs;
    }
    else if (strcmp(raw_unit, "K") == 0) {
        return nop;
    }
    else if (strcmp(raw_unit, "kg/s") == 0) {
        return nop;
    }
    else if (strcmp(raw_unit, "km") == 0) {
        return km_to_m;
    }
    else if (strcmp(raw_unit, "km/h") == 0) {
        return kmh_to_ms;
    }
    else if (strcmp(raw_unit, "kPa") == 0) {
        return kpa_to_pa;
    }
    else if (strcmp(raw_unit, "lat") == 0) {
        return nop;
    }
    else if (strcmp(raw_unit, "lng") == 0) {
        return nop;
    }
    else if (strcmp(raw_unit, "m") == 0) {
        return nop;
    }
    else if (strcmp(raw_unit, "m/s") == 0) {
        return nop;
    }
    else if (strcmp(raw_unit, "m/s^2") == 0) {
        return nop;
    }
    else if (strcmp(raw_unit, "nm") == 0) {
        return nm_to_m;
    }
    else if (strcmp(raw_unit, "ns") == 0) {
        return nop;
    }
    else if (strcmp(raw_unit, "Pa") == 0) {
        return nop;
    }
    else if (strcmp(raw_unit, "percent") == 0) {
        return nop;
    }
    else if (strcmp(raw_unit, "rad/s") == 0) {
        return nop;
    }
    else if (strcmp(raw_unit, "rpm") == 0) {
        return rpm_to_rads;
    }
    else if (strcmp(raw_unit, "s") == 0) {
        return s_to_ns;
    }

    /* We need to add a new conversion function. */
    XASSERT_ERROR;
}
