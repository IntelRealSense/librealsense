/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

/** \file rs2_processing.h
* \brief
* Exposes RealSense processing-block functionality for C compilers
*/


#ifndef LIBREALSENSE_RS2_PIPELINE_H
#define LIBREALSENSE_RS2_PIPELINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rs_types.h"

/**
* create pipeline
* \param[in]  ctx    context
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
rs2_pipeline* rs2_create_pipeline(rs2_context* ctx, rs2_error ** error);

/**
* stop streaming will not change the pipeline configuration
* \param[in] pipe  pipeline
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_pipeline_stop(rs2_pipeline* pipe, rs2_error ** error);

/**
* wait until new frame becomes available
* \param[in] pipe the pipeline
* \param[in] timeout_ms   Max time in milliseconds to wait until an exception will be thrown
* \return Set of coherent frames
*/
rs2_frame* rs2_pipeline_wait_for_frames(rs2_pipeline* pipe, unsigned int timeout_ms, rs2_error ** error);

/**
* poll if a new frame is available and dequeue if it is
* \param[in] pipe the pipeline
* \param[out] output_frame frame handle to be released using rs2_release_frame
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return true if new frame was stored to output_frame
*/
int rs2_pipeline_poll_for_frames(rs2_pipeline* pipe, rs2_frame** output_frame, rs2_error ** error);


/**
* delete pipeline
* \param[in] pipeline to delete
*/
void rs2_delete_pipeline(rs2_pipeline* pipe);



//TODO: Document all below:

//pipeline
rs2_pipeline_profile* rs2_pipeline_start(rs2_pipeline* pipe, rs2_config* config, rs2_error ** error);
rs2_pipeline_profile* rs2_pipeline_start_with_record(rs2_pipeline* pipe, rs2_config* config, const char* file, rs2_error ** error);
rs2_pipeline_profile* rs2_pipeline_get_active_profile(rs2_pipeline* pipe, rs2_error ** error);

//pipeline_profile
rs2_device* rs2_pipeline_profile_get_device(rs2_pipeline_profile* profile, rs2_error ** error);
rs2_stream_profile_list* rs2_pipeline_profile_get_active_streams(rs2_pipeline_profile* profile, rs2_error** error);
void rs2_delete_pipeline_profile(rs2_pipeline_profile* profile);

//config
rs2_config* rs2_create_config(rs2_error** error);
void rs2_delete_config(rs2_config* config);
void rs2_config_enable_stream(rs2_config* config,
                              rs2_stream stream,
                              int index,
                              int width,
                              int height,
                              rs2_format format,
                              int framerate,
                              rs2_error** error);
void rs2_config_enable_all_stream(rs2_config* config, rs2_error ** error);
void rs2_config_enable_device(rs2_config* config, const char* serial, rs2_error ** error);
void rs2_config_enable_device_from_file(rs2_config* config, const char* file, rs2_error ** error);
void rs2_config_disable_stream(rs2_config* config, rs2_stream stream, rs2_error ** error);
void rs2_config_disable_all_streams(rs2_config* config, rs2_error ** error);
rs2_pipeline_profile* rs2_config_resolve(rs2_config* config, rs2_pipeline* pipe, rs2_error ** error);
int rs2_config_can_resolve(rs2_config* config, rs2_pipeline* pipe, rs2_error ** error);

#ifdef __cplusplus
}
#endif
#endif
