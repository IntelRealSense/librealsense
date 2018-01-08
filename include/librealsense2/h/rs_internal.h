/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

/** \file rs2_internal.h
* \brief
* Exposes RealSense internal functionality for C compilers
*/


#ifndef LIBREALSENSE_RS2_INTERNAL_H
#define LIBREALSENSE_RS2_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif
#include "../rs.h"

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
 TODO
**/
rs2_device* rs2_create_bypass_device(rs2_error** error);
rs2_sensor* rs2_bypass_add_sensor(rs2_device* dev, const char* sensor_name, rs2_error** error);
void rs2_bypass_on_video_frame(rs2_device* dev, 
    int sensor, 
    void* pixels, 
    void(*deleter)(void*),
    int stride, int bpp,
    rs2_time_t ts, rs2_timestamp_domain domain,
    int frame_number,
    const rs2_stream_profile* profile, 
    rs2_error** error);
void rs2_bypass_add_video_stream(rs2_sensor* sensor, rs2_stream type, int index, int uid, int width, int height, int bpp, rs2_format fmt, rs2_error** error);

#ifdef __cplusplus
}
#endif
#endif
