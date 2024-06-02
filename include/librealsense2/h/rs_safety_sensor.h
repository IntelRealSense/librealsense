/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2024 Intel Corporation. All Rights Reserved. */

/** \file rs_sensor.h
* \brief
* Exposes RealSense sensor functionality for C compilers
*/


#ifndef LIBREALSENSE_RS2_SAFETY_SENSOR_H
#define LIBREALSENSE_RS2_SAFETY_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rs_types.h"
#include "rs_safety_types.h"

/**
* rs2_get_safety_preset
* \param[in]   sensor        Safety sensor
* \param[in]   index         Index to read from
* \param[out]  sp            Safety preset struct result
* \param[out]  error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_get_safety_preset(rs2_sensor const* sensor, int index, rs2_safety_preset* sp, rs2_error** error);

/**
* rs2_set_safety_preset
* \param[in]  sensor        Safety sensor
* \param[in]  index         Index to write to
* \param[in]  sp            Safety preset struct to set
* \param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_set_safety_preset(rs2_sensor const* sensor, int index, rs2_safety_preset const* sp, rs2_error** error);

/**
* rs2_json_string_to_safety_preset
* \param[in]  sensor        Safety sensor
* \param[in]  json_str      JSON string to convert
* \param[out] sp            Safety preset struct result
* \param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_json_string_to_safety_preset(rs2_sensor const* sensor, const char* json_str, rs2_safety_preset* sp, rs2_error** error);

/**
* rs2_safety_preset_to_json_string
* \param[in]  sensor        Safety sensor
* \param[in]  sp            Safety preset to convert
* \param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return                   JSON string representing the safety preset
*/
const char* rs2_safety_preset_to_json_string(rs2_sensor const* sensor, rs2_safety_preset const* sp, rs2_error** error);

/**
* rs2_get_safety_interface_config
* \param[in]   sensor        Safety sensor
* \param[out]  sic           Safety Interface Config struct result
* \param[out]  loc           Calibration location that needs to be read from
* \param[out]  error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_get_safety_interface_config(rs2_sensor const* sensor, rs2_safety_interface_config* sic, rs2_calib_location loc, rs2_error** error);

/**
* rs2_set_safety_interface_config
* \param[in]  sensor        Safety sensor
* \param[in]  sic           Safety Interface Config struct to set
* \param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_set_safety_interface_config(rs2_sensor const* sensor, rs2_safety_interface_config const* sic, rs2_error** error);

#ifdef __cplusplus
}
#endif
#endif  // LIBREALSENSE_RS2_SAFETY_SENSOR_H
