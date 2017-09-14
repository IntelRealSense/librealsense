/* License: Apache 2.0. See LICENSE file in root directory.
   Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

/** \file rs2_frame.h
* \brief
* Exposes RealSense frame functionality for C compilers
*/


#ifndef LIBREALSENSE_RS2_FRAME_H
#define LIBREALSENSE_RS2_FRAME_H

#ifdef __cplusplus
extern "C" {
#endif
#include "rs_types.h"

/** \brief Specifies the clock in relation to which the frame timestamp was measured. */
typedef enum rs2_timestamp_domain
{
    RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, /**< Frame timestamp was measured in relation to the camera clock */
    RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME,    /**< Frame timestamp was measured in relation to the OS system clock */
    RS2_TIMESTAMP_DOMAIN_COUNT           /**< Number of enumeration values. Not a valid input: intended to be used in for-loops. */
} rs2_timestamp_domain;
const char* rs2_timestamp_domain_to_string(rs2_timestamp_domain info);

/** \brief Per-Frame-Metadata are set of read-only properties that might be exposed for each individual frame */
typedef enum rs2_frame_metadata_value
{
    RS2_FRAME_METADATA_FRAME_COUNTER        , /**< A sequential index managed per-stream. Integer value*/
    RS2_FRAME_METADATA_FRAME_TIMESTAMP      , /**< Timestamp set by device clock when data readout and transmit commence. usec*/
    RS2_FRAME_METADATA_SENSOR_TIMESTAMP     , /**< Timestamp of the middle of sensor's exposure calculated by device. usec*/
    RS2_FRAME_METADATA_ACTUAL_EXPOSURE      , /**< Sensor's exposure width. When Auto Exposure (AE) is on the value is controlled by firmware. usec*/
    RS2_FRAME_METADATA_GAIN_LEVEL           , /**< A relative value increasing which will increase the Sensor's gain factor. \
                                              When AE is set On, the value is controlled by firmware. Integer value*/
    RS2_FRAME_METADATA_AUTO_EXPOSURE        , /**< Auto Exposure Mode indicator. Zero corresponds to AE switched off. */
    RS2_FRAME_METADATA_WHITE_BALANCE        , /**< White Balance setting as a color temperature. Kelvin degrees*/
    RS2_FRAME_METADATA_TIME_OF_ARRIVAL      , /**< Time of arrival in system clock */
    RS2_FRAME_METADATA_COUNT
} rs2_frame_metadata_value;
const char* rs2_frame_metadata_to_string(rs2_frame_metadata_value metadata);

/** \brief 3D coordinates with origin at topmost left corner of the lense,
     with positive Z pointing away from the camera, positive X pointing camera right and positive Y pointing camera down */
typedef struct rs2_vertex
{
    float xyz[3];
} rs2_vertex;

/** \brief Pixel location within 2D image. (0,0) is the topmost, left corner. Positive X is right, positive Y is down */
typedef struct rs2_pixel
{
    int ij[2];
} rs2_pixel;


/**
* retrieve metadata from frame handle
* \param[in] frame      handle returned from a callback
* \param[in] frame_metadata  the rs2_frame_metadata whose latest frame we are interested in
* \return            the metadata value
*/
rs2_metadata_type rs2_get_frame_metadata(const rs2_frame* frame, rs2_frame_metadata_value frame_metadata, rs2_error** error);

/**
* determine device metadata
* \param[in] frame      handle returned from a callback
* \param[in] metadata    the metadata to check for support
* \return                true if device has this metadata
*/
int rs2_supports_frame_metadata(const rs2_frame* frame, rs2_frame_metadata_value frame_metadata, rs2_error** error);

/**
* retrieve timestamp domain from frame handle. timestamps can only be comparable if they are in common domain
* (for example, depth timestamp might come from system time while color timestamp might come from the device)
* this method is used to check if two timestamp values are comparable (generated from the same clock)
* \param[in] frame      handle returned from a callback
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               the timestamp domain of the frame (camera / microcontroller / system time)
*/
rs2_timestamp_domain rs2_get_frame_timestamp_domain(const rs2_frame* frameset, rs2_error** error);

/**
* retrieve timestamp from frame handle in milliseconds
* \param[in] frame      handle returned from a callback
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               the timestamp of the frame in milliseconds
*/
rs2_time_t rs2_get_frame_timestamp(const rs2_frame* frame, rs2_error** error);

/**
* retrieve frame number from frame handle
* \param[in] frame      handle returned from a callback
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               the frame nubmer of the frame, in milliseconds since the device was started
*/
unsigned long long rs2_get_frame_number(const rs2_frame* frame, rs2_error** error);

/**
* retrieve data from frame handle
* \param[in] frame      handle returned from a callback
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               the pointer to the start of the frame data
*/
const void* rs2_get_frame_data(const rs2_frame* frame, rs2_error** error);

/**
* retrieve frame width in pixels
* \param[in] frame      handle returned from a callback
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               frame width in pixels
*/
int rs2_get_frame_width(const rs2_frame* frame, rs2_error** error);

/**
* retrieve frame height in pixels
* \param[in] frame      handle returned from a callback
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               frame height in pixels
*/
int rs2_get_frame_height(const rs2_frame* frame, rs2_error** error);

/**
* retrieve frame stride in bytes (number of bytes from start of line N to start of line N+1)
* \param[in] frame      handle returned from a callback
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               stride in bytes
*/
int rs2_get_frame_stride_in_bytes(const rs2_frame* frame, rs2_error** error);

/**
* retrieve bits per pixels in the frame image
* (note that bits per pixel is not necessarily divided by 8, as in 12bpp)
* \param[in] frame      handle returned from a callback
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               bits per pixel
*/
int rs2_get_frame_bits_per_pixel(const rs2_frame* frame, rs2_error** error);

/**
* create additional reference to a frame without duplicating frame data
* \param[in] frame      handle returned from a callback
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               new frame reference, has to be released by rs2_release_frame
*/
void rs2_frame_add_ref(rs2_frame* frame, rs2_error ** error);

/**
* relases the frame handle
* \param[in] frame handle returned from a callback
* \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
*/
void rs2_release_frame(rs2_frame* frame);

/**
* When called on Points frame type, this method returns a pointer to an array of 3D vertices of the model
* The coordinate system is: X right, Y up, Z away from the camera. Units: Meters
* \param[in] frame       Points frame
* \param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return                Pointer to an array of vertices, lifetime is managed by the frame
*/
rs2_vertex* rs2_get_frame_vertices(const rs2_frame* frame, rs2_error** error);

/**
* When called on Points frame type, this method returns a pointer to an array of texture coordinates per vertex
* Each coordinate represent a (u,v) pair within [0,1] range, to be mapped to texture image
* \param[in] frame       Points frame
* \param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return                Pointer to an array of texture coordinates, lifetime is managed by the frame
*/
rs2_pixel* rs2_get_frame_texture_coordinates(const rs2_frame* frame, rs2_error** error);

/**
* When called on Points frame type, this method returns the number of vertices in the frame
* \param[in] frame       Points frame
* \param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return                Number of vertices
*/
int rs2_get_frame_points_count(const rs2_frame* frame, rs2_error** error);

/**
* Returns the stream profile that was used to start the stream of this frame
* \param[in] frame       frame reference, owned by the user
* \param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return                Pointer to the stream profile object, lifetime is managed elsewhere
*/
const rs2_stream_profile* rs2_get_frame_stream_profile(const rs2_frame* frame, rs2_error** error);

/**
* Test if the given frame can be extended to the requested extension
* \param[in]  frame      Realsense frame
* \param[in]  extension  The extension to which the frame should be tested if it is extendable
* \param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return non-zero value iff the frame can be extended to the given extension
*/
int rs2_is_frame_extendable_to(const rs2_frame* frame, rs2_extension extension_type, rs2_error ** error);

/**
* Allocate new video frame using a frame-source provided form a processing block
* \param[in] source      Frame pool to allocate the frame from
* \param[in] new_stream  New stream profile to assign to newly created frame
* \param[in] original    A reference frame that can be used to fill in auxilary information like format, width, height, bpp, stride (if applicable)
* \param[in] new_bpp     New value for bits per pixel for the allocated frame
* \param[in] new_width   New value for width for the allocated frame
* \param[in] new_height  New value for height for the allocated frame
* \param[in] new stride  New value for stride in bytes for the allocated frame
* \param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return                reference to a newly allocated frame, must be released with release_frame
*                        memory for the frame is likely to be re-used from previous frame, but in lack of available frames in the pool will be allocated from the free store
*/
rs2_frame* rs2_allocate_synthetic_video_frame(rs2_source* source, const rs2_stream_profile* new_stream, rs2_frame* original,
    int new_bpp, int new_width, int new_height, int new_stride, rs2_extension frame_type, rs2_error** error);

/**
* Allocate new composite frame, aggregating a set of existing frames
* \param[in] source      Frame pool to allocate the frame from
* \param[in] frames      Array of existing frames
* \param[in] count       Number of input frames
* \param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return                reference to a newly allocated frame, must be released with release_frame
*                        when composite frame gets released it will automatically release all of the input frames
*/
rs2_frame* rs2_allocate_composite_frame(rs2_source* source, rs2_frame** frames, int count, rs2_error** error);

/**
* Extract frame from within a composite frame
* \param[in] composite   Composite frame
* \param[in] index       Index of the frame to extract within the composite frame
* \param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return                returns reference to a frame existing within the composite frame
*                        If you wish to keep this frame after the composite is released, you need to call aquire_ref
*                        Otherwise the resulting frame lifetime is bound by owning composite frame
*/
rs2_frame* rs2_extract_frame(rs2_frame* composite, int index, rs2_error** error);

/**
* Get number of frames embedded within a composite frame
* \param[in] composite   Composite input frame
* \param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return                Number of embedded frames
*/
int rs2_embedded_frames_count(rs2_frame* composite, rs2_error** error);

/**
* This method will dispatch frame callback on a frame
* \param[in] source      Frame pool provided by the processing block
* \param[in] frame       Frame to dispatch, frame ownership is passed to this function, so you don't have to call release_frame after it
*/
void rs2_synthetic_frame_ready(rs2_source* source, rs2_frame* frame, rs2_error** error);

#ifdef __cplusplus
}
#endif
#endif
