{
Exposes RealSense structs
}
unit rs_types;

interface



const
{$IFDEF MSWINDOWS}
  REALSENSE_DLL = 'realsense2.dll';
{$ENDIF}
{$IFDEF LINUX}
  REALSENSE_DLL = 'realsense2.so';
{$ENDIF}



type
  pRS2_error = POpaqueData;
  pRS2_context = POpaqueData;
  pRS2_device_list = POpaqueData;
  pRS2_device_info = POpaqueData;
  pRS2_device = POpaqueData;
  pvoid = POpaqueData;          //?? void*
  pRS2_device_hub = POpaqueData;
  pRS2_pipeline = POpaqueData;
  pRS2_exception_type = POpaqueData;
  pRS2_config = POpaqueData;
  pRS2_pipeline_profile = POpaqueData;
  pRS2_frame = POpaqueData;
  pRS2_raw_data_buffer = POpaqueData;
  pRS2_sensor_list = POpaqueData;
  pRS2_frame_queue = POpaqueData;
  pRS2_sensor = POpaqueData;
  pRS2_stream_profile = POpaqueData;
  prs2_stream_profile_list = POpaqueData;
  pRS2_source = POpaqueData;
  pRS2_notification = POpaqueData;
  pRS2_processing_block_list = POpaqueData;
  pRS2_processing_block = POpaqueData;
  pRS2_frame_processor_callback = POpaqueData;
  pRS2_frame_processor_callback_ptr = POpaqueData;
  pRS2_options_list = POpaqueData;
  pRS2_options = POpaqueData;
  pRS2_log_message = POpaqueData;
  pRS2_firmware_log_message = POpaqueData;
  pRS2_firmware_log_parsed_message = POpaqueData;
  pRS2_terminal_parser = POpaqueData;

type
  Trs2_time_t = double;     //< Timestamp format. units are milliseconds
  Trs2_metadata_type = Int64;
//< Metadata attribute type is defined as 64 bit signed integer

{
  Category of the librealsense notification.
}
type
  Trs2_notification_category = (
    RS2_NOTIFICATION_CATEGORY_FRAMES_TIMEOUT,
    //< Frames didn't arrived within 5 seconds
    RS2_NOTIFICATION_CATEGORY_FRAME_CORRUPTED,
    //< Received partial/incomplete frame
    RS2_NOTIFICATION_CATEGORY_HARDWARE_ERROR,
    //< Error reported from the device
    RS2_NOTIFICATION_CATEGORY_HARDWARE_EVENT,
    //< General Hardeware notification that is not an error
    RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR,
    //< Received unknown error from the device
    RS2_NOTIFICATION_CATEGORY_FIRMWARE_UPDATE_RECOMMENDED,
    //< Current firmware version installed is not the latest available
    RS2_NOTIFICATION_CATEGORY_POSE_RELOCALIZATION,
    //< A relocalization event has updated the pose provided by a pose sensor
    RS2_NOTIFICATION_CATEGORY_COUNT
    //< Number of enumeration values. Not a valid input: intended to be used in for-loops.
    );


function rs2_notification_category_to_string(category:
  Trs2_notification_category): PChar;
  cdecl; external REALSENSE_DLL;
{
  Specifies advanced interfaces (capabilities) objects may implement.
}
type
  Trs2_extension = (
    RS2_EXTENSION_UNKNOWN,
    RS2_EXTENSION_DEBUG,
    RS2_EXTENSION_INFO,
    RS2_EXTENSION_MOTION,
    RS2_EXTENSION_OPTIONS,
    RS2_EXTENSION_VIDEO,
    RS2_EXTENSION_ROI,
    RS2_EXTENSION_DEPTH_SENSOR,
    RS2_EXTENSION_VIDEO_FRAME,
    RS2_EXTENSION_MOTION_FRAME,
    RS2_EXTENSION_COMPOSITE_FRAME,
    RS2_EXTENSION_POINTS,
    RS2_EXTENSION_DEPTH_FRAME,
    RS2_EXTENSION_ADVANCED_MODE,
    RS2_EXTENSION_RECORD,
    RS2_EXTENSION_VIDEO_PROFILE,
    RS2_EXTENSION_PLAYBACK,
    RS2_EXTENSION_DEPTH_STEREO_SENSOR,
    RS2_EXTENSION_DISPARITY_FRAME,
    RS2_EXTENSION_MOTION_PROFILE,
    RS2_EXTENSION_POSE_FRAME,
    RS2_EXTENSION_POSE_PROFILE,
    RS2_EXTENSION_TM2,
    RS2_EXTENSION_SOFTWARE_DEVICE,
    RS2_EXTENSION_SOFTWARE_SENSOR,
    RS2_EXTENSION_DECIMATION_FILTER,
    RS2_EXTENSION_THRESHOLD_FILTER,
    RS2_EXTENSION_DISPARITY_FILTER,
    RS2_EXTENSION_SPATIAL_FILTER,
    RS2_EXTENSION_TEMPORAL_FILTER,
    RS2_EXTENSION_HOLE_FILLING_FILTER,
    RS2_EXTENSION_ZERO_ORDER_FILTER,
    RS2_EXTENSION_RECOMMENDED_FILTERS,
    RS2_EXTENSION_POSE,
    RS2_EXTENSION_POSE_SENSOR,
    RS2_EXTENSION_WHEEL_ODOMETER,
    RS2_EXTENSION_GLOBAL_TIMER,
    RS2_EXTENSION_UPDATABLE,
    RS2_EXTENSION_UPDATE_DEVICE,
    RS2_EXTENSION_L500_DEPTH_SENSOR,
    RS2_EXTENSION_TM2_SENSOR,
    RS2_EXTENSION_AUTO_CALIBRATED_DEVICE,
    RS2_EXTENSION_COLOR_SENSOR,
    RS2_EXTENSION_MOTION_SENSOR,
    RS2_EXTENSION_FISHEYE_SENSOR,
    RS2_EXTENSION_DEPTH_HUFFMAN_DECODER,
    RS2_EXTENSION_SERIALIZABLE,
    RS2_EXTENSION_FW_LOGGER,
    RS2_EXTENSION_AUTO_CALIBRATION_FILTER,
    RS2_EXTENSION_DEVICE_CALIBRATION,
    RS2_EXTENSION_CALIBRATED_SENSOR,
    RS2_EXTENSION_HDR_MERGE,
    RS2_EXTENSION_SEQUENCE_ID_FILTER,
    RS2_EXTENSION_MAX_USABLE_RANGE_SENSOR,
    RS2_EXTENSION_DEBUG_STREAM_SENSOR,
    RS2_EXTENSION_CALIBRATION_CHANGE_DEVICE,
    RS2_EXTENSION_COUNT);


function rs2_extension_type_to_string(atype: Trs2_extension): PChar;
  cdecl; external REALSENSE_DLL;

function rs2_extension_to_string(aType: Trs2_extension): PChar;
  cdecl; external REALSENSE_DLL;

{
 A stream's format identifies how binary data is encoded within a frame. */
}
type
  Trs2_format = (
    RS2_FORMAT_ANY,
    //< When passed to enable stream, librealsense will try to provide best suited format */
    RS2_FORMAT_Z16,
    //< 16-bit linear depth values. The depth is meters is equal to depth scale * pixel value. */
    RS2_FORMAT_DISPARITY16,
    //< 16-bit float-point disparity values. Depth->Disparity conversion : Disparity = Baseline*FocalLength/Depth. */
    RS2_FORMAT_XYZ32F, //< 32-bit floating point 3D coordinates. */
    RS2_FORMAT_YUYV,
    //< 32-bit y0, u, y1, v data for every two pixels. Similar to YUV422 but packed in a different order - https://en.wikipedia.org/wiki/YUV */
    RS2_FORMAT_RGB8, //< 8-bit red, green and blue channels */
    RS2_FORMAT_BGR8,
    //< 8-bit blue, green, and red channels -- suitable for OpenCV */
    RS2_FORMAT_RGBA8,
    //< 8-bit red, green and blue channels + constant alpha channel equal to FF */
    RS2_FORMAT_BGRA8,
    //< 8-bit blue, green, and red channels + constant alpha channel equal to FF */
    RS2_FORMAT_Y8, //< 8-bit per-pixel grayscale image */
    RS2_FORMAT_Y16, //< 16-bit per-pixel grayscale image */
    RS2_FORMAT_RAW10,
    //< Four 10 bits per pixel luminance values packed into a 5-byte macropixel */
    RS2_FORMAT_RAW16, //< 16-bit raw image */
    RS2_FORMAT_RAW8, //< 8-bit raw image */
    RS2_FORMAT_UYVY,
    //< Similar to the standard YUYV pixel format, but packed in a different order */
    RS2_FORMAT_MOTION_RAW, //< Raw data from the motion sensor */
    RS2_FORMAT_MOTION_XYZ32F,
    //< Motion data packed as 3 32-bit float values, for X, Y, and Z axis */
    RS2_FORMAT_GPIO_RAW,
    //< Raw data from the external sensors hooked to one of the GPIO's */
    RS2_FORMAT_6DOF,
    //< Pose data packed as floats array, containing translation vector, rotation quaternion and prediction velocities and accelerations vectors */
    RS2_FORMAT_DISPARITY32,
    //< 32-bit float-point disparity values. Depth->Disparity conversion : Disparity = Baseline*FocalLength/Depth */
    RS2_FORMAT_Y10BPACK,
    //< 16-bit per-pixel grayscale image unpacked from 10 bits per pixel packed ([8:8:8:8:2222]) grey-scale image. The data is unpacked to LSB and padded with 6 zero bits */
    RS2_FORMAT_DISTANCE, //< 32-bit float-point depth distance value.  */
    RS2_FORMAT_MJPEG,
    //< Bitstream encoding for video in which an image of each frame is encoded as JPEG-DIB   */
    RS2_FORMAT_Y8I,
    //< 8-bit per pixel interleaved. 8-bit left, 8-bit right.  */
    RS2_FORMAT_Y12I,
    //< 12-bit per pixel interleaved. 12-bit left, 12-bit right. Each pixel is stored in a 24-bit word in little-endian order. */
    RS2_FORMAT_INZI, //< multi-planar Depth 16bit + IR 10bit.  */
    RS2_FORMAT_INVI, //< 8-bit IR stream.  */
    RS2_FORMAT_W10,
    //< Grey-scale image as a bit-packed array. 4 pixel data stream taking 5 bytes */
    RS2_FORMAT_Z16H,
    //< Variable-length Huffman-compressed 16-bit depth values. */
    RS2_FORMAT_FG, //< 16-bit per-pixel frame grabber format. */
    RS2_FORMAT_Y411, //< 12-bit per-pixel. */
    RS2_FORMAT_COUNT
    //< Number of enumeration values. Not a valid input: intended to be used in for-loops. */
    );

  pRS2_format = Trs2_format;

function rs2_format_to_string(format: Trs2_format): PChar;
  cdecl; external REALSENSE_DLL;

{
  Distortion model: defines how pixel coordinates should be mapped to sensor coordinates. */
}
type
  Trs2_distortion = (
    RS2_DISTORTION_NONE,
    //< Rectilinear images. No distortion compensation required. */
    RS2_DISTORTION_MODIFIED_BROWN_CONRADY,
    //< Equivalent to Brown-Conrady distortion, except that tangential distortion is applied to radially distorted points */
    RS2_DISTORTION_INVERSE_BROWN_CONRADY,
    //< Equivalent to Brown-Conrady distortion, except undistorts image instead of distorting it */
    RS2_DISTORTION_FTHETA, //< F-Theta fish-eye distortion model */
    RS2_DISTORTION_BROWN_CONRADY,
    //< Unmodified Brown-Conrady distortion model */
    RS2_DISTORTION_KANNALA_BRANDT4,
    //< Four parameter Kannala Brandt distortion model */
    RS2_DISTORTION_COUNT
    //< Number of enumeration values. Not a valid input: intended to be used in for-loops. */
    );
  PRS2_distortion = ^Trs2_distortion;

function rs2_distortion_to_string(distortion: Trs2_distortion): PChar;
  cdecl; external REALSENSE_DLL;
{
  Video stream intrinsics.
}
type
  Trs2_intrinsics = record
    Width: integer;    //< Width of the image in pixels */
    Height: integer;   //< Height of the image in pixels */
    ppx: single;
    //< Horizontal coordinate of the principal point of the image, as a pixel offset from the left edge */
    ppy: single;
    //< Vertical coordinate of the principal point of the image, as a pixel offset from the top edge */
    fx: single;
    //< Focal length of the image plane, as a multiple of pixel width */
    fy: single;
    //< Focal length of the image plane, as a multiple of pixel height */
    model: Trs2_distortion;    //< Distortion model of the image */
    coeffs: array [0..5] of single;
    //< Distortion coefficients. Order for Brown-Conrady: [k1, k2, p1, p2, k3]. Order for F-Theta Fish-eye: [k1, k2, k3, k4, 0]. Other models are subject to their own interpretations */
  end;
  pRS2_intrinsics = ^Trs2_intrinsics;

type
  pRS2_frame_callback_ptr = ^Trs2_frame_callback_ptr;
  Trs2_frame_callback_ptr = procedure(frame: pRS2_frame; void: pvoid); cdecl;

type
  pRS2_devices_changed_callback = ^RS2_devices_changed_callback;

  RS2_devices_changed_callback = procedure(DeviceList1: pRS2_Device_List;
    DeviceList2: pRS2_Device_List); cdecl;


type
  prs2_update_progress_callback_ptr = ^rs2_update_progress_callback_ptr;
  rs2_update_progress_callback_ptr = procedure(const Value: single;
    aPointer: pVoid); cdecl;

type
  pRS2_notification_callback_ptr = ^rs2_notification_callback_ptr;
  rs2_notification_callback_ptr = procedure(rs2_notification: pRS2_notification;
    Data: pvoid);
{
  Exception types are the different categories of errors that RealSense API might return. */
}

type
  Trs2_exception_type = (
    RS2_EXCEPTION_TYPE_UNKNOWN,
    RS2_EXCEPTION_TYPE_CAMERA_DISCONNECTED,
    //< Device was disconnected, this can be caused by outside intervention, by internal firmware error or due to insufficient power
    RS2_EXCEPTION_TYPE_BACKEND,
    //< Error was returned from the underlying OS-specific layer
    RS2_EXCEPTION_TYPE_INVALID_VALUE,            //< Invalid value was passed to the API
    RS2_EXCEPTION_TYPE_WRONG_API_CALL_SEQUENCE,  //< Function precondition was violated
    RS2_EXCEPTION_TYPE_NOT_IMPLEMENTED,
    //< The method is not implemented at this point
    RS2_EXCEPTION_TYPE_DEVICE_IN_RECOVERY_MODE,
    //< Device is in recovery mode and might require firmware update
    RS2_EXCEPTION_TYPE_IO,                       //< IO Device failure
    RS2_EXCEPTION_TYPE_COUNT
    //< Number of enumeration values. Not a valid input: intended to be used in for-loops.
    );
{
 Motion device intrinsics: scale, bias, and variances. */
}

type
  Trs2_motion_device_intrinsic = record
    Data: array [0..2, 0..3] of single;  //< Interpret data array values
    noise_variances: array [0..2] of single; //< Variance of noise for X, Y, and Z axis
    bias_variances: array [0..2] of single;   //< Variance of bias for X, Y, and Z axis
  end;
  pRS2_motion_device_intrinsic = ^Trs2_motion_device_intrinsic;

{
  Video DSM (Digital Sync Module) parameters for calibration (same layout as in FW ac_depth_params)
  This is the block in MC that converts angles to dimensionless integers reported to MA (using "DSM coefficients")
}
type
  Trs2_dsm_params = packed record
    timestamp: QWord;
    //< system_clock::time_point::time_since_epoch().count()
    version: word;                     //< MAJOR<<12 | MINOR<<4 | PATCH
    model: byte;                        //< rs2_dsm_correction_model
    flags: array [0..4] of byte;   //< TBD, now 0s
    h_scale: single;
    //< the scale factor to horizontal DSM scale thermal results
    v_scale: single;
    //< the scale factor to vertical DSM scale thermal results
    h_offset: single;
    //< the offset to horizontal DSM offset thermal results
    v_offset: single;
    //< the offset to vertical DSM offset thermal results
    rtd_offset: single;
    //< the offset to the Round-Trip-Distance delay thermal results
    temp_x2: byte;
    //< the temperature recorded times 2 (ldd for depth; hum for rgb)
    mc_h_scale: single;
    //< the scale factor to horizontal LOS coefficients in MC
    mc_v_scale: single;
    //< the scale factor to vertical LOS coefficients in MC
    weeks_since_calibration: byte;
    //< time (in weeks) since factory calibration
    ac_weeks_since_calibaration: byte;
    //< time (in weeks) between factory calibration and last AC event
    reserved: byte;
  end;
  pRS2_dsm_params = ^Trs2_dsm_params;



{
 Streams are different types of data provided by RealSense devices.
}

type
  Trs2_stream = (
    RS2_STREAM_ANY,
    RS2_STREAM_DEPTH,
    //< Native stream of depth data produced by RealSense device */
    RS2_STREAM_COLOR,
    //< Native stream of color data captured by RealSense device */
    RS2_STREAM_INFRARED,
    //< Native stream of infrared data captured by RealSense device */
    RS2_STREAM_FISHEYE,
    //< Native stream of fish-eye (wide) data captured from the dedicate motion camera */
    RS2_STREAM_GYRO,
    //< Native stream of gyroscope motion data produced by RealSense device */
    RS2_STREAM_ACCEL,
    //< Native stream of accelerometer motion data produced by RealSense device */
    RS2_STREAM_GPIO,
    //< Signals from external device connected through GPIO */
    RS2_STREAM_POSE,
    //< 6 Degrees of Freedom pose data, calculated by RealSense device */
    RS2_STREAM_CONFIDENCE,
    //< 4 bit per-pixel depth confidence level */
    RS2_STREAM_COUNT);

  prs2_stream = ^Trs2_stream;

function rs2_stream_to_string(stream: Trs2_stream): PChar; cdecl; external REALSENSE_DLL;


{
Specifies types of different matchers
}
type
  Trs2_matchers = (
    RS2_MATCHER_DI,      //<compare depth and ir based on frame number

    RS2_MATCHER_DI_C,
    //<compare depth and ir based on frame number,compare the pair of corresponding depth and ir with color based on closest timestamp,commonly used by SR300

    RS2_MATCHER_DLR_C,
    //compare depth, left and right ir based on frame number,compare the set of corresponding depth, left and right with color based on closest timestamp,commonly used by RS415, RS435

    RS2_MATCHER_DLR,
    //compare depth, left and right ir based on frame number,commonly used by RS400, RS405, RS410, RS420, RS430

    RS2_MATCHER_DIC,     //compare depth, ir and confidence based on frame number used by RS500

    RS2_MATCHER_DIC_C,
    //compare depth, ir and confidence based on frame number,compare the set of corresponding depth, ir and confidence with color based on closest timestamp,commonly used by RS515

    RS2_MATCHER_DEFAULT,
    //the default matcher compare all the streams based on closest timestamp

    RS2_MATCHER_COUNT
    );

function rs2_matchers_to_string(stram: Trs2_matchers): PChar; cdecl;
  external REALSENSE_DLL;
{
 3D vector in Euclidean coordinate space
}
type
  Trs2_vector = record
    x: single;
    y: single;
    z: single;
  end;

{
Quaternion used to represent rotation
}

type
  Trs2_quaternion = record
    x: single;
    y: single;
    z: single;
    w: single;
  end;

{
 All the parameters required to define a pose stream.
}
type
  Trs2_pose_stream = record
    aType: Trs2_stream;
    index: integer;
    uid: integer;
    fps: integer;
    fmt: Trs2_format;
  end;
 {

 translation;          //< X, Y, Z values of translation, in meters (relative to initial position)                                    */
 velocity;             //< X, Y, Z values of velocity, in meters/sec                                                                  */
 acceleration;         //< X, Y, Z values of acceleration, in meters/sec^2                                                            */
 rotation;             //< Qi, Qj, Qk, Qr components of rotation as represented in quaternion rotation (relative to initial position) */
 angular_velocity;     //< X, Y, Z values of angular velocity, in radians/sec                                                         */
 angular_acceleration; //< X, Y, Z values of angular acceleration, in radians/sec^2                                                   */
 tracker_confidence;   //< Pose confidence 0x0 - Failed, 0x1 - Low, 0x2 - Medium, 0x3 - High                                          */
 mapper_confidence;    //< Pose map confidence 0x0 - Failed, 0x1 - Low, 0x2 - Medium, 0x3 - High                                      */

}

type
  Trs2_pose = record
    translation: Trs2_vector;
    velocity: Trs2_vector;
    acceleration: Trs2_vector;
    rotation: Trs2_quaternion;
    angular_velocity: Trs2_vector;
    tracker_confidence: integer; //??    unsigned int
    mapper_confidence: integer;  //??    unsigned int
  end;
  pRS2_pose = ^Trs2_pose;

{
brief Severity of the librealsense logger.
}
type
  Trs2_log_severity = (
    RS2_LOG_SEVERITY_DEBUG,  //< Detailed information about ordinary operations
    RS2_LOG_SEVERITY_INFO, //< Terse information about ordinary operations
    RS2_LOG_SEVERITY_WARN, //< Indication of possible failure
    RS2_LOG_SEVERITY_ERROR, //< Indication of definite failure
    RS2_LOG_SEVERITY_FATAL, //< Indication of unrecoverable failure
    RS2_LOG_SEVERITY_NONE, //< No logging will occur
    RS2_LOG_SEVERITY_COUNT,
    //< Number of enumeration values. Not a valid input: intended to be used in for-loops.
    RS2_LOG_SEVERITY_ALL = RS2_LOG_SEVERITY_DEBUG   //**< Include any/all log messages
    );

function rs2_log_severity_to_string(info: Trs2_log_severity): PChar; cdecl; external;

{
  3D coordinates with origin at topmost left corner of the lense,
  with positive Z pointing away from the camera, positive X pointing camera right and positive Y pointing camera down
}
type
  Trs2_vertex = record
    xyx: array [0..2] of single;
  end;
  pRS2_vertex = ^Trs2_vertex;

{
  Pixel location within 2D image. (0,0) is the topmost, left corner. Positive X is right, positive Y is down *
}
type
  Trs2_pixel = record
    ij: array [0..1] of integer;
  end;
  pRS2_pixel = ^Trs2_pixel;

{
typedef void (*rs2_log_callback_ptr)(rs2_log_severity, rs2_log_message const *, void * arg);
}
type
  pRS2_log_callback_ptr = ^rs2_log_callback_ptr;

  rs2_log_callback_ptr = procedure(log_severity: Trs2_log_severity;
    const log_message: PChar; arg: pvoid);


function rs2_get_error_message(error: prs2_error): PChar;
  cdecl; external REALSENSE_DLL;
function rs2_get_failed_function(Error: pRS2_error): PChar;
  cdecl; external REALSENSE_DLL;
function rs2_get_failed_args(Error: pRS2_error): PChar; cdecl
  external REALSENSE_DLL;
procedure rs2_free_error(error: prs2_error); cdecl;
  external REALSENSE_DLL;
function rs2_exception_type_to_string(TypeException: prs2_exception_type): PChar;
  cdecl; external REALSENSE_DLL;



implementation


end.
