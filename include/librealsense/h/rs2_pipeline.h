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

#include "rs2_types.h"

/**
* create pipeline
* \param[in]  ctx    context
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return            the requested sensor, should be released by rs2_delete_sensor
*/
rs2_pipeline* rs2_create_pipeline(rs2_context* ctx, rs2_error ** error);

/**
* start streaming with default configuration or commited configuration
* \param[in] pipe  pipeline
* \param[in] on_frame function pointer to register as per-frame callback
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_start_pipeline_with_callback(rs2_pipeline* pipe, rs2_frame_callback* on_frame, rs2_error ** error);

/**
* start streaming with default configuration or commited configuration
* \param[in] pipe  pipeline
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_start_pipeline(rs2_pipeline* pipe, rs2_error ** error);

/**
* committing a configuration to pipeline
* \param[in] stream    stream type
* \param[in] index     stream index
* \param[in] width     width
* \param[in] height    height
* \param[in] format    stream format
* \param[in] framerate    stream framerate
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_enable_stream_pipeline(rs2_pipeline* pipe, rs2_stream stream, int index, int width, int height, rs2_format format, int framerate, rs2_error ** error);

/**
*  remove a configuration from the pipeline
* \param[in] stream    stream type
*/
void rs2_disable_stream_pipeline(rs2_pipeline* pipe, rs2_stream stream, rs2_error ** error);

/**
*  remove all configurations from the pipeline
*/
void rs2_disable_all_streams_pipeline(rs2_pipeline* pipe, rs2_error ** error);

/**
* wait until new frame becomes available
* \param[in] timeout_ms   Max time in milliseconds to wait until an exception will be thrown
* \return Set of coherent frames
*/
rs2_frame* rs2_pipeline_wait_for_frames(rs2_pipeline* pipe, unsigned int timeout_ms, rs2_error ** error);

/**
* delete pipeline
* \param[in] pipeline to delete
*/
void rs2_delete_pipeline(rs2_pipeline* block);

#ifdef __cplusplus
}
#endif
#endif
