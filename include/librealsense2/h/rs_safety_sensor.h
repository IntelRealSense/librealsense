/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2024 Intel Corporation. All Rights Reserved. */

/** \file rs_safety_sensor.h
* \brief
* Exposes RealSense safety sensor functionality for C compilers
*/


#ifndef LIBREALSENSE_RS2_SAFETY_SENSOR_H
#define LIBREALSENSE_RS2_SAFETY_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rs_types.h"

/**
* rs2_get_safety_preset
* \param[in]   sensor        Safety sensor
* \param[in]   index         Index to read from
* \param[out]  error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return                    JSON string representing the safety preset at the given index as rs2_raw_data_buffer
*/
const rs2_raw_data_buffer* rs2_get_safety_preset(rs2_sensor const* sensor, int index, rs2_error** error);

/**
* rs2_set_safety_preset
* \param[in]  sensor        Safety sensor
* \param[in]  index         Index to write to
* \param[in]  sp_json_str   Safety preset as JSON string
* \param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_set_safety_preset(rs2_sensor const* sensor, int index, const char* sp_json_str, rs2_error** error);

/**
* rs2_get_safety_interface_config
* \param[in]   sensor        Safety sensor
* \param[out]  loc           Calibration location that needs to be read from
* \param[out]  error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return                    JSON string representing the safety interface config as rs2_raw_data_buffer
*/
const rs2_raw_data_buffer*  rs2_get_safety_interface_config(rs2_sensor const* sensor, rs2_calib_location loc, rs2_error** error);

/**
* rs2_set_safety_interface_config
* \param[in]  sensor          Safety sensor
* \param[in]  sic_json_str    Safety interface config as JSON string
* \param[out] error           If non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_set_safety_interface_config(rs2_sensor const* sensor, const char* sic_json_str, rs2_error** error);

/**
* rs2_get_application_config
* \param[in]  sensor        Safety sensor
* \param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return                   JSON string representing the application config as rs2_raw_data_buffer
*/
const rs2_raw_data_buffer* rs2_get_application_config(rs2_sensor const* sensor, rs2_error** error);

/**
* rs2_set_application_config
* \param[in]  sensor                           Safety sensor
* \param[in]  application_config_json_str      Application config as JSON string
* \param[out] error                            If non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_set_application_config(rs2_sensor const* sensor, const char* application_config_json_str,  rs2_error** error);

#ifdef __cplusplus
}
#endif
#endif  // LIBREALSENSE_RS2_SAFETY_SENSOR_H
