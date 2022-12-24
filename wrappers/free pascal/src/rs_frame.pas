{
Exposes RealSense frame functionality
}

unit rs_frame;

{$mode ObjFPC}{$H+}

interface

uses
  rs_types;

{
Specifies the clock in relation to which the frame timestamp was measured. */

    RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, /**< Frame timestamp was measured in relation to the camera clock */
    RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME,    /**< Frame timestamp was measured in relation to the OS system clock */
    RS2_TIMESTAMP_DOMAIN_GLOBAL_TIME,    /**< Frame timestamp was measured in relation to the camera clock and converted to OS system clock by constantly measure the difference*/
    RS2_TIMESTAMP_DOMAIN_COUNT           /**< Number of enumeration values. Not a valid input: intended to be used in for-loops. */
}

type
  Trs2_timestamp_domain = (
    RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK,
    //< Frame timestamp was measured in relation to the camera clock
    RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME,
    //< Frame timestamp was measured in relation to the OS system clock
    RS2_TIMESTAMP_DOMAIN_GLOBAL_TIME,
    //< Frame timestamp was measured in relation to the camera clock and converted to OS system clock by constantly measure the difference
    RS2_TIMESTAMP_DOMAIN_COUNT
    //< Number of enumeration values. Not a valid input: intended to be used in for-loops.
    );

function rs2_timestamp_domain_to_string(info: Trs2_timestamp_domain): PChar;
  cdecl; external REALSENSE_DLL;
  {
Per-Frame-Metadata is the set of read-only properties that might be exposed for each individual frame. */
  }

type
  Trs2_frame_metadata_value = (
    RS2_FRAME_METADATA_FRAME_COUNTER,
    //< A sequential index managed per-stream. Integer value
    RS2_FRAME_METADATA_FRAME_TIMESTAMP,
    //< Timestamp set by device clock when data readout and transmit commence. usec
    RS2_FRAME_METADATA_SENSOR_TIMESTAMP,
    //< Timestamp of the middle of sensor's exposure calculated by device. usec
    RS2_FRAME_METADATA_ACTUAL_EXPOSURE,
    //< Sensor's exposure width. When Auto Exposure (AE) is on the value is controlled by firmware. usec
    RS2_FRAME_METADATA_GAIN_LEVEL,
    //< A relative value increasing which will increase the Sensor's gain fctor. When AE is set On, the value is controlled by firmware. Integer value
    RS2_FRAME_METADATA_AUTO_EXPOSURE,
    //< Auto Exposure Mode indicator. Zero corresponds to AE switched off.
    RS2_FRAME_METADATA_WHITE_BALANCE,
    //< White Balance setting as a color temperature. Kelvin degrees
    RS2_FRAME_METADATA_TIME_OF_ARRIVAL,
    //< Time of arrival in system clock
    RS2_FRAME_METADATA_TEMPERATURE,
    //< Temperature of the device, measured at the time of the frame capture. Celsius degrees
    RS2_FRAME_METADATA_BACKEND_TIMESTAMP,
    //< Timestamp get from uvc driver. usec
    RS2_FRAME_METADATA_ACTUAL_FPS, //< Actual fps
    RS2_FRAME_METADATA_FRAME_LASER_POWER,
    //< Laser power value 0-360.
    RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE,
    //< Laser power mode. Zero corresponds to Laser power switched off and one for switched on. deprecated, replaced by RS2_FRAME_METADATA_FRAME_EMITTER_MODE
    RS2_FRAME_METADATA_EXPOSURE_PRIORITY, //< Exposure priority.
    RS2_FRAME_METADATA_EXPOSURE_ROI_LEFT,
    //< Left region of interest for the auto exposure Algorithm.
    RS2_FRAME_METADATA_EXPOSURE_ROI_RIGHT,
    //< Right region of interest for the auto exposure Algorithm.
    RS2_FRAME_METADATA_EXPOSURE_ROI_TOP,
    //< Top region of interest for the auto exposure Algorithm.
    RS2_FRAME_METADATA_EXPOSURE_ROI_BOTTOM,
    //< Bottom region of interest for the auto exposure Algorithm.
    RS2_FRAME_METADATA_BRIGHTNESS, //< Color image brightness.
    RS2_FRAME_METADATA_CONTRAST, //< Color image contrast.
    RS2_FRAME_METADATA_SATURATION, //< Color image saturation.
    RS2_FRAME_METADATA_SHARPNESS, //< Color image sharpness.
    RS2_FRAME_METADATA_AUTO_WHITE_BALANCE_TEMPERATURE,
    //< Auto white balance temperature Mode indicator. Zero corresponds to automatic mode switched off.
    RS2_FRAME_METADATA_BACKLIGHT_COMPENSATION,
    //< Color backlight compensation. Zero corresponds to switched off.
    RS2_FRAME_METADATA_HUE, //< Color image hue.
    RS2_FRAME_METADATA_GAMMA, //< Color image gamma.
    RS2_FRAME_METADATA_MANUAL_WHITE_BALANCE,
    //< Color image white balance.
    RS2_FRAME_METADATA_POWER_LINE_FREQUENCY,
    //< Power Line Frequency for anti-flickering Off/50Hz/60Hz/Auto.
    RS2_FRAME_METADATA_LOW_LIGHT_COMPENSATION,
    //< Color lowlight compensation. Zero corresponds to switched off.
    RS2_FRAME_METADATA_FRAME_EMITTER_MODE,
    //< Emitter mode: 0 - all emitters disabled. 1 - laser enabled. 2 - auto laser enabled (opt). 3 - LED enabled (opt).
    RS2_FRAME_METADATA_FRAME_LED_POWER, //< Led power value 0-360.
    RS2_FRAME_METADATA_RAW_FRAME_SIZE,
    //< The number of transmitted payload bytes, not including metadata */
    RS2_FRAME_METADATA_GPIO_INPUT_DATA, //< GPIO input data
    RS2_FRAME_METADATA_SEQUENCE_NAME, //< sub-preset id
    RS2_FRAME_METADATA_SEQUENCE_ID, //< sub-preset sequence id
    RS2_FRAME_METADATA_SEQUENCE_SIZE, //< sub-preset sequence size
    RS2_FRAME_METADATA_COUNT
    );
 {
 Calibration target type.
 }

type
  Trs2_calib_target_type = (

    RS2_CALIB_TARGET_RECT_GAUSSIAN_DOT_VERTICES,
    //< Flat rectangle with vertices as the centers of Gaussian dots
    RS2_CALIB_TARGET_ROI_RECT_GAUSSIAN_DOT_VERTICES,
    //< Flat rectangle with vertices as the centers of Gaussian dots with target inside the ROI
    RS2_CALIB_TARGET_POS_GAUSSIAN_DOT_VERTICES,
    //< Positions of vertices as the centers of Gaussian dots with target inside the ROI
    RS2_CALIB_TARGET_COUNT
    //< Number of enumeration values. Not a valid input: intended to be used in for-loops.
    );

function rs2_calib_target_type_to_string(atype: Trs2_calib_target_type): PChar;
  cdecl; external REALSENSE_DLL;

function rs2_frame_metadata_to_string(metadata: Trs2_frame_metadata_value): PChar;
  cdecl; external REALSENSE_DLL;
function rs2_frame_metadata_value_to_string(metadata: Trs2_frame_metadata_value): PChar;
  external REALSENSE_DLL;




{

 Extract frame from within a composite frame
 param[in] composite   Composite frame
 param[in] index       Index of the frame to extract within the composite frame
 param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored
 return                returns reference to a frame existing within the composite frame
                        If you wish to keep this frame after the composite is released, you need to call acquire_ref
                        Otherwise the resulting frame lifetime is bound by owning composite frame

}

function rs2_extract_frame(composite: pRS2_frame; index: integer;
  error: pRS2_error): pRS2_frame; cdecl; external REALSENSE_DLL;
{

Get number of frames embedded within a composite frame
param[in] composite   Composite input frame
param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored
return                Number of embedded frames

}

function rs2_embedded_frames_count(frame: pRS2_frame; error: pRS2_error): integer;
  cdecl; external REALSENSE_DLL;
{

 relases the frame handle
 param[in] frame handle returned from a callback

}

procedure rs2_release_frame(frame: pRS2_frame); cdecl; external REALSENSE_DLL;

{
 retrieve data size from frame handle
 param[in] frame      handle returned from a callback
 param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 return               the size of the frame data

}

function rs2_get_frame_data_size(const frame: pRS2_frame; error: pRS2_error): integer;
  cdecl; external REALSENSE_DLL;
 {

  retrieve data from frame handle
  param[in] frame      handle returned from a callback
  param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return               the pointer to the start of the frame data

 }

function rs2_get_frame_data(const frame: pRS2_frame; error: pRS2_error): pVoid; cdecl;
  external REALSENSE_DLL;

{

retrieve frame number from frame handle
param[in] frame      handle returned from a callback
param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
return               the frame nubmer of the frame, in milliseconds since the device was started

}

function rs2_get_frame_number(const frame: pRS2_frame; error: pRS2_error): uint64;
  cdecl; external REALSENSE_DLL;

{
 retrieve timestamp domain from frame handle. timestamps can only be comparable if they are in common domain
 (for example, depth timestamp might come from system time while color timestamp might come from the device)
 this method is used to check if two timestamp values are comparable (generated from the same clock)
 param[in] frameset   handle returned from a callback
 param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 return               the timestamp domain of the frame (camera / microcontroller / system time)

}
function rs2_get_frame_timestamp_domain(const frame: pRS2_frame;
  error: pRS2_error): Trs2_timestamp_domain; cdecl; external REALSENSE_DLL;
{
 retrieve timestamp from frame handle in milliseconds
 param[in] frame      handle returned from a callback
 param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 return               the timestamp of the frame in milliseconds

}

function rs2_get_frame_timestamp(const frame: pRS2_frame; error: pRS2_error): Trs2_time_t;
  cdecl; external REALSENSE_DLL;
 {

 retrieve metadata from frame handle
param[in] frame      handle returned from a callback
param[in] frame_metadata  the rs2_frame_metadata whose latest frame we are interested in
param[out] error         if non-null, receives any error that occurs during this call, otherwise, errors are ignored
return            the metadata value

rs2_metadata_type rs2_get_frame_metadata(const rs2_frame* frame, rs2_frame_metadata_value frame_metadata, rs2_error** error);
 }
function rs2_get_frame_metadata(const frame: pRS2_frame;
  frame_metadata: Trs2_frame_metadata_value; error: pRS2_error): Trs2_metadata_type;
  cdecl; external REALSENSE_DLL;
{

 determine device metadata
 param[in] frame             handle returned from a callback
 param[in] frame_metadata    the metadata to check for support
 param[out] error         if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 return                true if device has this metadata

}
function rs2_supports_frame_metadata(const frame: pRS2_frame;
  frame_metadata: Trs2_frame_metadata_value; error: pRS2_error): integer;
  cdecl; external REALSENSE_DLL;
{

 retrieve frame parent sensor from frame handle
 param[in] frame      handle returned from a callback
 param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 return               the parent sensor of the frame
}
function rs2_get_frame_sensor(const frame: pRS2_frame; error: pRS2_error): prs2_sensor;
  cdecl; external REALSENSE_DLL;
{

retrieve frame width in pixels
param[in] frame      handle returned from a callback
param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
return               frame width in pixels

}

function rs2_get_frame_width(const frame: pRS2_frame; error: pRS2_error): integer;
  cdecl; external REALSENSE_DLL;

{

retrieve frame height in pixels
param[in] frame      handle returned from a callback
param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
return               frame height in pixels
 }

function rs2_get_frame_height(frame: pRS2_frame; error: pRS2_error): integer;
  cdecl; external REALSENSE_DLL;

{
 retrieve the scaling factor to use when converting a depth frame's get_data() units to meters
return single - depth, in meters, per 1 unit stored in the frame data
}
function rs2_depth_frame_get_units(frame: pRS2_frame; error: pRS2_error): single;
  cdecl; external REALSENSE_DLL;
{

retrieve frame stride in bytes (number of bytes from start of line N to start of line N+1)
param[in] frame      handle returned from a callback
param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
return               stride in bytes

}
function rs2_get_frame_stride_in_bytes(frame: pRS2_frame; error: pRS2_error): integer;
  cdecl; external REALSENSE_DLL;
{
retrieve bits per pixels in the frame image
(note that bits per pixel is not necessarily divided by 8, as in 12bpp)
param[in] frame      handle returned from a callback
param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
return               bits per pixel
}
function rs2_get_frame_bits_per_pixel(frame: pRS2_frame; error: pRS2_error): integer;
  cdecl; external REALSENSE_DLL;
{
/**
* create additional reference to a frame without duplicating frame data
* \param[in] frame      handle returned from a callback
* \param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
* \return               new frame reference, has to be released by rs2_release_frame
*/
void rs2_frame_add_ref(rs2_frame* frame, rs2_error ** error);
}
procedure rs2_frame_add_ref(frame: pRS2_frame; error: pRS2_error);
  cdecl; external REALSENSE_DLL;

{
 communicate to the library you intend to keep the frame alive for a while
 this will remove the frame from the regular count of the frame pool
 once this function is called, the SDK can no longer guarantee 0-allocations during frame cycling
 \param[in] frame handle returned from a callback
}
procedure rs2_keep_frame(frame: pRS2_frame); cdecl; external REALSENSE_DLL;
{

 When called on Points frame type, this method returns a pointer to an array of 3D vertices of the model
 The coordinate system is: X right, Y up, Z away from the camera. Units: Meters
 param[in] frame       Points frame
 param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored
 return                Pointer to an array of vertices, lifetime is managed by the frame

}
function rs2_get_frame_vertices(frame: pRS2_frame; error: pRS2_error): pRS2_vertex;
  cdecl; external REALSENSE_DLL;
{

 When called on Points frame type, this method creates a ply file of the model with the given file name.
 param[in] frame       Points frame
 param[in] fname       The name for the ply file
 param[in] texture     Texture frame
 param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored

}

procedure rs2_export_to_ply(frame: pRS2_frame; fname: PChar;
  texture: pRS2_frame; error: pRS2_error); cdecl; external REALSENSE_DLL;
{

When called on Points frame type, this method returns a pointer to an array of texture coordinates per vertex
Each coordinate represent a (u,v) pair within [0,1] range, to be mapped to texture image
param[in] frame       Points frame
param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored
return                Pointer to an array of texture coordinates, lifetime is managed by the frame

}
function rs2_get_frame_texture_coordinates(frame: pRS2_frame;
  error: pRS2_error): pRS2_pixel; cdecl; external REALSENSE_DLL;
{

When called on Points frame type, this method returns the number of vertices in the frame
param[in] frame       Points frame
param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored
return                Number of vertices

}
function rs2_get_frame_points_count(frame: pRS2_frame; error: pRS2_error): integer;
  cdecl; external REALSENSE_DLL;
{
Returns the stream profile that was used to start the stream of this frame
param[in] frame       frame reference, owned by the user
param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored
return                Pointer to the stream profile object, lifetime is managed elsewhere

}

function rs2_get_frame_stream_profile(frame: pRS2_frame;
  error: pRS2_error): pRS2_stream_profile; cdecl; external REALSENSE_DLL;

{

Test if the given frame can be extended to the requested extension
param[in]  frame             Realsense frame
param[in]  extension_type    The extension to which the frame should be tested if it is extendable
param[out] error             If non-null, receives any error that occurs during this call, otherwise, errors are ignored
return non-zero value iff the frame can be extended to the given extension

}
function rs2_is_frame_extendable_to(frame: pRS2_frame; extension_type: Trs2_extension;
  error: pRS2_error): integer; cdecl; external REALSENSE_DLL;
 {
 Allocate new video frame using a frame-source provided form a processing block
param[in] source      Frame pool to allocate the frame from
param[in] new_stream  New stream profile to assign to newly created frame
param[in] original    A reference frame that can be used to fill in auxilary information like format, width, height, bpp, stride (if applicable)
param[in] new_bpp     New value for bits per pixel for the allocated frame
param[in] new_width   New value for width for the allocated frame
param[in] new_height  New value for height for the allocated frame
param[in] new_stride  New value for stride in bytes for the allocated frame
param[in] frame_type  New value for frame type for the allocated frame
param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored
return                reference to a newly allocated frame, must be released with release_frame
                      memory for the frame is likely to be re-used from previous frame, but in lack of available frames in the pool will be allocated from the free store
 }

function rs2_allocate_synthetic_video_frame(Source: pRS2_source;
  new_stream: pRS2_stream_profile; original: pRS2_frame; new_bpp: integer;
  new_width: integer; new_height: integer; new_stride: integer;
  frame_type: Trs2_extension; error: pRS2_error): pRS2_frame; cdecl;
  external REALSENSE_DLL;
{

 Allocate new motion frame using a frame-source provided form a processing block
 param[in] source      Frame pool to allocate the frame from
 param[in] new_stream  New stream profile to assign to newly created frame
 param[in] original    A reference frame that can be used to fill in auxilary information like format, width, height, bpp, stride (if applicable)
 param[in] frame_type  New value for frame type for the allocated frame
 param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored
 return                reference to a newly allocated frame, must be released with release_frame
                        memory for the frame is likely to be re-used from previous frame, but in lack of available frames in the pool will be allocated from the free store

}
function rs2_allocate_synthetic_motion_frame(Source: pRS2_source;
  new_stream: pRS2_stream_profile; original: pRS2_frame;
  frame_type: Trs2_extension; error: pRS2_error): pRS2_frame; cdecl;
  external REALSENSE_DLL;
{

 Allocate new points frame using a frame-source provided from a processing block
 param[in] source      Frame pool to allocate the frame from
 param[in] new_stream  New stream profile to assign to newly created frame
 param[in] original    A reference frame that can be used to fill in auxilary information like format, width, height, bpp, stride (if applicable)
 param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored
 return                reference to a newly allocated frame, must be released with release_frame
                        memory for the frame is likely to be re-used from previous frame, but in lack of available frames in the pool will be allocated from the free store

}

function rs2_allocate_points(Source: pRS2_source; const new_stream: pRS2_stream_profile;
  original: pRS2_frame; error: pRS2_error): pRS2_frame; cdecl; external REALSENSE_DLL;

{
Allocate new composite frame, aggregating a set of existing frames
param[in] source      Frame pool to allocate the frame from
param[in] frames      Array of existing frames
param[in] count       Number of input frames
param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored
return                reference to a newly allocated frame, must be released with release_frame
                      when composite frame gets released it will automatically release all of the input frames

Note: This is a pointer to a Array of Frames
Todo: Test it.
Original declaration:
rs2_frame* rs2_allocate_composite_frame(rs2_source* source, rs2_frame** frames, int count, rs2_error** error);
}

function rs2_allocate_composite_frame(Source: pRS2_source; frames: pRS2_frame;
  Count: integer; error: pRS2_error): pRS2_frame; cdecl; external REALSENSE_DLL;

{

This method will dispatch frame callback on a frame
param[in] source      Frame pool provided by the processing block
param[in] frame       Frame to dispatch, frame ownership is passed to this function, so you don't have to call release_frame after it
param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored

}

procedure rs2_synthetic_frame_ready(Source: pRS2_source; frame: pRS2_frame;
  error: pRS2_error); cdecl; external REALSENSE_DLL;

{

 When called on Pose frame type, this method returns the transformation represented by the pose data
 param[in] frame       Pose frame
 param[out] pose       Pointer to a user allocated struct, which contains the pose info after a successful return
 param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored


}

procedure rs2_pose_frame_get_pose_data(frame: pRS2_frame; pose: pRS2_pose;
  error: pRS2_error); cdecl; external REALSENSE_DLL;
{
Extract the target dimensions on the specific target
param[in] frame            Left or right camera frame of specified size based on the target type
param[in] calib_type       Calibration target type
param[out] target_dims     The array to hold the result target dimensions calculated.
                              For type RS2_CALIB_TARGET_RECT_GAUSSIAN_DOT_VERTICES and RS2_CALIB_TARGET_ROI_RECT_GAUSSIAN_DOT_VERTICES, the four rectangle side sizes in pixels with the order of top, bottom, left, and right
                              For type RS2_CALIB_TARGET_POS_GAUSSIAN_DOT_VERTICES, the four vertices coordinates in pixels with the order of top, bottom, left, and right
param[in] target_dims_size Target dimension array size. 4 for RS2_CALIB_TARGET_RECT_GAUSSIAN_DOT_VERTICES and 8 for RS2_CALIB_TARGET_POS_GAUSSIAN_DOT_VERTICES.
param[out] error           If non-null, receives any error that occurs during this call, otherwise, errors are ignored

 }

procedure rs2_extract_target_dimensions(frame: pRS2_frame;
  calib_type: Trs2_calib_target_type; target_dims: single; dims_size: nativeuint;
  error: pRS2_error); cdecl; external REALSENSE_DLL;

implementation

end.
