{
  Exposes RealSense sensor functionality
}
unit rs_sensor;

interface

uses rs_types;

{ brief Read-only strings that can be queried from the device.
  Not all information attributes are available on all camera types.
  This information is mainly available for camera debug and troubleshooting
  and should not be used in applications. }
type
  Trs2_camera_info = (RS2_CAMERA_INFO_NAME, // < Friendly name
    RS2_CAMERA_INFO_SERIAL_NUMBER, // < Device serial number
    RS2_CAMERA_INFO_FIRMWARE_VERSION, // < Primary firmware version
    RS2_CAMERA_INFO_RECOMMENDED_FIRMWARE_VERSION,
    // < Recommended firmware version
    RS2_CAMERA_INFO_PHYSICAL_PORT,
    // < Unique identifier of the port the device is connected to (platform specific)
    RS2_CAMERA_INFO_DEBUG_OP_CODE,
    // < If device supports firmware logging,// this is the command to send to get logs from firmware
    RS2_CAMERA_INFO_ADVANCED_MODE,
    // < True iff the device is in advanced mode
    RS2_CAMERA_INFO_PRODUCT_ID,
    // < Product ID as reported in the USB descriptor
    RS2_CAMERA_INFO_CAMERA_LOCKED, // < True iff EEPROM is locked
    RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR,
    // < Designated USB specification: USB2/USB3
    RS2_CAMERA_INFO_PRODUCT_LINE,
    // < Device product line D400/SR300/L500/T200
    RS2_CAMERA_INFO_ASIC_SERIAL_NUMBER, // < ASIC serial number
    RS2_CAMERA_INFO_FIRMWARE_UPDATE_ID, // < Firmware update ID
    RS2_CAMERA_INFO_IP_ADDRESS, // < IP address for remote camera.
    RS2_CAMERA_INFO_COUNT
    // <  Number of enumeration values. Not a valid input: intended to be used in for-loops.
    );

function rs2_camera_info_to_string(info: Trs2_camera_info): PChar; cdecl;
  external REALSENSE_DLL;
{
  brief Streams are different types of data provided by RealSense devices./
}

type
  Trs2_stream = (RS2_STREAM_ANY, RS2_STREAM_DEPTH,
    // < Native stream of depth data produced by RealSense device */
    RS2_STREAM_COLOR,
    // < Native stream of color data captured by RealSense device */
    RS2_STREAM_INFRARED,
    // < Native stream of infrared data captured by RealSense device */
    RS2_STREAM_FISHEYE,
    // < Native stream of fish-eye (wide) data captured from the dedicate motion camera */
    RS2_STREAM_GYRO,
    // < Native stream of gyroscope motion data produced by RealSense device */
    RS2_STREAM_ACCEL,
    // < Native stream of accelerometer motion data produced by RealSense device */
    RS2_STREAM_GPIO,
    // < Signals from external device connected through GPIO */
    RS2_STREAM_POSE,
    // < 6 Degrees of Freedom pose data, calculated by RealSense device */
    RS2_STREAM_CONFIDENCE,
    // < 4 bit per-pixel depth confidence level */
    RS2_STREAM_COUNT);

function rs2_stream_to_string(stream: Trs2_stream): PChar; cdecl;
  external REALSENSE_DLL;

{
  A stream's format identifies how binary data is encoded within a frame. */
}
type
  Trs2_format = (RS2_FORMAT_ANY,
    // < When passed to enable stream, librealsense will try to provide best suited format */
    RS2_FORMAT_Z16,
    // < 16-bit linear depth values. The depth is meters is equal to depth scale * pixel value. */
    RS2_FORMAT_DISPARITY16,
    // < 16-bit float-point disparity values. Depth->Disparity conversion : Disparity = Baseline*FocalLength/Depth. */
    RS2_FORMAT_XYZ32F, // < 32-bit floating point 3D coordinates. */
    RS2_FORMAT_YUYV,
    // < 32-bit y0, u, y1, v data for every two pixels. Similar to YUV422 but packed in a different order - https://en.wikipedia.org/wiki/YUV */
    RS2_FORMAT_RGB8, // < 8-bit red, green and blue channels */
    RS2_FORMAT_BGR8,
    // < 8-bit blue, green, and red channels -- suitable for OpenCV */
    RS2_FORMAT_RGBA8,
    // < 8-bit red, green and blue channels + constant alpha channel equal to FF */
    RS2_FORMAT_BGRA8,
    // < 8-bit blue, green, and red channels + constant alpha channel equal to FF */
    RS2_FORMAT_Y8, // < 8-bit per-pixel grayscale image */
    RS2_FORMAT_Y16, // < 16-bit per-pixel grayscale image */
    RS2_FORMAT_RAW10,
    // < Four 10 bits per pixel luminance values packed into a 5-byte macropixel */
    RS2_FORMAT_RAW16, // < 16-bit raw image */
    RS2_FORMAT_RAW8, // < 8-bit raw image */
    RS2_FORMAT_UYVY,
    // < Similar to the standard YUYV pixel format, but packed in a different order */
    RS2_FORMAT_MOTION_RAW, // < Raw data from the motion sensor */
    RS2_FORMAT_MOTION_XYZ32F,
    // < Motion data packed as 3 32-bit float values, for X, Y, and Z axis */
    RS2_FORMAT_GPIO_RAW,
    // < Raw data from the external sensors hooked to one of the GPIO's */
    RS2_FORMAT_6DOF,
    // < Pose data packed as floats array, containing translation vector, rotation quaternion and prediction velocities and accelerations vectors */
    RS2_FORMAT_DISPARITY32,
    // < 32-bit float-point disparity values. Depth->Disparity conversion : Disparity = Baseline*FocalLength/Depth */
    RS2_FORMAT_Y10BPACK,
    // < 16-bit per-pixel grayscale image unpacked from 10 bits per pixel packed ([8:8:8:8:2222]) grey-scale image. The data is unpacked to LSB and padded with 6 zero bits */
    RS2_FORMAT_DISTANCE, // < 32-bit float-point depth distance value.  */
    RS2_FORMAT_MJPEG,
    // < Bitstream encoding for video in which an image of each frame is encoded as JPEG-DIB   */
    RS2_FORMAT_Y8I,
    // < 8-bit per pixel interleaved. 8-bit left, 8-bit right.  */
    RS2_FORMAT_Y12I,
    // < 12-bit per pixel interleaved. 12-bit left, 12-bit right. Each pixel is stored in a 24-bit word in little-endian order. */
    RS2_FORMAT_INZI, // < multi-planar Depth 16bit + IR 10bit.  */
    RS2_FORMAT_INVI, // < 8-bit IR stream.  */
    RS2_FORMAT_W10,
    // < Grey-scale image as a bit-packed array. 4 pixel data stream taking 5 bytes */
    RS2_FORMAT_Z16H,
    // < Variable-length Huffman-compressed 16-bit depth values. */
    RS2_FORMAT_FG, // < 16-bit per-pixel frame grabber format. */
    RS2_FORMAT_Y411, // < 12-bit per-pixel. */
    RS2_FORMAT_COUNT
    // < Number of enumeration values. Not a valid input: intended to be used in for-loops. */

    );

function rs2_format_to_string(format: Trs2_format): PChar; cdecl;
  external REALSENSE_DLL;

{
  Cross-stream extrinsics: encodes the topology describing how the different devices are oriented. */
}
type
  Trs2_extrinsics = record
    rotation: array [0 .. 8] of single;
    translation: array [0 .. 2] of single;
  end;

  pRS2_extrinsics = ^Trs2_extrinsics;

  {

    Deletes sensors list, any sensors created from this list will remain unaffected
    param[in] info_list list to delete

  }

procedure rs2_delete_sensor_list(info_list: pRS2_sensor_list); cdecl;
  external REALSENSE_DLL;
{

  Determines number of sensors in a list
  param[in] info_list The list of connected sensors captured using rs2_query_sensors
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return            Sensors count
}

function rs2_get_sensors_count(info_list: pRS2_sensor; error: pRS2_error)
  : integer; cdecl; external REALSENSE_DLL;

{

  delete relasense sensor
  param[in] sensor realsense sensor to delete
}

procedure rs2_delete_sensor(sendor: pRS2_sensor); cdecl; external REALSENSE_DLL;
{
  create sensor by index
  param[in] index   the zero based index of sensor to retrieve
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return            the requested sensor, should be released by rs2_delete_sensor

  rs2_sensor* rs2_create_sensor(const rs2_sensor_list* list, int index, rs2_error** error);
}

function rs2_create_sensor(const list: pRS2_sensor_list; index: integer;
  error: pRS2_error): pRS2_sensor; cdecl; external REALSENSE_DLL;

{
  This is a helper function allowing the user to discover the device from one of its sensors
  param[in] sensor     Pointer to a sensor
  param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return               new device wrapper for the device of the sensor. Needs to be released by delete_device
}

function rs2_create_device_from_sensor(sensor: pRS2_sensor; error: pRS2_error)
  : pRS2_device; cdecl; external REALSENSE_DLL;
{
  retrieve sensor specific information, like versions of various internal components
  param[in] sensor     the RealSense sensor
  param[in] info       camera info type to retrieve
  param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return               the requested camera info string, in a format specific to the device model
}
function rs2_get_sensor_info(const sensor: pRS2_sensor; info: Trs2_camera_info;
  error: pRS2_error): PChar; cdecl; external REALSENSE_DLL;

{
  check if specific sensor info is supported
  param[in] info    the parameter to check for support
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return                true if the parameter both exist and well-defined for the specific device
}

function rs2_supports_sensor_info(const sensor: pRS2_sensor;
  info: Trs2_camera_info; error: pRS2_error): integer; cdecl;
  external REALSENSE_DLL;

{
  Test if the given sensor can be extended to the requested extension
  param[in] sensor  Realsense sensor
  param[in] extension The extension to which the sensor should be tested if it is extendable
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return non-zero value iff the sensor can be extended to the given extension
}
function rs2_is_sensor_extendable_to(const sensor: pRS2_sensor;
  extension: Trs2_extension; error: pRS2_error): integer; cdecl;
  external REALSENSE_DLL;
{
  When called on a depth sensor, this method will return the number of meters represented by a single depth unit
  param[in] sensor      depth sensor
  param[out] error      if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return                the number of meters represented by a single depth unit
}

function rs2_get_depth_scale(sensor: pRS2_sensor; error: pRS2_error): single;
  cdecl; external REALSENSE_DLL;
{
  Retrieve the stereoscopic baseline value from frame. Applicable to stereo-based depth modules
  param[out] float  Stereoscopic baseline in millimeters
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
function rs2_depth_stereo_frame_get_baseline(const frame: pRS2_frame;
  error: pRS2_error): single; cdecl; external REALSENSE_DLL;
{

  Retrieve the stereoscopic baseline value from sensor. Applicable to stereo-based depth modules
  param[out] float  Stereoscopic baseline in millimeters
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored

}
function rs2_get_stereo_baseline(sensor: pRS2_sensor; error: pRS2_error)
  : single; cdecl; external REALSENSE_DLL;
{

  sets the active region of interest to be used by auto-exposure algorithm
  param[in] sensor     the RealSense sensor
  param[in] min_x      lower horizontal bound in pixels
  param[in] min_y      lower vertical bound in pixels
  param[in] max_x      upper horizontal bound in pixels
  param[in] max_y      upper vertical bound in pixels
  param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored

}
procedure rs2_set_region_of_interest(sensor: pRS2_sensor; min_x: integer;
  min_y: integer; max_x: integer; max_y: integer; error: pRS2_error); cdecl;
  external REALSENSE_DLL;
{

  brief gets the active region of interest to be used by auto-exposure algorithm
  param[in] sensor     the RealSense sensor
  param[out] min_x     lower horizontal bound in pixels
  param[out] min_y     lower vertical bound in pixels
  param[out] max_x     upper horizontal bound in pixels
  param[out] max_y     upper vertical bound in pixels
  param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_get_region_of_interest(sensor: pRS2_sensor; min_x: PInteger;
  min_y: PInteger; max_x: PInteger; max_y: pRS2_error; error: pRS2_error);
  cdecl; external REALSENSE_DLL;
{

  open subdevice for exclusive access, by committing to a configuration
  param[in] device relevant RealSense device
  param[in] profile    stream profile that defines single stream configuration
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored

}

procedure rs2_open(device: pRS2_sensor; const profile: pRS2_stream_profile;
  error: pRS2_error); cdecl; external REALSENSE_DLL;
{

  open subdevice for exclusive access, by committing to composite configuration, specifying one or more stream profiles
  this method should be used for interdependent  streams, such as depth and infrared, that have to be configured together
  param[in] device relevant RealSense device
  param[in] profiles  list of stream profiles discovered by get_stream_profiles
  param[in] count      number of simultaneous  stream profiles to configure
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored

}
procedure rs2_open_multiple(device: pRS2_sensor;
  const profiles: pRS2_stream_profile; Count: integer; erroro: pRS2_error);
  cdecl; external REALSENSE_DLL;
{

  stop any streaming from specified subdevice
  param[in] sensor     RealSense device
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored

}
procedure rs2_close(sensor: pRS2_sensor; error: pRS2_error); cdecl;
  external REALSENSE_DLL;
{

  start streaming from specified configured sensor
  param[in] sensor  RealSense device
  param[in] on_frame function pointer to register as per-frame callback
  param[in] user auxiliary  data the user wishes to receive together with every frame callback
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored

}
procedure rs2_start(sensor: pRS2_sensor; on_frame: pRS2_frame; user: pvoid;
  error: pRS2_error); cdecl; external REALSENSE_DLL;
{
  start streaming from specified configured sensor of specific stream to frame queue
  param[in] sensor  RealSense Sensor
  param[in] queue   frame-queue to store new frames into
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_start_queue(sensor: pRS2_sensor; queue: pRS2_frame_queue;
  error: pRS2_error); cdecl; external REALSENSE_DLL;
{

  stops streaming from specified configured device
  param[in] sensor  RealSense sensor
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored

}

procedure rs2_stop(sensor: pRS2_sensor; error: pRS2_error); cdecl;
  external REALSENSE_DLL;
{

  set callback to get notifications from specified sensor
  param[in] sensor          RealSense device
  param[in] on_notification function pointer to register as per-notifications callback
  param[out] error          if non-null, receives any error that occurs during this call, otherwise, errors are ignored

}
procedure rs2_set_notifications_callback(sensor: pRS2_sensor;
  on_notification: pRS2_notification_callback_ptr; user: pvoid;
  error: pRS2_error); cdecl; external REALSENSE_DLL;
{

  retrieve description from notification handle
  param[in] notification      handle returned from a callback
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return            the notification description
}
function rs2_get_notification_description(notification: pRS2_notification;
  error: pRS2_error): PChar; cdecl; external REALSENSE_DLL;
{
  retrieve timestamp from notification handle
  param[in] notification      handle returned from a callback
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return            the notification timestamp
}
function rs2_get_notification_timestamp(notification: pRS2_notification;
  error: pRS2_error): Trs2_time_t; cdecl; external REALSENSE_DLL;
{
  retrieve severity from notification handle
  param[in] notification      handle returned from a callback
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return            the notification severity
}
function rs2_get_notification_severity(notification: pRS2_notification;
  error: pRS2_error): Trs2_log_severity; cdecl; external REALSENSE_DLL;
{
  retrieve category from notification handle
  param[in] notification      handle returned from a callback
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return            the notification category
}
function rs2_get_notification_category(notification: pRS2_notification;
  error: pRS2_error): Trs2_notification_category; cdecl; external REALSENSE_DLL;
{
  retrieve serialized data from notification handle
  param[in] notification      handle returned from a callback
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return            the serialized data (in JSON format)
}
function rs2_get_notification_serialized_data(notification: pRS2_notification;
  error: pRS2_error): PChar; cdecl; external REALSENSE_DLL;
{
  check if physical subdevice is supported
  param[in] sensor  input RealSense subdevice
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return            list of stream profiles that given subdevice can provide, should be released by rs2_delete_profiles_list
}
function rs2_get_stream_profiles(sensor: pRS2_sensor; error: pRS2_error)
  : prs2_stream_profile_list; cdecl; external REALSENSE_DLL;
{
  retrieve list of debug stream profiles that given subdevice can provide
  param[in] sensor  input RealSense subdevice
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return            list of debug stream profiles that given subdevice can provide, should be released by rs2_delete_profiles_list
}
function rs2_get_debug_stream_profiles(sensor: pRS2_sensor; error: pRS2_error)
  : prs2_stream_profile_list; cdecl; external REALSENSE_DLL;
{
  check how subdevice is streaming
  param[in] sensor  input RealSense subdevice
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return            list of stream profiles that given subdevice is currently streaming, should be released by rs2_delete_profiles_list
}
function rs2_get_active_streams(sensor: pRS2_sensor; error: pRS2_error)
  : prs2_stream_profile_list; cdecl; external REALSENSE_DLL;
{
  Get pointer to specific stream profile
  param[in] list        the list of supported profiles returned by rs2_get_supported_profiles
  param[in] index       the zero based index of the streaming mode
  param[out] error      if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
function rs2_get_stream_profile(const list: pRS2_stream_profile; index: integer;
  error: pRS2_error): pRS2_stream_profile; cdecl; external REALSENSE_DLL;
{
  Extract common parameters of a stream profiles
  param[in] mode        input stream profile
  param[out] stream     stream type of the input profile
  param[out] format     binary data format of the input profile
  param[out] index      stream index the input profile in case there are multiple streams of the same type
  param[out] unique_id  identifier for the stream profile, unique within the application
  param[out] framerate  expected rate for data frames to arrive, meaning expected number of frames per second
  param[out] error      if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_get_stream_profile_data(const mode: pRS2_stream_profile;
  stream: prs2_stream; format: pRS2_format; index: integer; unique_id: PInteger;
  framerate: PInteger; error: pRS2_error); cdecl; external REALSENSE_DLL;
{
  Creates a copy of stream profile, assigning new values to some of the fields
  param[in] mode        input stream profile
  param[in] stream      stream type for the profile
  param[in] format      binary data format of the profile
  param[in] index       stream index the profile in case there are multiple streams of the same type
  param[out] error      if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return                new stream profile, must be deleted by rs2_delete_stream_profile
}
function rs2_clone_stream_profile(const mode: pRS2_stream_profile;
  stream: Trs2_stream; index: integer; format: Trs2_format; error: pRS2_error)
  : pRS2_stream_profile; cdecl; external REALSENSE_DLL;
{
  Creates a copy of stream profile, assigning new values to some of the fields
  param[in] mode        input stream profile
  param[in] stream      stream type for the profile
  param[in] format      binary data format of the profile
  param[in] width       new width for the profile
  param[in] height      new height for the profile
  param[in] intr        new intrinsics for the profile
  param[in] index       stream index the profile in case there are multiple streams of the same type
  param[out] error      if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return                new stream profile, must be deleted by rs2_delete_stream_profile
}
function rs2_clone_video_stream_profile(const mode: pRS2_stream_profile;
  stream: Trs2_stream; index: integer; format: Trs2_format; Width: integer;
  Height: integer; const intr: pRS2_intrinsics; error: pRS2_error)
  : pRS2_stream_profile; cdecl; external REALSENSE_DLL;
{
  Delete stream profile allocated by rs2_clone_stream_profile
  Should not be called on stream profiles returned by the device
  param[in] mode        input stream profile
}
procedure rs2_delete_stream_profile(mode: pRS2_stream_profile); cdecl;
  external REALSENSE_DLL;
{
  Try to extend stream profile to an extension type
  param[in] mode        input stream profile
  param[in] type        extension type, for example RS2_EXTENSION_VIDEO_STREAM_PROFILE
  param[out] error      if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return                non-zero if profile is extendable to specified extension, zero otherwise
}
function rs2_stream_profile_is(mode: pRS2_stream_profile; atype: Trs2_extension;
  error: pRS2_error): integer; cdecl; external REALSENSE_DLL;
{
  When called on a video stream profile, will return the width and the height of the stream
  param[in] mode        input stream profile
  param[out] width      width in pixels of the video stream
  param[out] height     height in pixels of the video stream
  param[out] error      if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_get_video_stream_resolution(const mode: pRS2_stream_profile;
  Width: PInteger; Height: PInteger; error: pRS2_error); cdecl;
  external REALSENSE_DLL;
{
  Obtain the intrinsics of a specific stream configuration from the device.
  param[in] mode          input stream profile
  param[out] intrinsics   Pointer to the struct to store the data in
  param[out] error        If non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_get_motion_intrinsics(const mode: pRS2_stream_profile;
  intrinsics: pRS2_motion_device_intrinsic; error: pRS2_error); cdecl;
  external REALSENSE_DLL;
{
  Returns non-zero if selected profile is recommended for the sensor
  This is an optional hint we offer to suggest profiles with best performance-quality tradeof
  param[in] mode        input stream profile
  param[out] error      if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return                non-zero if selected profile is recommended for the sensor
}
function rs2_is_stream_profile_default(const mode: pRS2_stream_profile;
  error: pRS2_error): integer; cdecl; external REALSENSE_DLL;
{
  get the number of supported stream profiles
  param[in] list        the list of supported profiles returned by rs2_get_supported_profiles
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return number of supported subdevice profiles
}
function rs2_get_stream_profiles_count(const list: prs2_stream_profile_list;
  error: pRS2_error): integer; cdecl; external REALSENSE_DLL;
{
  delete stream profiles list
  param[in] list        the list of supported profiles returned by rs2_get_supported_profiles
}
procedure rs2_delete_stream_profile_list(const list: prs2_stream_profile_list);
  cdecl; external REALSENSE_DLL;
{
  param[in] from          origin stream profile
  param[in] to            target stream profile
  param[out] extrin       extrinsics from origin to target
  param[out] error        if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_get_extrinsics(const afrom: pRS2_stream_profile;
  ato: pRS2_stream_profile; extrin: pRS2_extrinsics; error: pRS2_error); cdecl;
  external REALSENSE_DLL;
{
  param[in] from          origin stream profile
  param[in] to            target stream profile
  param[out] extrin       extrinsics from origin to target
  param[out] error        if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_register_extrinsics(const afrom: pRS2_stream_profile;
  ato: pRS2_stream_profile; extrin: pRS2_extrinsics; error: pRS2_error); cdecl;
  external REALSENSE_DLL;
{
  Override extrinsics of a given sensor that supports calibrated_sensor.

  This will affect extrinsics at the source device and may affect multiple profiles. Used for DEPTH_TO_RGB calibration.

  param[in] sensor       The sensor
  param[in] extrinsics   Extrinsics from Depth to the named sensor
  param[out] error       If non-null, receives any error that occurs during this call, otherwise, errors are ignored
}

procedure rs2_override_extrinsics(const sensor: pRS2_sensor;
  const extrinsics: pRS2_extrinsics; error: pRS2_error); cdecl;
  external REALSENSE_DLL;
{
  When called on a video profile, returns the intrinsics of specific stream configuration
  param[in] mode          input stream profile
  param[out] intrinsics   resulting intrinsics for the video profile
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_get_video_stream_intrinsics(const sensor: pRS2_sensor;
  const extrinsics: pRS2_extrinsics; error: pRS2_error); cdecl;
  external REALSENSE_DLL;
{
  Returns the list of recommended processing blocks for a specific sensor.
  Order and configuration of the blocks are decided by the sensor
  param[in] sensor          input sensor
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return list of supported sensor recommended processing blocks
}
function rs2_get_recommended_processing_blocks(sensor: pRS2_sensor;
  error: pRS2_error): pRS2_processing_block_list; cdecl; external REALSENSE_DLL;
{
  Returns specific processing blocks from processing blocks list
  param[in] list           the processing blocks list
  param[in] index          the requested processing block
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return processing block
}
function rs2_get_processing_block(const list: pRS2_processing_block_list;
  index: integer; error: pRS2_error): pRS2_processing_block; cdecl;
  external REALSENSE_DLL;
{
  Returns the processing blocks list size
  param[in] list           the processing blocks list
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return the processing block list size
}
function rs2_get_recommended_processing_blocks_count
  (const list: pRS2_processing_block; error: pRS2_error): integer; cdecl;
  external REALSENSE_DLL;
{
  Deletes processing blocks list
  param[in] list list to delete
}
procedure rs2_delete_recommended_processing_blocks(list: pRS2_processing_block);
  cdecl; external REALSENSE_DLL;
{
  Imports a localization map from file to tm2 tracking device
  param[in]  sensor        TM2 position-tracking sensor
  param[in]  lmap_blob     Localization map raw buffer, serialized
  param[in]  blob_size     The buffer's size in bytes
  param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return                   Non-zero if succeeded, otherwise 0
}
function rs2_import_localization_map(const sensor: pRS2_sensor;
  const lmap_blob: pbyte; blob_size: cardinal; error: pRS2_error): integer;
  cdecl; external REALSENSE_DLL;
{
  Extract and store the localization map of tm2 tracking device to file
  param[in]  sensor        TM2 position-tracking sensor
  param[in]  lmap_fname    The file name of the localization map
  param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return                   Device's response in a rs2_raw_data_buffer, which should be released by rs2_delete_raw_data
}
function rs2_export_localization_map(sensor: pRS2_sensor; error: pRS2_error)
  : pRS2_raw_data_buffer; cdecl; external REALSENSE_DLL;
{
  Create a named location tag
  param[in]  sensor    T2xx position-tracking sensor
  param[in]  guid      Null-terminated string of up to 127 characters
  param[in]  pos       Position in meters, relative to the current tracking session
  param[in]  orient    Quaternion orientation, expressed the the coordinate system of the current tracking session
  param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return               Non-zero if succeeded, otherwise 0
}

function rs2_set_static_node(const sensor: pRS2_sensor; guid: PChar;
  const pos: Trs2_vector; const orient: Trs2_quaternion; error: pRS2_error)
  : integer; cdecl; external REALSENSE_DLL;
{
  Retrieve a named location tag
  param[in]  sensor    T2xx position-tracking sensor
  param[in]  guid      Null-terminated string of up to 127 characters
  param[out] pos       Position in meters of the tagged (stored) location
  param[out] orient    Quaternion orientation of the tagged (stored) location
  param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return               Non-zero if succeeded, otherwise 0
}
function rs2_get_static_node(const sensor: pRS2_sensor; const guid: PChar;
  const pos: Trs2_vector; const orient: Trs2_quaternion; error: pRS2_error)
  : integer; cdecl; external REALSENSE_DLL;
{
  Remove a named location tag
  param[in]  sensor    T2xx position-tracking sensor
  param[in]  guid      Null-terminated string of up to 127 characters
  param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return               Non-zero if succeeded, otherwise 0
}
function rs2_remove_static_node(const sensor: pRS2_sensor; const guid: PChar;
  error: pRS2_error): integer; cdecl; external REALSENSE_DLL;
{
  /** Load Wheel odometer settings from host to device
  * \param[in] odometry_config_buf   odometer configuration/calibration blob serialized from jsom file
  * \return true on success
  */
  int rs2_load_wheel_odometry_config(const rs2_sensor* sensor, const unsigned char* odometry_config_buf, unsigned int blob_size, rs2_error** error);
}
function rs2_load_wheel_odometry_config(sendor: pRS2_sensor;
  odometry_config_buf: pbyte; blob_size: cardinal; error: pRS2_error): integer;
  cdecl; external REALSENSE_DLL;
{
  param[in] wo_sensor_id       - Zero-based index of (wheel) sensor with the same type within device
  param[in] frame_num          - Monotonocally increasing frame number, managed per sensor.
  param[in] translational_velocity   - Translational velocity of the wheel sensor [meter/sec]
  return true on success
}
function rs2_send_wheel_odometry(const sensor: pRS2_sensor; wo_sensor_id: char;
  frame_num: cardinal; const translation_velocity: Trs2_vector;
  error: pRS2_error): integer; cdecl; external REALSENSE_DLL;
{
  Set intrinsics of a given sensor
  param[in] sensor       The RealSense device
  param[in] profile      Target stream profile
  param[in] intrinsics   Intrinsics value to be written to the device
  param[out] error       If non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_set_intrinsics(const sensor: pRS2_sensor;
  const profile: pRS2_stream_profile; intrinsics: pRS2_intrinsics;
  error: pRS2_error); cdecl; external REALSENSE_DLL;
{
  Override intrinsics of a given sensor that supports calibrated_sensor.

  This will affect intrinsics at the source and may affect multiple profiles. Used for DEPTH_TO_RGB calibration.

  param[in] sensor       The RealSense device
  param[in] intrinsics   Intrinsics value to be written to the sensor
  param[out] error       If non-null, receives any error that occurs during this call, otherwise, errors are ignored
}

procedure rs2_override_intrinsics(const sensor: pRS2_sensor;
  const extrinsics: pRS2_extrinsics; error: pRS2_error); cdecl;
  external REALSENSE_DLL;
{
  Get the DSM parameters for a sensor
  param[in]  sensor        Sensor that supports the CALIBRATED_SENSOR extension
  param[out] p_params_out  Pointer to the structure that will get the DSM parameters
  param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_get_dsm_params(sensor: pRS2_sensor; p_params_out: pRS2_dsm_params;
  error: pRS2_error); cdecl; external REALSENSE_DLL;
{
  Set the sensor DSM parameters
  This should ideally be done when the stream is NOT running. If it is, the
  parameters may not take effect immediately.
  param[in]  sensor        Sensor that supports the CALIBRATED_SENSOR extension
  param[out] p_params      Pointer to the structure that contains the DSM parameters
  param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_override_dsm_params(const sensor: pRS2_sensor;
  p_params: pRS2_dsm_params; error: pRS2_error); cdecl; external REALSENSE_DLL;
{
  Reset the sensor DSM parameters
  This should ideally be done when the stream is NOT running. May not take effect immediately.
  param[in]  sensor        Sensor that supports the CALIBRATED_SENSOR extension
  param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_reset_sensor_calibration(const sensor: pRS2_sensor;
  error: pRS2_error); cdecl; external REALSENSE_DLL;
{
  Set motion device intrinsics
  param[in]  sensor       Motion sensor
  param[in]  profile      Motion stream profile
  param[out] intrinsics   Pointer to the struct to store the data in
  param[out] error        If non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_set_motion_device_intrinsics(const sensor: pRS2_sensor;
  profile: pRS2_stream_profile; const intrinsics: pRS2_motion_device_intrinsic;
  error: pRS2_error); cdecl; external REALSENSE_DLL;
{
  When called on a depth sensor, this method will return the maximum range of the camera given the amount of ambient light in the scene
  param[in] sensor      depth sensor
  param[out] error      if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return                the max usable range in meters
}

function rs2_get_max_usable_depth_range(sensor: pRS2_sensor; error: pRS2_error)
  : single; cdecl; external REALSENSE_DLL;

implementation

end.
