/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

/** \file rs2_option.h
* \brief
* Exposes sensor options functionality for C compilers
*/


#ifndef LIBREALSENSE_RS2_OPTION_H
#define LIBREALSENSE_RS2_OPTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rs2_types.h"

/**
* check if an option is read-only
* \param[in] sensor   the RealSense sensor
* \param[in] option   option id to be checked
* \param[out] error   if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return true if option is read-only
*/
int rs2_is_option_read_only(const rs2_sensor* sensor, rs2_option option, rs2_error** error);

/**
* read option value from the sensor
* \param[in] sensor   the RealSense sensor
* \param[in] option   option id to be queried
* \param[out] error   if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return value of the option
*/
float rs2_get_option(const rs2_sensor* sensor, rs2_option option, rs2_error** error);

/**
* write new value to sensor option
* \param[in] sensor     the RealSense sensor
* \param[in] option     option id to be queried
* \param[in] value      new value for the option
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_set_option(const rs2_sensor* sensor, rs2_option option, float value, rs2_error** error);

/**
* check if particular option is supported by a subdevice
* \param[in] sensor     the RealSense sensor
* \param[in] option     option id to be checked
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return true if option is supported
*/
int rs2_supports_option(const rs2_sensor* sensor, rs2_option option, rs2_error** error);

/**
* retrieve the available range of values of a supported option
* \param[in] sensor  the RealSense device
* \param[in] option  the option whose range should be queried
* \param[out] min    the minimum value which will be accepted for this option
* \param[out] max    the maximum value which will be accepted for this option
* \param[out] step   the granularity of options which accept discrete values, or zero if the option accepts continuous values
* \param[out] def    the default value of the option
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_get_option_range(const rs2_sensor* sensor, rs2_option option, float* min, float* max, float* step, float* def, rs2_error** error);

/**
* get option description
* \param[in] sensor     the RealSense sensor
* \param[in] option     option id to be checked
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return human-readable option description
*/
const char* rs2_get_option_description(const rs2_sensor* sensor, rs2_option option, rs2_error ** error);

/**
* get option value description (in case specific option value hold special meaning)
* \param[in] device     the RealSense device
* \param[in] option     option id to be checked
* \param[in] value      value of the option
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return human-readable description of a specific value of an option or null if no special meaning
*/
const char* rs2_get_option_value_description(const rs2_sensor* sensor, rs2_option option, float value, rs2_error ** error);

#ifdef __cplusplus
}
#endif
#endif
