/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

/** \file rs2_sensor.h
* \brief
* Exposes RealSense sensor functionality for C compilers
*/


#ifndef LIBREALSENSE_RS2_SENSOR_H
#define LIBREALSENSE_RS2_SENSOR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rs2_types.h"
#include "rs2_device.h"

/**
* create a static snapshot of all connected sensors within specific device
* \param devuice     Specific RealSense device
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return            the list of devices, should be released by rs2_delete_device_list
*/
rs2_sensor_list* rs2_query_sensors(const rs2_device* device, rs2_error** error);

/**
* Deletes sensors list, any sensors created from this list will remain unaffected
* \param[in] info_list list to delete
*/
void rs2_delete_sensor_list(rs2_sensor_list* info_list);

/**
* Determines number of sensors in a list
* \param[in] info_list The list of connected sensors captured using rs2_query_sensors
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return            Sensors count
*/
int rs2_get_sensors_count(const rs2_sensor_list* info_list, rs2_error** error);

/**
* delete relasense sensor
* \param[in] sensor realsense sensor to delete
*/
void rs2_delete_sensor(rs2_sensor* sensor);

/**
* create sensor by index
* \param[in] index   the zero based index of sensor to retrieve
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return            the requested sensor, should be released by rs2_delete_sensor
*/
rs2_sensor* rs2_create_sensor(const rs2_sensor_list* list, int index, rs2_error** error);

rs2_device* rs2_create_device_from_sensor(const rs2_sensor* sensor, rs2_error ** error);

/**
* retrieve sensor specific information, like versions of various internal components
* \param[in] sensor     the RealSense sensor
* \param[in] info       camera info type to retrieve
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               the requested camera info string, in a format specific to the device model
*/
const char* rs2_get_sensor_info(const rs2_sensor* sensor, rs2_camera_info info, rs2_error** error);

/**
* check if specific sensor info is supported
* \param[in] info    the parameter to check for support
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return                true if the parameter both exist and well-defined for the specific device
*/
int rs2_supports_sensor_info(const rs2_sensor* sensor, rs2_camera_info info, rs2_error** error);

/**
 * Test if the given sensor can be extended to the requested extension
 * \param[in] sensor  Realsense sensor
 * \param[in] extension The extension to which the sensor should be tested if it is extendable
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 * \return non-zero value iff the sensor can be extended to the given extension
 */
int rs2_is_sensor_extendable_to(const rs2_sensor* sensor, rs2_extension extension, rs2_error ** error);

float rs2_get_depth_scale(rs2_sensor* sensor, rs2_error** error);

/**
 * \brief sets the active region of interest to be used by auto-exposure algorithm
 * \param[in] sensor     the RealSense sensor
 * \param[in] min_x      lower horizontal bound in pixels
 * \param[in] min_y      lower vertical bound in pixels
 * \param[in] max_x      upper horizontal bound in pixels
 * \param[in] max_y      upper vertical bound in pixels
 * \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 */
void rs2_set_region_of_interest(const rs2_sensor* sensor, int min_x, int min_y, int max_x, int max_y, rs2_error ** error);

/**
 * \brief gets the active region of interest to be used by auto-exposure algorithm
 * \param[in] sensor     the RealSense sensor
 * \param[out] min_x     lower horizontal bound in pixels
 * \param[out] min_y     lower vertical bound in pixels
 * \param[out] max_x     upper horizontal bound in pixels
 * \param[out] max_y     upper vertical bound in pixels
 * \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 */
void rs2_get_region_of_interest(const rs2_sensor* sensor, int* min_x, int* min_y, int* max_x, int* max_y, rs2_error ** error);

#ifdef __cplusplus
}
#endif
#endif
