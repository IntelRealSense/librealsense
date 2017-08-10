/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

/** \file rs2_processing.h
* \brief
* Exposes RealSense processing-block functionality for C compilers
*/


#ifndef LIBREALSENSE_RS2_PROCESSING_H
#define LIBREALSENSE_RS2_PROCESSING_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rs2_types.h"

rs2_processing_block* rs2_create_processing_block(rs2_context* ctx, rs2_frame_processor_callback* proc, rs2_error** error);

rs2_processing_block* rs2_create_sync_processing_block(rs2_error** error);

void rs2_start_processing(rs2_processing_block* block, rs2_frame_callback* on_frame, rs2_error** error);

void rs2_process_frame(rs2_processing_block* block, rs2_frame* frame, rs2_error** error);

void rs2_delete_processing_block(rs2_processing_block* block);

rs2_processing_block* rs2_create_pointcloud(rs2_context* ctx, rs2_error** error);

/**
* create frame queue. frame queues are the simplest x-platform synchronization primitive provided by librealsense
* to help developers who are not using async APIs
* \param[in] capacity max number of frames to allow to be stored in the queue before older frames will start to get dropped
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return handle to the frame queue, must be released using rs2_delete_frame_queue
*/
rs2_frame_queue* rs2_create_frame_queue(int capacity, rs2_error** error);

/**
* deletes frame queue and releases all frames inside it
* \param[in] frame queue to delete
*/
void rs2_delete_frame_queue(rs2_frame_queue* queue);

/**
* wait until new frame becomes available in the queue and dequeue it
* \param[in] queue the frame queue data structure
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return frame handle to be released using rs2_release_frame
*/
rs2_frame* rs2_wait_for_frame(rs2_frame_queue* queue, rs2_error** error);

/**
* poll if a new frame is available and dequeue if it is
* \param[in] queue the frame queue data structure
* \param[out] output_frame frame handle to be released using rs2_release_frame
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return true if new frame was stored to output_frame
*/
int rs2_poll_for_frame(rs2_frame_queue* queue, rs2_frame** output_frame, rs2_error** error);

/**
* enqueue new frame into a queue
* \param[in] frame frame handle to enqueue (this operation passed ownership to the queue)
* \param[in] queue the frame queue data structure
*/
void rs2_enqueue_frame(const rs2_frame* frame, void* queue);

#ifdef __cplusplus
}
#endif
#endif
