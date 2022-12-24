{
 Exposes RealSense internal functionality
}
unit rs_internal;

{$mode ObjFPC}{$H+}

interface

uses
  rs_types,
  rs_context,
  rs_sensor,
  rs_frame,
  rs_option;

{
 Firmware size constants
}
const
  signed_fw_size: integer = $18031C;
  signed_sr300_size: integer = $0C025C;
  unsigned_fw_size: integer = $200000;
  unsigned_sr300_size: integer = $100000;
{
  librealsense Recorder is intended for effective unit-testing
  Currently supports three modes of operation:
}
type
  Trs2_recording_mode = (
    RS2_RECORDING_MODE_BLANK_FRAMES,
    //< frame metadata will be recorded, but pixel data will be replaced with zeros to save space
    RS2_RECORDING_MODE_COMPRESSED,
    //< frames will be encoded using a proprietary lossy encoding, aiming at x5 compression at some CPU expense
    RS2_RECORDING_MODE_BEST_QUALITY,
    //< frames will not be compressed, but rather stored as-is. This gives best quality and low CPU overhead, but you might run out of memory
    RS2_RECORDING_MODE_COUNT);
{
All the parameters required to define a video stream.
}
type
  Trs2_video_stream = record
    aType: Trs2_stream;
    index: integer;
    uid: integer;
    Width: integer;
    Height: integer;
    fps: integer;
    bpp: integer;
    fmt: Trs2_format;
    intrinsics: Trs2_intrinsics;
  end;
{
 All the parameters required to define a motion stream.
}
type
  Trs2_motion_stream = record
    aType: Trs2_stream;
    index: integer;
    uid: integer;
    fps: integer;
    fmt: Trs2_format;
    intrinsics: Trs2_motion_device_intrinsic;
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

type
  TDeleterProc = procedure(arg: Pointer); cdecl;
  { #todo : Pendiente probar
Preguntado en el foro:
https://forum.lazarus.freepascal.org/index.php/topic,61311.0.html?PHPSESSID=tajqr2m9bi89t347sl80bapj70 }
{
 All the parameters required to define a video frame.
}
type
  Trs2_software_video_frame = record
    pixels: pvoid;
    deleter: TDeleterProc;
    stride: integer;
    timestamp: Trs2_time_t;
    domain: Trs2_timestamp_domain;
    frame_number: integer;
    profile: pRS2_stream_profile;
    depth_units: single;
  end;
{
  brief All the parameters required to define a motion frame.
}
type
  Trs2_software_motion_frame = record
    Data: pvoid;
    deleter: TDeleterProc;
    timestamp: Trs2_time_t;
    domain: Trs2_timestamp_domain;
    frame: integer;
    profile: pRS2_stream_profile;
  end;

type
  Tpose_frame_info = record
    Translation: array [0..2] of single;
    velocity: array [0..2] of single;
    acceleration: array [0..2] of single;
    rotation: array [0..3] of single;
    angular_velocity: array [0..2] of single;
    angular_acceleration: array [0..2] of single;
    tracker_confidence: integer;
    mapper_confidence: integer;
  end;
{
  All the parameters required to define a pose frame.
}

type
  Trs2_software_pose_frame = record
    pose_frame_info: Tpose_frame_info;
    Data: pvoid;
    timestamp: Trs2_time_t;
    frame_number: integer;
    profile: pRS2_stream_profile;
  end;
{
  All the parameters required to define a sensor notification. */
}
type
  Trs2_software_notification = record
    category: Trs2_notification_category;
    aType: integer;
    severity: Trs2_log_severity;
    descripcion: PChar;
    serialize_data: PChar;
  end;

type
  Trs2_software_device_destruction_callback = record
  end;

{
  Create librealsense context that will try to record all operations over librealsense into a file
  param[in] api_version realsense API version as provided by RS2_API_VERSION macro
  param[in] filename string representing the name of the file to record
  param[in] section  string representing the name of the section within existing recording
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return            context object, should be released by rs2_delete_context
}
function rs2_create_recording_context(api_version: integer;
  const filename: PChar; const seccion: PChar; mode: Trs2_recording_mode;
  error: pRS2_error): pRS2_context; cdecl; external REALSENSE_DLL;
{
  Create librealsense context that given a file will respond to calls exactly as the recording did
  if the user calls a method that was either not called during recording or violates causality of the recording error will be thrown
  param[in] api_version realsense API version as provided by RS2_API_VERSION macro
  param[in] filename string representing the name of the file to play back from
  param[in] section  string representing the name of the section within existing recording
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return            context object, should be released by rs2_delete_context
}
function rs2_create_mock_context(api_version: integer; filename: PChar;
  section: PChar; error: pRS2_error): pRS2_context; cdecl; external REALSENSE_DLL;
{
  Create librealsense context that given a file will respond to calls exactly as the recording did
  if the user calls a method that was either not called during recording or violates causality of the recording error will be thrown
  param[in] api_version realsense API version as provided by RS2_API_VERSION macro
  param[in] filename string representing the name of the file to play back from
  param[in] section  string representing the name of the section within existing recording
  param[in] min_api_version reject any file that was recorded before this version
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return            context object, should be released by rs2_delete_context
}
function rs2_create_mock_context_versioned(api_version: integer;
  filename: PChar; secction: PChar; min_api_version: PChar;
  error: pRS2_error): pRS2_context; cdecl; external REALSENSE_DLL;
{
  Create software device to enable use librealsense logic without getting data from backend
  but inject the data from outside
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return            software device object, should be released by rs2_delete_device
}
function rs2_create_software_device(error: pRS2_error): pRS2_device; cdecl; external;
{
  Add sensor to the software device
  param[in] dev the software device
  param[in] sensor_name the name of the sensor
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return            software sensor object, should be released by rs2_delete_sensor
}
function rs2_software_device_add_sensor(dev: pRS2_device; sensor_name: PChar;
  error: pRS2_error): pRS2_sensor; cdecl; external REALSENSE_DLL;
{
  Inject video frame to software sonsor
  param[in] sensor the software sensor
  param[in] frame all the frame components
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}

procedure rs2_software_sensor_on_video_frame(sensor: pRS2_sensor;
  frame: Trs2_software_video_frame; error: pRS2_error); cdecl; external REALSENSE_DLL;
{
  Inject motion frame to software sonsor
  param[in] sensor the software sensor
  param[in] frame all the frame components
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_software_sensor_on_motion_frame(sensor: pRS2_sensor;
  frame: Trs2_software_motion_frame; error: pRS2_error); cdecl; external REALSENSE_DLL;
{
  Inject pose frame to software sonsor
  param[in] sensor the software sensor
  param[in] frame all the frame components
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_software_sensor_on_pose_frame(sensor: pRS2_sensor;
  frame: Trs2_software_motion_frame; error: pRS2_error); cdecl; external REALSENSE_DLL;
{
  Inject notification to software sonsor
  param[in] sensor the software sensor
  param[in] notif all the notification components
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_software_sensor_on_notification(sensor: pRS2_sensor;
  notif: Trs2_software_notification; error: pRS2_error); cdecl; external REALSENSE_DLL;
{
  Set frame metadata for the upcoming frames
  param[in] sensor the software sensor
  param[in] value metadata key to set
  param[in] type metadata value
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_software_sensor_set_metadata(sensor: pRS2_sensor;
  Value: Trs2_frame_metadata_value; aType: Trs2_metadata_type; error: pRS2_error);
  cdecl; external REALSENSE_DLL;
{
  set callback to be notified when a specific software device is destroyed
  param[in] dev             software device
  param[in] on_notification function pointer to register as callback
  param[out] error          if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_software_device_set_destruction_callback(dev: pRS2_device;
  on_notification: Trs2_software_device_destruction_callback;
  user: pvoid; error: pRS2_error); cdecl; external REALSENSE_DLL;
{
  Set the wanted matcher type that will be used by the syncer
  param[in] dev the software device
  param[in] matcher matcher type
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_software_device_create_matcher(dev: pRS2_device;
  matcher: Trs2_matchers; error: pRS2_error); cdecl; external REALSENSE_DLL;
{
  Register a camera info value for the software device
  param[in] dev the software device
  param[in] info identifier for the camera info to add.
  param[in] val string value for this new camera info.
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_software_device_register_info(dev: pRS2_device;
  info: Trs2_camera_info; val: PChar; error: pRS2_error); cdecl; external REALSENSE_DLL;
{
  Update an existing camera info value for the software device
  param[in] dev the software device
  param[in] info identifier for the camera info to add.
  param[in] val string value for this new camera info.
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_software_device_update_info(dev: pRS2_device;
  info: Trs2_camera_info; val: PChar; error: pRS2_error); cdecl; external REALSENSE_DLL;
{
  Add video stream to sensor
  param[in] sensor the software sensor
  param[in] video_stream all the stream components
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
function rs2_software_sensor_add_video_stream(sendor: pRS2_sensor;
  video_stream: Trs2_video_stream; error: pRS2_error): pRS2_stream_profile;
  cdecl; external REALSENSE_DLL;
{
  Add video stream to sensor
  param[in] sensor the software sensor
  param[in] video_stream all the stream components
  param[in] is_default whether or not the stream should be a default stream for the device
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
function rs2_software_sensor_add_video_stream_ex(sensor: pRS2_sensor;
  video_stream: Trs2_video_stream; is_default: integer;
  error: pRS2_error): pRS2_stream_profile; cdecl; external REALSENSE_DLL;
{
 Add motion stream to sensor
 param[in] sensor the software sensor
 param[in] motion_stream all the stream components
 param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
function rs2_software_sensor_add_motion_stream(sensor: pRS2_sensor;
  motion_stream: Trs2_motion_stream; error: pRS2_error): pRS2_stream_profile;
  cdecl; external;
{
  Add motion stream to sensor
  param[in] sensor the software sensor
  param[in] motion_stream all the stream components
  param[in] is_default whether or not the stream should be a default stream for the device
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
function rs2_software_sensor_add_motion_stream(sensor: pRS2_sensor;
  motion_stream: Trs2_motion_stream; is_default: integer;
  error: pRS2_error): pRS2_stream_profile;
  cdecl; external;
{
  Add pose stream to sensor
  param[in] sensor the software sensor
  param[in] pose_stream all the stream components
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
function rs2_software_sensor_add_pose_stream(sensor: pRS2_sensor;
  pose_stream: Trs2_pose_stream; error: pRS2_error): pRS2_stream_profile;
  cdecl; external REALSENSE_DLL;
{
  Add pose stream to sensor
  param[in] sensor the software sensor
  param[in] pose_stream all the stream components
  param[in] is_default whether or not the stream should be a default stream for the device
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
function rs2_software_sensor_add_pose_stream(sensor: pRS2_sensor;
  pose_stream: Trs2_pose_stream; is_defautl: integer;
  error: pRS2_error): pRS2_stream_profile;
  cdecl; external REALSENSE_DLL;
{
  Add read only option to sensor
  param[in] sensor the software sensor
  param[in] option the wanted option
  param[in] val the initial value
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_software_sensor_add_read_only_option(sensor: pRS2_sensor;
  option: Trs2_option; val: single; error: pRS2_error); cdecl; external REALSENSE_DLL;
{
  Update the read only option added to sensor
  param[in] sensor the software sensor
  param[in] option the wanted option
  param[in] val the wanted value
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_software_sensor_update_read_only_option(sensor: pRS2_sensor;
  option: Trs2_option; val: single; error: pRS2_error); cdecl; external REALSENSE_DLL;

{
  Add an option to sensor
  param[in] sensor        the software sensor
  param[in] option        the wanted option
  param[in] min           the minimum value which will be accepted for this option
  param[in] max           the maximum value which will be accepted for this option
  param[in] step          the granularity of options which accept discrete values, or zero if the option accepts continuous values
  param[in] def           the initial value of the option
  param[in] is_writable   should the option be read-only or not
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_software_sensor_add_option(sensor: pRS2_sensor;
  option: Trs2_option; min: single; max: single; step: single; def: single;
  is_writable: integer; error: pRS2_error); cdecl; external REALSENSE_DLL;
{
  Sensors hold the parent device in scope via a shared_ptr. This function detaches that so that the software sensor doesn't keep the software device alive.
  Note that this is dangerous as it opens the door to accessing freed memory if care isn't taken.
  param[in] sensor         the software sensor
  param[out] error         if non-null, recieves any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_software_sensor_detach(sensor: pRS2_sensor; error: pRS2_sensor);
  cdecl; external REALSENSE_DLL;
{
  Creates RealSense firmware log message.
  param[in] dev            Device from which the FW log will be taken using the created message
  param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
  return                   pointer to created empty firmware log message
}
function rs2_create_fw_log_message(dev: pRS2_device;
  error: pRS2_error): pRS2_firmware_log_message; cdecl; external REALSENSE_DLL;
{
  Gets RealSense firmware log.
  param[in] dev            Device from which the FW log should be taken
  param[in] fw_log_msg     Firmware log message object to be filled
  param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
  return                   true for success, false for failure - failure happens if no firmware log was sent by the hardware monitor
}
function rs2_get_fw_log(dev: pRS2_device; fw_log_msg: pRS2_firmware_log_message;
  error: pRS2_error): integer; cdecl; external REALSENSE_DLL;
{
  Gets RealSense flash log - this is a fw log that has been written in the device during the previous shutdown of the device
  param[in] dev            Device from which the FW log should be taken
  param[in] fw_log_msg     Firmware log message object to be filled
  param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
  return                   true for success, false for failure - failure happens if no firmware log was sent by the hardware monitor
}
function rs2_get_flash_log(dev: pRS2_sensor; fw_log_msg: pRS2_firmware_log_message;
  error: pRS2_error): integer; cdecl; external REALSENSE_DLL;
{
  Delete RealSense firmware log message
  param[in]  device    Realsense firmware log message to delete
}
procedure rs2_delete_fw_log_message(msg: pRS2_firmware_log_message);
  cdecl; external REALSENSE_DLL;
{
  Gets RealSense firmware log message data.
  param[in] msg        firmware log message object
  param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
  return               pointer to start of the firmware log message data
}
function rs2_fw_log_message_data(msg: pRS2_firmware_log_message;
  error: pRS2_error): pbyte;
  cdecl; external REALSENSE_DLL;
{
  Gets RealSense firmware log message size.
  param[in] msg        firmware log message object
  param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
  return               size of the firmware log message data
}
function rs2_fw_log_message_size(msg: pRS2_firmware_log_message;
  error: pRS2_error): integer; cdecl; external REALSENSE_DLL;
{
  Gets RealSense firmware log message timestamp.
  param[in] msg        firmware log message object
  param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
  return               timestamp of the firmware log message
}
function rs2_fw_log_message_timestamp(msg: pRS2_firmware_log_message;
  error: pRS2_error): cardinal; cdecl; external REALSENSE_DLL;
{
  Gets RealSense firmware log message severity.
  param[in] msg        firmware log message object
  param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
  return               severity of the firmware log message data
}
function rs2_fw_log_message_severity(msg: pRS2_firmware_log_message;
  error: pRS2_error): Trs2_log_severity; cdecl; external REALSENSE_DLL;
{
  Initializes RealSense firmware logs parser in device.
  param[in] dev            Device from which the FW log will be taken
  param[in] xml_content    content of the xml file needed for parsing
  param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
  return                   true for success, false for failure - failure happens if opening the xml from the xml_path input fails
}
function rs2_init_fw_log_parser(dev: pRS2_device; xml_content: PChar;
  error: pRS2_error): integer; cdecl; external REALSENSE_DLL;
{
  Creates RealSense firmware log parsed message.
  param[in] dev            Device from which the FW log will be taken using the created message
  param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
  return                   pointer to created empty firmware log message
}
function rs2_create_fw_log_parsed_message(dev: pRS2_device;
  error: pRS2_device): pRS2_firmware_log_parsed_message; cdecl; external REALSENSE_DLL;
{
  Gets RealSense firmware log parser
  param[in] dev                Device from which the FW log will be taken
  param[in] fw_log_msg         firmware log message to be parsed
  param[in] parsed_msg         firmware log parsed message - place holder for the resulting parsed message
  param[out] error             If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
  return                       true for success, false for failure - failure happens if message could not be parsed
}
function rs2_parse_firmware_log(dev: pRS2_device; fw_log_msg: pRS2_firmware_log_message;
  parsed_msg: pRS2_firmware_log_parsed_message; error: pRS2_error): integer;
  cdecl; external REALSENSE_DLL;
{
  brief Returns number of fw logs already polled from device but not by user yet
  param[in] dev                Device from which the FW log will be taken
  param[out] error             If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
  return                       number of fw logs already polled from device but not by user yet
}
function rs2_get_number_of_fw_logs(dev: pRS2_device; error: pRS2_error): cardinal;
  cdecl; external REALSENSE_DLL;
{
  Gets RealSense firmware log parsed message.
  param[in] fw_log_parsed_msg      firmware log parsed message object
  param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
  return               message of the firmware log parsed message
}
function rs2_get_fw_log_parsed_message(
  fw_log_parsed_msg: pRS2_firmware_log_parsed_message; error: pRS2_error): PChar;
  cdecl; external REALSENSE_DLL;
{
  Gets RealSense firmware log parsed message file name.
  param[in] fw_log_parsed_msg      firmware log parsed message object
  param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
  return               file name of the firmware log parsed message
}
function rs2_get_fw_log_parsed_file_name(fw_log_parsed_msg:
  pRS2_firmware_log_parsed_message; error: pRS2_error): PChar;
  cdecl; external REALSENSE_DLL;
{
  Gets RealSense firmware log parsed message thread name.
  param[in] fw_log_parsed_msg      firmware log parsed message object
  param[out] error                 If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
  return                           thread name of the firmware log parsed message
}
function rs2_get_fw_log_parsed_thread_name(fw_log_parsed_msg:
  pRS2_firmware_log_parsed_message; error: pRS2_error): PChar;
  cdecl; external REALSENSE_DLL;
{
  Gets RealSense firmware log parsed message severity.
  param[in] fw_log_parsed_msg      firmware log parsed message object
  param[out] error                 If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
  return                           severity of the firmware log parsed message
}
function rs2_get_fw_log_parsed_severity(fw_log_parsed_msg:
  pRS2_firmware_log_parsed_message; error: pRS2_error): Trs2_log_severity;
  cdecl; external REALSENSE_DLL;
{
  Gets RealSense firmware log parsed message relevant line (in the file that is returned by rs2_get_fw_log_parsed_file_name).
  param[in] fw_log_parsed_msg      firmware log parsed message object
  param[out] error                 If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
  return                           line number of the firmware log parsed message
}
function rs2_get_fw_log_parsed_line(fw_log_parsed_msg: pRS2_firmware_log_parsed_message;
  error: pRS2_error): integer; cdecl; external REALSENSE_DLL;
{
  Gets RealSense firmware log parsed message timestamp
  param[in] fw_log_parsed_msg      firmware log parsed message object
  param[out] error                 If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
  return                           timestamp of the firmware log parsed message
}
function rs2_get_fw_log_parsed_timestamp(fw_log_parsed_msg:
  pRS2_firmware_log_parsed_message; error: pRS2_error): cardinal;
  cdecl; external REALSENSE_DLL;
{
  Gets RealSense firmware log parsed message sequence id - cyclic number of FW log with [0..15] range
  param[in] fw_log_parsed_msg      firmware log parsed message object
  param[out] error                 If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
  return                           sequence of the firmware log parsed message
}
function rs2_get_fw_log_parsed_sequence_id(
  fw_log_parsed_msg: pRS2_firmware_log_parsed_message; error: pRS2_error): cardinal;
  cdecl; external REALSENSE_DLL;
{
  Creates RealSense terminal parser.
  param[in] xml_content    content of the xml file needed for parsing
  param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
  return                   pointer to created terminal parser object
}
function rs2_create_terminal_parser(xml_content: PChar;
  error: pRS2_error): pRS2_terminal_parser; cdecl; external REALSENSE_DLL;
{
  Deletes RealSense terminal parser.
  param[in] terminal_parser            terminal parser to be deleted
}
procedure rs2_delete_terminal_parser(terminal_parser: pRS2_terminal_parser);
  cdecl; external REALSENSE_DLL;
{
  Parses terminal command via RealSense terminal parser
  param[in] terminal_parser        Terminal parser object
  param[in] command                command to be sent to the hw monitor of the device
  param[in] size_of_command        size of command to be sent to the hw monitor of the device
  param[out] error                 If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
  return                           command to hw monitor, in hex
}
function rs2_terminal_parse_command(terminal_parse: pRS2_terminal_parser;
  command: PChar; size_of_command: cardinal; error: pRS2_error): pRS2_raw_data_buffer;
  cdecl; external REALSENSE_DLL;

{
  Parses terminal response via RealSense terminal parser
  param[in] terminal_parser        Terminal parser object
  param[in] command                command sent to the hw monitor of the device
  param[in] size_of_command        size of the command to sent to the hw monitor of the device
  param[in] response               response received by the hw monitor of the device
  param[in] size_of_response       size of the response received by the hw monitor of the device
  param[out] error                 If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
  return                           answer parsed
}

function pRS2_terminal_parser(terminal_parse: pRS2_terminal_parser;
  command: PChar; size_of_commando: cardinal; response: pvoid;
  size_of_response: cardinal; error: pRS2_error): pRS2_raw_data_buffer;
  cdecl; external REALSENSE_DLL;

implementation


end.
