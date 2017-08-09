/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

/** \file rs2_device.h
* \brief
* Exposes RealSense device functionality for C compilers
*/


#ifndef LIBREALSENSE_RS2_DEVICE_H
#define LIBREALSENSE_RS2_DEVICE_H

#ifdef __cplusplus
extern "C" {
#endif
#include "rs2_types.h"
#include "rs2_sensor.h"

/** \brief Motion device intrinsics: scale, bias, and variances */
typedef struct rs2_motion_device_intrinsic
{
    /* Scale X        cross axis        cross axis      Bias X */
    /* cross axis     Scale Y           cross axis      Bias Y */
    /* cross axis     cross axis        Scale Z         Bias Z */
    float data[3][4];          /**< Interpret data array values */

    float noise_variances[3];  /**< Variance of noise for X, Y, and Z axis */
    float bias_variances[3];   /**< Variance of bias for X, Y, and Z axis */
} rs2_motion_device_intrinsic;

/**
* Determines number of devices in a list
* \param[in] info_list The list of connected devices captured using rs2_query_devices
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return            Device count
*/
int rs2_get_device_count(const rs2_device_list* info_list, rs2_error** error);

/**
* Deletes device list, any devices created from this list will remain unaffected
* \param[in] info_list list to delete
*/
void rs2_delete_device_list(rs2_device_list* info_list);

/**
* this function returns true if the specific device is contained inside the device list "removed"
* \param[in] device    RealSense device
* \param[in] event_information    handle returned from a callback
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return            true if the device was disconnected and false otherwise
*/
int rs2_device_list_contains(const rs2_device_list* removed, const rs2_device* dev, rs2_error** error);

/**
* create device by index
* \param[in] index   the zero based index of device to retrieve
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return            the requested device, should be released by rs2_delete_device
*/
rs2_device* rs2_create_device(const rs2_device_list* list, int index, rs2_error** error);

/**
* delete relasense device
* \param[in] device realsense device to delete
*/
void rs2_delete_device(rs2_device* device);

/**
* retrieve camera specific information, like versions of various internal components
* \param[in] device     the RealSense device
* \param[in] info       camera info type to retrieve
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               the requested camera info string, in a format specific to the device model
*/
const char* rs2_get_device_info(const rs2_device* device, rs2_camera_info info, rs2_error** error);

/**
* check if specific camera info is supported
* \param[in] info    the parameter to check for support
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return                true if the parameter both exist and well-defined for the specific device
*/
int rs2_supports_device_info(const rs2_device* device, rs2_camera_info info, rs2_error** error);

/**
 * send hardware reset request to the device
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 */
void rs2_hardware_reset(const rs2_device * device, rs2_error ** error);

/**
* send raw data to device
* \param[in] device  input RealSense device
* \param[in] raw_data_to_send   raw data to be send to device
* \param[in] size_of_raw_data_to_send   size of raw_data_to_send
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return            rs2_raw_data_buffer, should be released by rs2_delete_raw_data
*/
const rs2_raw_data_buffer* rs2_send_and_receive_raw_data(rs2_device* device, void* raw_data_to_send, unsigned size_of_raw_data_to_send, rs2_error** error);

/**
 * returns the intrinsics of specific stream configuration
 * \param[in]  device    RealSense device to query
 * \param[in]  stream    type of stream
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 */
void rs2_get_motion_intrinsics(const rs2_sensor * device, rs2_stream stream, rs2_motion_device_intrinsic * intrinsics, rs2_error ** error);

/**
* Test if the given device can be extended to the requested extension
* \param[in]  device     Realsense device
* \param[in]  extension  The extension to which the device should be tested if it is extendable
* \param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return non-zero value iff the device can be extended to the given extension
*/
int rs2_is_device_extendable_to(const rs2_device* device, rs2_extension extension_type, rs2_error ** error);

/**
* create a static snapshot of all connected sensors within specific device
* \param devuice     Specific RealSense device
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return            the list of devices, should be released by rs2_delete_device_list
*/
rs2_sensor_list* rs2_query_sensors(const rs2_device* device, rs2_error** error);

#ifdef __cplusplus
}
#endif
#endif
