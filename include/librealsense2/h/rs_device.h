/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

/** \file rs_device.h
* \brief Exposes RealSense device functionality for C compilers
*/


#ifndef LIBREALSENSE_RS2_DEVICE_H
#define LIBREALSENSE_RS2_DEVICE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rs_types.h"
#include "rs_sensor.h"

/**
* Determines number of devices in a list.
* \param[in]  info_list The list of connected devices captured using rs2_query_devices
* \param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               Device count
*/
int rs2_get_device_count(const rs2_device_list* info_list, rs2_error** error);

/**
* Deletes device list, any devices created using this list will remain unaffected.
* \param[in]  info_list List to delete
*/
void rs2_delete_device_list(rs2_device_list* info_list);

/**
* Checks if a specific device is contained inside a device list.
* \param[in]  info_list The list of devices to check in
* \param[in]  device    RealSense device to check for
* \param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               True if the device is in the list and false otherwise
*/
int rs2_device_list_contains(const rs2_device_list* info_list, const rs2_device* device, rs2_error** error);

/**
* Creates a device by index. The device object represents a physical camera and provides the means to manipulate it.
* \param[in]  info_list the list containing the device to retrieve
* \param[in]  index     The zero based index of device to retrieve
* \param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               The requested device, should be released by rs2_delete_device
*/
rs2_device* rs2_create_device(const rs2_device_list* info_list, int index, rs2_error** error);

/**
* Delete RealSense device
* \param[in]  device    Realsense device to delete
*/
void rs2_delete_device(rs2_device* device);

/**
* Retrieve camera specific information, like versions of various internal components.
* \param[in]  device    The RealSense device
* \param[in]  info      Camera info type to retrieve
* \param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               The requested camera info string, in a format specific to the device model
*/
const char* rs2_get_device_info(const rs2_device* device, rs2_camera_info info, rs2_error** error);

/**
* Check if a camera supports a specific camera info type.
* \param[in]  device    The RealSense device to check
* \param[in]  info      The parameter to check for support
* \param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               True if the parameter both exist and well-defined for the specific device
*/
int rs2_supports_device_info(const rs2_device* device, rs2_camera_info info, rs2_error** error);

/**
 * Send hardware reset request to the device. The actual reset is asynchronous.
 * Note: Invalidates all handles to this device.
 * \param[in]  device   The RealSense device to reset
 * \param[out] error    If non-null, receives any error that occurs during this call, otherwise, errors are ignored
 */
void rs2_hardware_reset(const rs2_device * device, rs2_error ** error);

/**
* Send raw data to device
* \param[in]  device                    RealSense device to send data to
* \param[in]  raw_data_to_send          Raw data to be sent to device
* \param[in]  size_of_raw_data_to_send  Size of raw_data_to_send in bytes
* \param[out] error                     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return                               Device's response in a rs2_raw_data_buffer, which should be released by rs2_delete_raw_data
*/
const rs2_raw_data_buffer* rs2_send_and_receive_raw_data(rs2_device* device, void* raw_data_to_send, unsigned size_of_raw_data_to_send, rs2_error** error);

/**
* Test if the given device can be extended to the requested extension.
* \param[in]  device    Realsense device
* \param[in]  extension The extension to which the device should be tested if it is extendable
* \param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               Non-zero value iff the device can be extended to the given extension
*/
int rs2_is_device_extendable_to(const rs2_device* device, rs2_extension extension, rs2_error ** error);

/**
* Create a static snapshot of all connected sensors within a specific device.
* \param[in]  device    Specific RealSense device
* \param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               The list of sensors, should be released by rs2_delete_sensor_list
*/
rs2_sensor_list* rs2_query_sensors(const rs2_device* device, rs2_error** error);

/**
* Enter the given device into loopback operation mode that uses the given file as input for raw data
* \param[in]  device     Device to enter into loopback operation mode
* \param[in]  from_file  Path to bag file with raw data for loopback
* \param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_loopback_enable(const rs2_device* device, const char* from_file, rs2_error** error);

/**
* Restores the given device into normal operation mode
* \param[in]  device     Device to restore to normal operation mode
* \param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_loopback_disable(const rs2_device* device, rs2_error** error);

/**
* Checks if the device is in loopback mode or not
* \param[in]  device     Device to check for operation mode
* \param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return true if the device is in loopback operation mode
*/
int rs2_loopback_is_enabled(const rs2_device* device, rs2_error** error);

/**
* Connects to a given tm2 controller
* \param[in]  device     Device to connect to the controller
* \param[in]  mac_addr   The MAC address of the desired controller
* \param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_connect_tm2_controller(const rs2_device* device, const unsigned char* mac_addr, rs2_error** error);

/**
* Disconnects a given tm2 controller
* \param[in]  device     Device to disconnect the controller from
* \param[in]  id         The ID of the desired controller
* \param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_disconnect_tm2_controller(const rs2_device* device, int id, rs2_error** error);


/** 
* Reset device to factory calibration
* \param[in] device       The RealSense device
* \param[out] error       If non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_reset_to_factory_calibration(const rs2_device* device, rs2_error** e);

/**
* Write calibration to device's EEPROM
* \param[in] device       The RealSense device
* \param[out] error       If non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_write_calibration(const rs2_device* device, rs2_error** e);

/**
* Update device to the provided firmware, the device must be extendable to RS2_EXTENSION_UPDATABLE.
* This call is executed on the caller's thread and it supports progress notifications via the optional callback.
* \param[in]  device        Device to update
* \param[in]  fw_image      Firmware image buffer
* \param[in]  fw_image_size Firmware image buffer size
* \param[in]  callback      Optional callback for update progress notifications, the progress value is normailzed to 1
* \param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_update_firmware_cpp(const rs2_device* device, const void* fw_image, int fw_image_size, rs2_update_progress_callback* callback, rs2_error** error);

/**
* Update device to the provided firmware, the device must be extendable to RS2_EXTENSION_UPDATABLE.
* This call is executed on the caller's thread and it supports progress notifications via the optional callback.
* \param[in]  device        Device to update
* \param[in]  fw_image      Firmware image buffer
* \param[in]  fw_image_size Firmware image buffer size
* \param[in]  callback      Optional callback for update progress notifications, the progress value is normailzed to 1
* \param[in]  client_data   Optional client data for the callback
* \param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_update_firmware(const rs2_device* device, const void* fw_image, int fw_image_size, rs2_update_progress_callback_ptr callback, void* client_data, rs2_error** error);

/**
* Create backup of camera flash memory. Such backup does not constitute valid firmware image, and cannot be
* loaded back to the device, but it does contain all calibration and device information.
* \param[in]  device        Device to update
* \param[in]  callback      Optional callback for update progress notifications, the progress value is normailzed to 1
* \param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
const rs2_raw_data_buffer* rs2_create_flash_backup_cpp(const rs2_device* device, rs2_update_progress_callback* callback, rs2_error** error);

/**
* Create backup of camera flash memory. Such backup does not constitute valid firmware image, and cannot be
* loaded back to the device, but it does contain all calibration and device information.
* \param[in]  device        Device to update
* \param[in]  callback      Optional callback for update progress notifications, the progress value is normailzed to 1
* \param[in]  client_data   Optional client data for the callback
* \param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
const rs2_raw_data_buffer* rs2_create_flash_backup(const rs2_device* device, rs2_update_progress_callback_ptr callback, void* client_data, rs2_error** error);

#define RS2_UNSIGNED_UPDATE_MODE_UPDATE     0
#define RS2_UNSIGNED_UPDATE_MODE_READ_ONLY  1
#define RS2_UNSIGNED_UPDATE_MODE_FULL       2

/**
* Update device to the provided firmware by writing raw data directly to the flash, this command can be executed only on unlocked camera.
* The device must be extendable to RS2_EXTENSION_UPDATABLE.
* This call is executed on the caller's thread and it supports progress notifications via the optional callback.
* \param[in]  device        Device to update
* \param[in]  fw_image      Firmware image buffer
* \param[in]  fw_image_size Firmware image buffer size
* \param[in]  callback      Optional callback for update progress notifications, the progress value is normailzed to 1
* \param[in]  update_mode   Select one of RS2_UNSIGNED_UPDATE_MODE, WARNING!!! setting to any option other than RS2_UNSIGNED_UPDATE_MODE_UPDATE will make this call unsafe and might damage the camera
* \param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_update_firmware_unsigned_cpp(const rs2_device* device, const void* fw_image, int fw_image_size, rs2_update_progress_callback* callback, int update_mode, rs2_error** error);

/**
* Update device to the provided firmware by writing raw data directly to the flash, this command can be executed only on unlocked camera.
* The device must be extendable to RS2_EXTENSION_UPDATABLE.
* This call is executed on the caller's thread and it supports progress notifications via the optional callback.
* \param[in]  device        Device to update
* \param[in]  fw_image      Firmware image buffer
* \param[in]  fw_image_size Firmware image buffer size
* \param[in]  callback      Optional callback for update progress notifications, the progress value is normailzed to 1
* \param[in]  client_data   Optional client data for the callback
* \param[in]  update_mode   Select one of RS2_UNSIGNED_UPDATE_MODE, WARNING!!! setting to any option other than RS2_UNSIGNED_UPDATE_MODE_UPDATE will make this call unsafe and might damage the camera
* \param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_update_firmware_unsigned(const rs2_device* device, const void* fw_image, int fw_image_size, rs2_update_progress_callback_ptr callback, void* client_data, int update_mode, rs2_error** error);

/**
* Enter the device to update state, this will cause the updatable device to disconnect and reconnect as update device.
* \param[in]  device     Device to update
* \param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_enter_update_state(const rs2_device* device, rs2_error** error);

/**
* This will improve the depth noise.
* \param[in] json_content       Json string to configure speed on chip calibration parameters:
                                    {
                                      "speed": 3,
                                      "scan parameter": 0,
                                      "data sampling": 0
                                    }
                                    speed - value can be one of: Very fast = 0, Fast = 1, Medium = 2, Slow = 3, White wall = 4, default is  Slow
                                    scan_parameter - value can be one of: Py scan (default) = 0, Rx scan = 1
                                    data_sampling - value can be one of:polling data sampling = 0, interrupt data sampling = 1
                                    if json is nullptr it will be ignored and calibration will use the default parameters
* \param[out] health            Calibration Health-Check captures how far camera calibration is from the optimal one
                                [0, 0.25) - Good
                                [0.25, 0.75) - Can be Improved
                                [0.75, ) - Requires Calibration
* \param[in] callback           Optional callback to get progress notifications
* \param[in] timeout_ms         Timeout in ms (use 5000 msec unless instructed otherwise)
* \return                       New calibration table
*/
const rs2_raw_data_buffer* rs2_run_on_chip_calibration_cpp(rs2_device* device, const void* json_content, int content_size, float* health, rs2_update_progress_callback* progress_callback, int timeout_ms, rs2_error** error);

/**
* This will improve the depth noise.
* \param[in] json_content       Json string to configure speed on chip calibration parameters:
                                    {
                                      "speed": 3,
                                      "scan parameter": 0,
                                      "data sampling": 0
                                    }
                                    speed - value can be one of: Very fast = 0, Fast = 1, Medium = 2, Slow = 3, White wall = 4, default is  Slow
                                    scan_parameter - value can be one of: Py scan (default) = 0, Rx scan = 1
                                    data_sampling - value can be one of:polling data sampling = 0, interrupt data sampling = 1
                                    if json is nullptr it will be ignored and calibration will use the default parameters
* \param[out] health            Calibration Health-Check captures how far camera calibration is from the optimal one
                                [0, 0.25) - Good
                                [0.25, 0.75) - Can be Improved
                                [0.75, ) - Requires Calibration
* \param[in]  callback          Optional callback for update progress notifications, the progress value is normailzed to 1
* \param[in]  client_data       Optional client data for the callback
* \param[in] timeout_ms         Timeout in ms (use 5000 msec unless instructed otherwise)
* \return                       New calibration table
*/
const rs2_raw_data_buffer* rs2_run_on_chip_calibration(rs2_device* device, const void* json_content, int content_size, float* health, rs2_update_progress_callback_ptr callback, void* client_data, int timeout_ms, rs2_error** error);

/**
* This will adjust camera absolute distance to flat target. User needs to enter the known ground truth.
* \param[in] ground_truth_mm     Ground truth in mm must be between 2500 - 2000000
* \param[in] json_content        Json string to configure tare calibration parameters:
                                    {
                                      "average step count": 20,
                                      "step count": 20,
                                      "accuracy": 2,
                                      "scan parameter": 0,
                                      "data sampling": 0
                                    }
                                    average step count - number of frames to average, must be between 1 - 30, default = 20
                                    step count - max iteration steps, must be between 5 - 30, default = 10
                                    accuracy - Subpixel accuracy level, value can be one of: Very high = 0 (0.025%), High = 1 (0.05%), Medium = 2 (0.1%), Low = 3 (0.2%), Default = Very high (0.025%), default is very high (0.025%)
                                    scan_parameter - value can be one of: Py scan (default) = 0, Rx scan = 1
                                    data_sampling - value can be one of:polling data sampling = 0, interrupt data sampling = 1
                                    if json is nullptr it will be ignored and calibration will use the default parameters
* \param[in]  content_size        Json string size if its 0 the json will be ignored and calibration will use the default parameters
* \param[in]  callback            Optional callback to get progress notifications
* \param[in] timeout_ms          Timeout in ms (use 5000 msec unless instructed otherwise)
* \return                         New calibration table
*/
const rs2_raw_data_buffer* rs2_run_tare_calibration_cpp(rs2_device* dev, float ground_truth_mm, const void* json_content, int content_size, rs2_update_progress_callback* progress_callback, int timeout_ms, rs2_error** error);

/**
* This will adjust camera absolute distance to flat target. User needs to enter the known ground truth.
* \param[in] ground_truth_mm     Ground truth in mm must be between 2500 - 2000000
* \param[in] json_content        Json string to configure tare calibration parameters:
                                    {
                                      "average_step_count": 20,
                                      "step count": 20,
                                      "accuracy": 2,
                                      "scan parameter": 0,
                                      "data sampling": 0
                                    }
                                    average step count - number of frames to average, must be between 1 - 30, default = 20
                                    step count - max iteration steps, must be between 5 - 30, default = 10
                                    accuracy - Subpixel accuracy level, value can be one of: Very high = 0 (0.025%), High = 1 (0.05%), Medium = 2 (0.1%), Low = 3 (0.2%), Default = Very high (0.025%), default is very high (0.025%)
                                    scan_parameter - value can be one of: Py scan (default) = 0, Rx scan = 1
                                    data_sampling - value can be one of:polling data sampling = 0, interrupt data sampling = 1
                                    if json is nullptr it will be ignored and calibration will use the default parameters
* \param[in]  content_size       Json string size if its 0 the json will be ignored and calibration will use the default parameters
* \param[in]  callback           Optional callback for update progress notifications, the progress value is normailzed to 1
* \param[in]  client_data        Optional client data for the callback
* \param[in] timeout_ms          Timeout in ms (use 5000 msec unless instructed otherwise)
* \return                        New calibration table
*/
const rs2_raw_data_buffer* rs2_run_tare_calibration(rs2_device* dev, float ground_truth_mm, const void* json_content, int content_size, rs2_update_progress_callback_ptr callback, void* client_data, int timeout_ms, rs2_error** error);

/**
*  Read current calibration table from flash.
* \return    Calibration table
*/
const rs2_raw_data_buffer* rs2_get_calibration_table(const rs2_device* dev, rs2_error** error);

/**
*  Set current table to dynamic area.
* \param[in]     Calibration table
*/
void rs2_set_calibration_table(const rs2_device* device, const void* calibration, int calibration_size, rs2_error** error);

/* Serialize JSON content, returns ASCII-serialized JSON string on success. otherwise nullptr */
rs2_raw_data_buffer* rs2_serialize_json(rs2_device* dev, rs2_error** error);

/* Load JSON and apply advanced-mode controls */
void rs2_load_json(rs2_device* dev, const void* json_content, unsigned content_size, rs2_error** error);

#ifdef __cplusplus
}
#endif
#endif
