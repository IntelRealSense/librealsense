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

rs2_pipeline* rs2_create_pipeline(rs2_context* ctx, rs2_error ** error);

void rs2_start_pipeline_with_callback(rs2_pipeline* pipe, rs2_frame_callback* on_frame, rs2_error ** error);

void rs2_start_pipeline(rs2_pipeline* pipe, rs2_error ** error);

rs2_frame* rs2_pipeline_wait_for_frames(rs2_pipeline* pipe, unsigned int timeout_ms, rs2_error ** error);

void rs2_delete_pipeline(rs2_pipeline* block);

#ifdef __cplusplus
}
#endif
#endif
