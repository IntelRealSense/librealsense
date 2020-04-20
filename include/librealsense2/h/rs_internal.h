/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

/** \file rs_internal.h
* \brief
* Exposes RealSense internal functionality for C compilers
*/


#ifndef LIBREALSENSE_RS2_INTERNAL_H
#define LIBREALSENSE_RS2_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif
#include "rs_types.h"
#include "rs_context.h"
#include "rs_sensor.h"
#include "rs_frame.h"
#include "rs_option.h"
/**
 * librealsense Recorder is intended for effective unit-testing
 * Currently supports three modes of operation:
 */
typedef enum rs2_recording_mode
{
    RS2_RECORDING_MODE_BLANK_FRAMES, /* frame metadata will be recorded, but pixel data will be replaced with zeros to save space */
    RS2_RECORDING_MODE_COMPRESSED,   /* frames will be encoded using a proprietary lossy encoding, aiming at x5 compression at some CPU expense */
    RS2_RECORDING_MODE_BEST_QUALITY, /* frames will not be compressed, but rather stored as-is. This gives best quality and low CPU overhead, but you might run out of memory */
    RS2_RECORDING_MODE_COUNT
} rs2_recording_mode;

/** \brief All the parameters required to define a video stream. */
typedef struct rs2_video_stream
{
    rs2_stream type;
    int index;
    int uid;
    int width;
    int height;
    int fps;
    int bpp;
    rs2_format fmt;
    rs2_intrinsics intrinsics;
} rs2_video_stream;

/** \brief All the parameters required to define a motion stream. */
typedef struct rs2_motion_stream
{
    rs2_stream type;
    int index;
    int uid;
    int fps;
    rs2_format fmt;
    rs2_motion_device_intrinsic intrinsics;
} rs2_motion_stream;

/** \brief All the parameters required to define a pose stream. */
typedef struct rs2_pose_stream
{
    rs2_stream type;
    int index;
    int uid;
    int fps;
    rs2_format fmt;
} rs2_pose_stream;

/** \brief All the parameters required to define a video frame. */
typedef struct rs2_software_video_frame
{
    void* pixels;
    void(*deleter)(void*);
    int stride;
    int bpp;
    rs2_time_t timestamp;
    rs2_timestamp_domain domain;
    int frame_number;
    const rs2_stream_profile* profile;
} rs2_software_video_frame;

/** \brief All the parameters required to define a motion frame. */
typedef struct rs2_software_motion_frame
{
    void* data;
    void(*deleter)(void*);
    rs2_time_t timestamp;
    rs2_timestamp_domain domain;
    int frame_number;
    const rs2_stream_profile* profile;
} rs2_software_motion_frame;

/** \brief All the parameters required to define a pose frame. */
typedef struct rs2_software_pose_frame
{
    struct pose_frame_info
    {
        float translation[3];
        float velocity[3];
        float acceleration[3];
        float rotation[4];
        float angular_velocity[3];
        float angular_acceleration[3];
        int tracker_confidence;
        int mapper_confidence;
    };
    void* data;
    void(*deleter)(void*);
    rs2_time_t timestamp;
    rs2_timestamp_domain domain;
    int frame_number;
    const rs2_stream_profile* profile;
} rs2_software_pose_frame;

/** \brief All the parameters required to define a sensor notification. */
typedef struct rs2_software_notification
{
    rs2_notification_category category;
    int type;
    rs2_log_severity severity;
    const char* description;
    const char* serialized_data;
} rs2_software_notification;

struct rs2_software_device_destruction_callback;

/**
 * Create librealsense context that will try to record all operations over librealsense into a file
 * \param[in] api_version realsense API version as provided by RS2_API_VERSION macro
 * \param[in] filename string representing the name of the file to record
 * \param[in] section  string representing the name of the section within existing recording
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 * \return            context object, should be released by rs2_delete_context
 */
rs2_context* rs2_create_recording_context(int api_version, const char* filename, const char* section, rs2_recording_mode mode, rs2_error** error);

/**
 * Create librealsense context that given a file will respond to calls exactly as the recording did
 * if the user calls a method that was either not called during recording or violates causality of the recording error will be thrown
 * \param[in] api_version realsense API version as provided by RS2_API_VERSION macro
 * \param[in] filename string representing the name of the file to play back from
 * \param[in] section  string representing the name of the section within existing recording
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 * \return            context object, should be released by rs2_delete_context
 */
rs2_context* rs2_create_mock_context(int api_version, const char* filename, const char* section, rs2_error** error);

/**
* Create librealsense context that given a file will respond to calls exactly as the recording did
* if the user calls a method that was either not called during recording or violates causality of the recording error will be thrown
* \param[in] api_version realsense API version as provided by RS2_API_VERSION macro
* \param[in] filename string representing the name of the file to play back from
* \param[in] section  string representing the name of the section within existing recording
* \param[in] min_api_version reject any file that was recorded before this version
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return            context object, should be released by rs2_delete_context
*/
rs2_context* rs2_create_mock_context_versioned(int api_version, const char* filename, const char* section, const char* min_api_version, rs2_error** error);

/**
 * Create software device to enable use librealsense logic without getting data from backend
 * but inject the data from outside
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 * \return            software device object, should be released by rs2_delete_device
 */
rs2_device* rs2_create_software_device(rs2_error** error);

/**
 * Add sensor to the software device
 * \param[in] dev the software device
 * \param[in] sensor_name the name of the sensor
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 * \return            software sensor object, should be released by rs2_delete_sensor
 */
rs2_sensor* rs2_software_device_add_sensor(rs2_device* dev, const char* sensor_name, rs2_error** error);

/**
 * Inject video frame to software sonsor
 * \param[in] sensor the software sensor
 * \param[in] frame all the frame components
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 */
void rs2_software_sensor_on_video_frame(rs2_sensor* sensor, rs2_software_video_frame frame, rs2_error** error);

/**
* Inject motion frame to software sonsor
* \param[in] sensor the software sensor
* \param[in] frame all the frame components
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_software_sensor_on_motion_frame(rs2_sensor* sensor, rs2_software_motion_frame frame, rs2_error** error);

/**
* Inject pose frame to software sonsor
* \param[in] sensor the software sensor
* \param[in] frame all the frame components
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_software_sensor_on_pose_frame(rs2_sensor* sensor, rs2_software_pose_frame frame, rs2_error** error);


/**
* Inject notification to software sonsor
* \param[in] sensor the software sensor
* \param[in] notif all the notification components
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_software_sensor_on_notification(rs2_sensor* sensor, rs2_software_notification notif, rs2_error** error);

/**
* Set frame metadata for the upcoming frames
* \param[in] sensor the software sensor
* \param[in] value metadata key to set
* \param[in] type metadata value
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_software_sensor_set_metadata(rs2_sensor* sensor, rs2_frame_metadata_value value, rs2_metadata_type type, rs2_error** error);

/**
* set callback to be notified when a specific software device is destroyed
* \param[in] dev             software device
* \param[in] on_notification function pointer to register as callback
* \param[out] error          if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_software_device_set_destruction_callback(const rs2_device* dev, rs2_software_device_destruction_callback_ptr on_notification, void* user, rs2_error** error);

/**
* set callback to be notified when a specific software device is destroyed
* \param[in] dev      software device
* \param[in] callback callback object created from c++ application. ownership over the callback object is moved into the relevant device lock
* \param[out] error   if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_software_device_set_destruction_callback_cpp(const rs2_device* dev, rs2_software_device_destruction_callback* callback, rs2_error** error);

/**
 * Set the wanted matcher type that will be used by the syncer
 * \param[in] dev the software device
 * \param[in] matcher matcher type
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 */
void rs2_software_device_create_matcher(rs2_device* dev, rs2_matchers matcher, rs2_error** error);

/**
 * Register a camera info value for the software device
 * \param[in] dev the software device
 * \param[in] info identifier for the camera info to add.
 * \param[in] val string value for this new camera info.
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 */
void rs2_software_device_register_info(rs2_device* dev, rs2_camera_info info, const char *val, rs2_error** error);

/**
 * Update an existing camera info value for the software device
 * \param[in] dev the software device
 * \param[in] info identifier for the camera info to add.
 * \param[in] val string value for this new camera info.
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 */
void rs2_software_device_update_info(rs2_device* dev, rs2_camera_info info, const char * val, rs2_error** error);

/**
 * Add video stream to sensor
 * \param[in] sensor the software sensor
 * \param[in] video_stream all the stream components
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 */
rs2_stream_profile* rs2_software_sensor_add_video_stream(rs2_sensor* sensor, rs2_video_stream video_stream, rs2_error** error);

/**
 * Add video stream to sensor
 * \param[in] sensor the software sensor
 * \param[in] video_stream all the stream components
 * \param[in] is_default whether or not the stream should be a default stream for the device
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 */
rs2_stream_profile* rs2_software_sensor_add_video_stream_ex(rs2_sensor* sensor, rs2_video_stream video_stream, int is_default, rs2_error** error);

/**
* Add motion stream to sensor
* \param[in] sensor the software sensor
* \param[in] motion_stream all the stream components
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
rs2_stream_profile* rs2_software_sensor_add_motion_stream(rs2_sensor* sensor, rs2_motion_stream motion_stream, rs2_error** error);

/**
* Add motion stream to sensor
* \param[in] sensor the software sensor
* \param[in] motion_stream all the stream components
* \param[in] is_default whether or not the stream should be a default stream for the device
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
rs2_stream_profile* rs2_software_sensor_add_motion_stream_ex(rs2_sensor* sensor, rs2_motion_stream motion_stream, int is_default, rs2_error** error);

/**
* Add pose stream to sensor
* \param[in] sensor the software sensor
* \param[in] pose_stream all the stream components
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
rs2_stream_profile* rs2_software_sensor_add_pose_stream(rs2_sensor* sensor, rs2_pose_stream pose_stream, rs2_error** error);

/**
* Add pose stream to sensor
* \param[in] sensor the software sensor
* \param[in] pose_stream all the stream components
* \param[in] is_default whether or not the stream should be a default stream for the device
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
rs2_stream_profile* rs2_software_sensor_add_pose_stream_ex(rs2_sensor* sensor, rs2_pose_stream pose_stream, int is_default, rs2_error** error);

/**
 * Add read only option to sensor
 * \param[in] sensor the software sensor
 * \param[in] option the wanted option
 * \param[in] val the initial value
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 */
void rs2_software_sensor_add_read_only_option(rs2_sensor* sensor, rs2_option option, float val, rs2_error** error);

/**
 * Update the read only option added to sensor
 * \param[in] sensor the software sensor
 * \param[in] option the wanted option
 * \param[in] val the wanted value
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 */
void rs2_software_sensor_update_read_only_option(rs2_sensor* sensor, rs2_option option, float val, rs2_error** error);

/**
 * Add an option to sensor
 * \param[in] sensor        the software sensor
 * \param[in] option        the wanted option
 * \param[in] min           the minimum value which will be accepted for this option
 * \param[in] max           the maximum value which will be accepted for this option
 * \param[in] step          the granularity of options which accept discrete values, or zero if the option accepts continuous values
 * \param[in] def           the initial value of the option
 * \param[in] is_writable   should the option be read-only or not
 * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 */
void rs2_software_sensor_add_option(rs2_sensor* sensor, rs2_option option, float min, float max, float step, float def, int is_writable, rs2_error** error);

/**
* Sensors hold the parent device in scope via a shared_ptr. This function detaches that so that the software sensor doesn't keep the software device alive.
* Note that this is dangerous as it opens the door to accessing freed memory if care isn't taken.
* \param[in] sensor         the software sensor
* \param[out] error         if non-null, recieves any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_software_sensor_detach(rs2_sensor* sensor, rs2_error** error);

#ifdef __cplusplus
}
#endif
#endif
