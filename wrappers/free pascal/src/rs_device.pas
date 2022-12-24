{
Exposes RealSense device functionality
}
unit rs_device;

{$mode ObjFPC}{$H+}
{$WARN 2005 off : Comment level $1 found}
interface

uses rs_sensor, rs_types;

const
  RS2_UNSIGNED_UPDATE_MODE_UPDATE = 0;
  RS2_UNSIGNED_UPDATE_MODE_READ_ONLY = 1;
  RS2_UNSIGNED_UPDATE_MODE_FULL = 2;

{
Determines number of devices in a list.
param[in]  info_list The list of connected devices captured using rs2_query_devices
param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
return               Device count
}
function rs2_get_device_count(Info_List: Prs2_device_list;
  var Error: prs2_error): integer;
  cdecl; external REALSENSE_DLL;
{
Deletes device list, any devices created using this list will remain unaffected.
param[in]  info_list List to delete
}

procedure rs2_delete_device_list(info_list: prs2_device_list); cdecl;
  external REALSENSE_DLL;

{

Checks if a specific device is contained inside a device list.
param[in]  info_list The list of devices to check in
param[in]  device    RealSense device to check for
param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
return               True if the device is in the list and false otherwise

}

function rs2_device_list_contains(const info_list: pRS2_device_list;
  const device: pRS2_device; error: pRS2_error): integer; cdecl; external REALSENSE_DLL;

{
 Creates a device by index. The device object represents a physical camera and provides the means to manipulate it.
param[in]  info_list the list containing the device to retrieve
param[in]  index     The zero based index of device to retrieve
param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
return               The requested device, should be released by rs2_delete_device
}

function rs2_create_device(Info_List: Prs2_device_list; Index: integer;
  var Error: prs2_error): prs2_device; cdecl; external REALSENSE_DLL;

{
Delete RealSense device
param[in]  device  Realsense device to delete
}

procedure rs2_delete_device(Device: prs2_device); cdecl; external REALSENSE_DLL;

{
 Retrieve camera specific information, like versions of various internal components.
param[in]  device    The RealSense device
param[in]  info      Camera info type to retrieve
param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
return               The requested camera info string, in a format specific to the device model
}

function rs2_get_device_info(Device: prs2_device; Info: Trs2_camera_info;
  var Error: prs2_error): PChar; cdecl; external REALSENSE_DLL;

{


 Check if a camera supports a specific camera info type.
param[in]  device    The RealSense device to check
param[in]  info      The parameter to check for support
param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
return               True if the parameter both exist and well-defined for the specific device

}

function rs2_supports_device_info(const device: pRS2_device;
  info: Trs2_camera_info; error: pRS2_error): integer; cdecl; external REALSENSE_DLL;
{
  Send hardware reset request to the device. The actual reset is asynchronous.
  Note: Invalidates all handles to this device.
  param[in]  device   The RealSense device to reset
  param[out] error    If non-null, receives any error that occurs during this call, otherwise, errors are ignored
}

procedure rs2_hardware_reset(const device: pRS2_device; error: pRS2_error);
  cdecl; external REALSENSE_DLL;
{

 Build debug_protocol raw data command from opcode, parameters and data.
 The result can be used as raw_data_to_send parameter in send_and_receive_raw_data
 param[in]  device                    RealSense device to send data to
 param[in]  opcode                    Commad opcode
 param[in]  param1                    First input parameter
 param[in]  param2                    Second parameter
 param[in]  param3                    Third parameter
 param[in]  param4                    Fourth parameter
 param[in]  data                      Input Data (up to 1024 bytes)
 param[in]  size_of_data              Size of input data in bytes
 param[out] error                     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
 return                               rs2_raw_data_buffer which includes raw command
Note:
This the original declaration.
const rs2_raw_data_buffer* rs2_build_debug_protocol_command(rs2_device* device, unsigned opcode, unsigned param1, unsigned param2,
    unsigned param3, unsigned param4, void* data, unsigned size_of_data, rs2_error** error);
You can see that use unsigned. I'm not sure which  is the equivalent in Pascal, so I used NavtiveUint;
    }
function rs2_build_debug_protocol_command(device: pRS2_device;
  opcode: nativeuint; param1: nativeuint; param2: nativeuint;
  param3: nativeuint; param4: nativeuint; Data: pvoid; size_of_data: nativeuint;
  error: pRS2_error): prs2_raw_data_buffer; cdecl; external REALSENSE_DLL;
{
 Send raw data to device
 param[in]  device                    RealSense device to send data to
 param[in]  raw_data_to_send          Raw data to be sent to device
 param[in]  size_of_raw_data_to_send  Size of raw_data_to_send in bytes
 param[out] error                     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
return                               Device's response in a rs2_raw_data_buffer, which should be released by rs2_delete_raw_data

Note:
This the original declaration.
const rs2_raw_data_buffer* rs2_send_and_receive_raw_data(rs2_device* device, void* raw_data_to_send, unsigned size_of_raw_data_to_send, rs2_error** error);
You can see that use unsigned. I'm not sure which  is the equivalent in Pascal, so I used NavtiveUint;
}
function rs2_send_and_receive_raw_data(device: pRS2_device;
  raw_data_to_send: pvoid; size_of_raw_data_to_send: nativeuint;
  error: pRS2_error): prs2_raw_data_buffer; cdecl; external REALSENSE_DLL;
{
 Test if the given device can be extended to the requested extension.
 param[in]  device    Realsense device
 param[in]  extension The extension to which the device should be tested if it is extendable
 param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
 return               Non-zero value iff the device can be extended to the given extension
}
function rs2_is_device_extendable_to(const device: pRS2_device;
  extension: Trs2_extension; error: pRS2_error): integer; cdecl; external REALSENSE_DLL;
{

 Create a static snapshot of all connected sensors within a specific device.
param[in]  device    Specific RealSense device
param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
return               The list of sensors, should be released by rs2_delete_sensor_list

}
function rs2_query_sensors(const device: pRS2_device;
  error: pRS2_error): prs2_sensor_list;
  cdecl; external REALSENSE_DLL;
{

 Enter the given device into loopback operation mode that uses the given file as input for raw data
 param[in]  device     Device to enter into loopback operation mode
 param[in]  from_file  Path to bag file with raw data for loopback
 param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored
}

procedure rs2_loopback_enable(const device: pRS2_device; from_file: PChar;
  error: pRS2_error); cdecl; external REALSENSE_DLL;
{
 Restores the given device into normal operation mode
 param[in]  device     Device to restore to normal operation mode
 param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored
}

procedure rs2_loopback_disable(const device: pRS2_device; error: pRS2_error);
  cdecl; external REALSENSE_DLL;
{
 Checks if the device is in loopback mode or not
 param[in]  device     Device to check for operation mode
 param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored
 return true if the device is in loopback operation mode
}
function rs2_loopback_is_enabled(const device: pRS2_device; error: pRS2_error): integer;
  cdecl; external REALSENSE_DLL;
{

 Connects to a given tm2 controller
 param[in]  device     Device to connect to the controller
 param[in]  mac_addr   The MAC address of the desired controller
 param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored

}
procedure rs2_connect_tm2_controller(const device: pRS2_device;
  const mac_addr: PChar; error: pRS2_error); cdecl; external REALSENSE_DLL;
{

 Disconnects a given tm2 controller
 param[in]  device     Device to disconnect the controller from
 param[in]  id         The ID of the desired controller
 param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored

}
procedure rs2_disconnect_tm2_controller(const device: pRS2_device;
  id: integer; error: pRS2_error); cdecl; external REALSENSE_DLL;
{

 Reset device to factory calibration
 param[in] device       The RealSense device
 param[out] error       If non-null, receives any error that occurs during this call, otherwise, errors are ignored

}

procedure rs2_reset_to_factory_calibration(const device: pRS2_device;
  error: pRS2_error); cdecl; external REALSENSE_DLL;
{

Write calibration to device's EEPROM
param[in] device       The RealSense device
param[out] error       If non-null, receives any error that occurs during this call, otherwise, errors are ignored

}

procedure rs2_write_calibration(const device: pRS2_device; error: pRS2_error);
  cdecl; external REALSENSE_DLL;

{
 Update device to the provided firmware, the device must be extendable to RS2_EXTENSION_UPDATABLE.
 This call is executed on the caller's thread and it supports progress notifications via the optional callback.
 param[in]  device        Device to update
 param[in]  fw_image      Firmware image buffer
 param[in]  fw_image_size Firmware image buffer size
 param[in]  callback      Optional callback for update progress notifications, the progress value is normailzed to 1
 param[in]  client_data   Optional client data for the callback
}
procedure rs2_update_firmware(const device: pRS2_device; fw_image: pvoid;
  fw_image_size: integer; callback: prs2_update_progress_callback_ptr;
  client_data: pvoid; error: pRS2_error); cdecl; external REALSENSE_DLL;

{

Create backup of camera flash memory. Such backup does not constitute valid firmware image, and cannot be
loaded back to the device, but it does contain all calibration and device information.
param[in]  device        Device to update
param[in]  callback      Optional callback for update progress notifications, the progress value is normailzed to 1
param[in]  client_data   Optional client data for the callback
param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored

const rs2_raw_data_buffer* rs2_create_flash_backup(const rs2_device* device, rs2_update_progress_callback_ptr callback, void* client_data, rs2_error** error);
}

function rs2_create_flash_backup(device: pRS2_device;
  callback: prs2_update_progress_callback_ptr; client_data: pvoid;
  error: pRS2_error): prs2_raw_data_buffer; cdecl; external REALSENSE_DLL;
{
 Checks if the device and the provided firmware image are compatible
param[in]  device        Device to update
param[in]  fw_image      Firmware image buffer
param[in]  fw_image_size Firmware image buffer size in bytes
param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored
return                   Non-zero if the firmware is compatible with the device and 0 otherwise
int rs2_check_firmware_compatibility(const rs2_device* device, const void* fw_image, int fw_image_size, rs2_error** error);
}

function rs2_check_firmware_compatibility(device: pRS2_device;
  fw_image: pvoid; fw_image_size: integer; error: pRS2_error): integer; cdecl; external;
{

 Update device to the provided firmware by writing raw data directly to the flash, this command can be executed only on unlocked camera.
 The device must be extendable to RS2_EXTENSION_UPDATABLE.
 This call is executed on the caller's thread and it supports progress notifications via the optional callback.
 param[in]  device        Device to update
 param[in]  fw_image      Firmware image buffer
 param[in]  fw_image_size Firmware image buffer size
 param[in]  callback      Optional callback for update progress notifications, the progress value is normailzed to 1
 param[in]  client_data   Optional client data for the callback
 param[in]  update_mode   Select one of RS2_UNSIGNED_UPDATE_MODE, WARNING!!! setting to any option other than RS2_UNSIGNED_UPDATE_MODE_UPDATE will make this call unsafe and might damage the camera
 param[out] error         If non-null, receives any error that occurs during this call, otherwise, errors are ignored

void rs2_update_firmware_unsigned(const rs2_device* device, const void* fw_image, int fw_image_size, rs2_update_progress_callback_ptr callback, void* client_data, int update_mode, rs2_error** error);
}

procedure rs2_update_firmware_unsigned(const device: prs2_device;
  const fw_image: pvoid; fw_image_size: integer;
  callback: rs2_update_progress_callback_ptr; client_data: pvoid;
  update_mode: integer; error: pRS2_error); cdecl; external REALSENSE_DLL;

{
Enter the device to update state, this will cause the updatable device to disconnect and reconnect as update device.
param[in]  device     Device to update
param[out] error      If non-null, receives any error that occurs during this call, otherwise, errors are ignored

}
procedure rs2_enter_update_state(const device: pRS2_device; error: pRS2_error);
  cdecl; external REALSENSE_DLL;

{

This will improve the depth noise.
param[in] json_content       Json string to configure regular speed on chip calibration parameters:
                                     (

                                      "calib type" : 0,
                                      "speed": 3,
                                      "scan parameter": 0,
                                      "adjust both sides": 0,
                                      "white wall mode": 0

                                    )
calib_type - calibraton type: 0 = regular, 1 = focal length, 2 = both regular and focal length in order
                                    speed - for regular calibration, value can be one of: Very fast = 0, Fast = 1, Medium = 2, Slow = 3, White wall = 4, default is Slow for type 0 and Fast for type 2
                                    scan_parameter - for regular calibration. value can be one of: Py scan (default) = 0, Rx scan = 1
                                    adjust_both_sides - for focal length calibration. value can be one of: 0 = adjust right only, 1 = adjust both sides
                                    white_wall_mode - white wall mode: 0 for normal mode and 1 for white wall mode
                                    if json is nullptr it will be ignored and calibration will use the default parameters
param[out] health            The absolute value of regular calibration Health-Check captures how far camera calibration is from the optimal one
                                    [0, 0.25) - Good
                                    [0.25, 0.75) - Can be Improved
                                    [0.75, ) - Requires Calibration
                                The absolute value of focal length calibration Health-Check captures how far camera calibration is from the optimal one
                                    [0, 0.15) - Good
                                    [0.15, 0.75) - Can be Improved
                                    [0.75, ) - Requires Calibration
                                The two health numbers are encoded in one integer as follows for calib_type 2:
                                    Regular health number times 1000 are bits 0 to 11
                                    Regular health number is negative if bit 24 is 1
                                    Focal length health number times 1000 are bits 12 to 23
                                    Focal length health number is negative if bit 25 is 1
param[in] callback           Optional callback for update progress notifications, the progress value is normailzed to 1
param[in] client_data        Optional client data for the callback
param[in] timeout_ms         Timeout in ms (use 5000 msec unless instructed otherwise)
return                       New calibration table


}
function rs2_run_on_chip_calibration(device: pRS2_device; json_content: pvoid;
  content_size: integer; healt: PSingle; callback: prs2_update_progress_callback_ptr;
  client_data: pvoid; timeout_ms: integer; error: pRS2_error): prs2_raw_data_buffer;
  cdecl; external REALSENSE_DLL;

{
  Used in device_calibration; enumerates the different calibration types
  available for that extension.
}
type
  rs2_calibration_type = (
    RS2_CALIBRATION_AUTO_DEPTH_TO_RGB,
    RS2_CALIBRATION_MANUAL_DEPTH_TO_RGB,
    RS2_CALIBRATION_THERMAL,
    RS2_CALIBRATION_TYPE_COUNT);

function rs2_calibration_type_to_string(aType: rs2_calibration_type): PChar;
  cdecl; external REALSENSE_DLL;
{
  Used in device_calibration with rs2_calibration_change_callback
}
{ Anything >= 0 is not an issue }
type
  rs2_calibration_status = (
    RS2_CALIBRATION_STATUS_FIRST = -5,
    RS2_CALIBRATION_BAD_CONDITIONS = -5,
    //< Trigger was attempted but conditions (temp/APD) were invalid (still inactive)
    RS2_CALIBRATION_BAD_RESULT = -4,
    //< Calibration finished, but results aren't good; will retry
    RS2_CALIBRATION_SCENE_INVALID = -3,
    //< Scene was not good enough for calibration; will retry
    RS2_CALIBRATION_FAILED = -2,
    //< Unexpected: exception, device removed, stream stopped, etc.
    RS2_CALIBRATION_RETRY = -1,
    //< Initiating retry (asked for a new special frame)
    RS2_CALIBRATION_TRIGGERED = 0,
    //< AC triggered and is active; conditions are valid
    RS2_CALIBRATION_SPECIAL_FRAME = 1,
    //< Special frame received; expect a frame-drop!
    RS2_CALIBRATION_STARTED = 2,
    //< Have all frames in hand; starting processing
    RS2_CALIBRATION_NOT_NEEDED = 3,
    //< Finished; existing calibration within tolerances; nothing done!
    RS2_CALIBRATION_SUCCESSFUL = 4,  //< Finished; have new calibration in-hand
    RS2_CALIBRATION_STATUS_LAST = 4,
    RS2_CALIBRATION_STATUS_COUNT =
    10 //<RS2_CALIBRATION_STATUS_LAST - RS2_CALIBRATION_STATUS_FIRST + 1);

    );


function rs2_calibration_status_to_string(ars2_calibration_status:
  rs2_calibration_status): PChar; cdecl; external REALSENSE_DLL;

type
  prs2_calibration_change_callback = POpaqueData;
  Prs2_calibration_change_callback_ptr = ^rs2_calibration_change_callback_ptr;
  rs2_calibration_change_callback_ptr =
    procedure(ars2_calibration_status: rs2_calibration_status; arg: pvoid); cdecl;

{
  Adds a callback for a sensor that gets called when calibration (intrinsics) changes, e.g. due to auto-calibration
  param[in] sensor        the sensor
  param[in] callback      the C callback function that gets called
  param[in] user          user argument that gets passed to the callback function
  param[out] error        if non-null, receives any error that occurs during this call, otherwise, errors are ignored

}
procedure rs2_register_calibration_change_callback(device: pRS2_device;
  callback: prs2_calibration_change_callback; user: pvoid; error: pRS2_error);
  cdecl; external REALSENSE_DLL;

{

  Triggers calibration of the given type
  param[in] dev           the device
  param[in] type          the type of calibration requested
  param[out] error        if non-null, receives any error that occurs during this call, otherwise, errors are ignored

}

procedure rs2_trigger_device_calibration(device: pRS2_device;
  atype: rs2_calibration_type; error: pRS2_error); cdecl; external REALSENSE_DLL;
{

 This will adjust camera absolute distance to flat target. User needs to enter the known ground truth.
param[in] ground_truth_mm     Ground truth in mm must be between 60 and 10000
param[in] json_content        Json string to configure tare calibration parameters:
                                    (
                                      "average_step_count": 20,
                                      "step count": 20,
                                      "accuracy": 2,
                                      "scan parameter": 0,
                                      "data sampling": 0
                                    )
                                    average step count - number of frames to average, must be between 1 - 30, default = 20
                                    step count - max iteration steps, must be between 5 - 30, default = 10
                                    accuracy - Subpixel accuracy level, value can be one of: Very high = 0 (0.025%), High = 1 (0.05%), Medium = 2 (0.1%), Low = 3 (0.2%), Default = Very high (0.025%), default is Medium
                                    scan_parameter - value can be one of: Py scan (default) = 0, Rx scan = 1
                                    data_sampling - value can be one of:polling data sampling = 0, interrupt data sampling = 1
                                    if json is nullptr it will be ignored and calibration will use the default parameters
param[in]  content_size       Json string size if its 0 the json will be ignored and calibration will use the default parameters
param[in]  callback           Optional callback for update progress notifications, the progress value is normailzed to 1
param[in]  client_data        Optional client data for the callback
param[in] timeout_ms          Timeout in ms (use 5000 msec unless instructed otherwise)
return                        New calibration table


}
function rs2_run_tare_calibration(device: pRS2_device; ground_truth_mm: single;
  json_content: pvoid; content_size: integer;
  callback: prs2_update_progress_callback_ptr; client_data: pvoid;
  timeout_ms: integer; error: pRS2_error): prs2_raw_data_buffer;
  cdecl; external REALSENSE_DLL;
{

  Read current calibration table from flash.
return    Calibration table

}
function rs2_get_calibration_table(dev: pRS2_device;
  error: pRS2_error): prs2_raw_data_buffer; cdecl; external REALSENSE_DLL;
{
  Set current table to dynamic area.
  param[in]     Calibration table
}

procedure rs2_set_calibration_table(device: pRS2_device; calibration: pvoid;
  calibration_size: integer; error: pRS2_error); cdecl; external REALSENSE_DLL;
{
Serialize JSON content, returns ASCII-serialized JSON string on success. otherwise nullptr */
rs2_raw_data_buffer* rs2_serialize_json(rs2_device* dev, rs2_error** error);
}
function rs2_serialize_json(dev: pRS2_device; error: pRS2_error): prs2_raw_data_buffer;
  cdecl; external REALSENSE_DLL;
{
 /* Load JSON and apply advanced-mode controls */
void rs2_load_json(rs2_device* dev, const void* json_content, unsigned content_size, rs2_error** error);
}

procedure rs2_load_json(dev: pRS2_device; json_content: pvoid;
  content_size: nativeuint; error: pRS2_error); cdecl; external REALSENSE_DLL;

{
  Run target-based focal length calibration
param[in]    device: device to calibrate
param[in]    left_queue: container for left IR frames with resoluton of  1280x720 and the target in the center of 320x240 pixels ROI.
param[in]    right_queue: container for right IR frames with resoluton of  1280x720 and the target in the center of 320x240 pixels ROI
param[in]    target_width: the rectangle width in mm on the target
param[in]    target_height: the rectangle height in mm on the target
param[in]    adjust_both_sides: 1 for adjusting both left and right camera calibration tables, and 0 for adjusting right camera calibraion table only
param[out]   ratio: the corrected ratio from the calibration
param[out]   angle: the target's tilt angle
param[in]    callback: Optional callback for update progress notifications, the progress value is normailzed to 1
param[in]    client_data: Optional client data for the callback
return       New calibration table

}
function rs2_run_focal_length_calibration(device: pRS2_device;
  left_queue: prs2_frame_queue; right_queue: prs2_frame_queue;
  target_width: single; target_height: single; adjust_both_sides: integer;
  ration: single; angle: Psingle; callback: rs2_update_progress_callback_ptr;
  client_data: pvoid; error: pRS2_error): prs2_raw_data_buffer; cdecl;
  external REALSENSE_DLL;
 {

  Depth-RGB UV-Map calibration. Applicable for D400 cameras
param[in]    device: device to calibrate
param[in]    left_queue: the frame queue for left IR frames with resoluton of 1280x720 and the target captured in the center of 320x240 pixels ROI.
param[in]    color_queue: the frame queue for RGB frames with resoluton of 1280x720 and the target in the center of 320x240 pixels ROI
param[in]    depth_queue: the frame queue for Depth frames with resoluton of 1280x720
param[in]    py_px_only: 1 for calibrating color camera py and px only, 1 for calibrating color camera py, px, fy, and fx.
param[out]   health: The four health check numbers in order of px, py, fx, fy for the calibration
param[in]    health_size: number of health check numbers, which is 4 by default
param[in]    callback: Optional callback for update progress notifications, the progress value is normailzed to 1
param[in]    client_data: Optional client data for the callback
return       New calibration table

 }

function rs2_run_uv_map_calibration(device: pRS2_device;
  left_queue: prs2_frame_queue; color_queue: prs2_frame_queue;
  depth_queue: prs2_frame_queue; py_py_only: integer; health: Psingle;
  health_size: integer; callback: prs2_update_progress_callback_ptr;
  cliente_data: pvoid; error: pRS2_error): prs2_raw_data_buffer;
  cdecl; external REALSENSE_DLL;
{

/**
* Calculate Z for calibration target - distance to the target's plane
* \param[in]    queue1-3: A frame queue of raw images used to calculate and extract the distance to a predefined target pattern.
* For D400 the indexes 1-3 correspond to Left IR, Right IR and Depth with only the Left IR being used
* \param[in]    target_width: Expected target's horizontal dimension in mm
* \param[in]    target_height: Expected target's vertical dimension in mm
* \param[in]    callback: Optional callback for reporting progress status
* \param[in]    client_data: Optional client data for the callback
* \return       Calculated distance (Z) to target in millimeter, or negative number if failed
*/
float rs2_calculate_target_z(rs2_device* device, rs2_frame_queue* queue1, rs2_frame_queue* queue2, rs2_frame_queue* queue3,
    float target_width, float target_height, rs2_update_progress_callback_ptr progress_callback, void* client_data, rs2_error** error);
}
function rs2_calculate_target_z(device: pRS2_device;
  queue1: prs2_frame_queue; queue2: prs2_frame_queue; queue3: prs2_frame_queue;
  target_width: integer; target_height: integer;
  callback: prs2_update_progress_callback_ptr; client_data: pvoid; error: pRS2_error): single;
  cdecl; external REALSENSE_DLL;

implementation

end.
