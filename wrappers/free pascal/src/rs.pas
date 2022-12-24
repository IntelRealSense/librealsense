{

 Exposes librealsense functionality
}
unit rs;

{$mode ObjFPC}{$H+}

interface

uses  SysUtils,
  rs_types;

const

  RS2_API_MAJOR_VERSION = 2;
  RS2_API_MINOR_VERSION = 51;
  RS2_API_PATCH_VERSION = 1;
  RS2_API_BUILD_VERSION = 4348;
  RS2_API_VERSION = (((RS2_API_MAJOR_VERSION) * 10000) +
    ((RS2_API_MINOR_VERSION) * 100) + (RS2_API_PATCH_VERSION));

function RS2_API_VERSION_STR: string;
function RS2_API_FULL_VERSION_STR: string;

{
  get the size of rs2_raw_data_buffer
  param[in] buffer  pointer to rs2_raw_data_buffer returned by rs2_send_and_receive_raw_data
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return size of rs2_raw_data_buffer
}
function rs2_get_raw_data_size(const buffer: pRS2_raw_data_buffer;
  error: pRS2_error): integer; cdecl; external REALSENSE_DLL;
{
  Delete rs2_raw_data_buffer
  param[in] buffer        rs2_raw_data_buffer returned by rs2_send_and_receive_raw_data
}
procedure rs2_delete_raw_data(const buffer: pRS2_raw_data_buffer);
  cdecl; external REALSENSE_DLL;
{
  Retrieve char array from rs2_raw_data_buffer
  param[in] buffer   rs2_raw_data_buffer returned by rs2_send_and_receive_raw_data
  param[out] error   if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return raw data
}
function rs2_get_raw_data(buffer: pRS2_raw_data_buffer; error: pRS2_error): byte;
  cdecl; external REALSENSE_DLL;

{
Retrieve the API version from the source code. Evaluate that the value is conformant to the established policies
param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
return            the version API encoded into integer value "1.9.3" -> 10903
}
function rs2_get_api_version(error: pRS2_error): integer; cdecl; external REALSENSE_DLL;

procedure rs2_log_to_console(min_severity: Trs2_log_severity; error: pRS2_error);
  cdecl; external REALSENSE_DLL;

procedure rs2_log_to_file(min_severity: Trs2_log_severity; const file_path: PChar;
  error: pRS2_error); cdecl; external REALSENSE_DLL;

procedure rs2_log_to_callback(min_severity: Trs2_log_severity;
  callback: pRS2_log_callback_ptr; arg: pvoid; error: pRS2_error); cdecl;
  external REALSENSE_DLL;

procedure rs2_reset_logger(error: prs2_error); cdecl; external REALSENSE_DLL;

{
  Enable rolling log file when used with rs2_log_to_file:
  Upon reaching (max_size/2) bytes, the log will be renamed with an ".old" suffix and a new log created. Any
  previous .old file will be erased.
  Must have permissions to remove/rename files in log file directory.
  param[in] max_size   max file size in megabytes
  param[out] error     if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_enable_rolling_log_file(max_size: nativeuint; error: pRS2_error);
  cdecl; external REALSENSE_DLL;

function rs2_get_log_message_line_number(const msg: pRS2_log_message;
  error: pRS2_log_message): nativeuint; cdecl; external REALSENSE_DLL;

function rs2_get_log_message_filename(const msg: pRS2_log_message;
  error: pRS2_error): PChar; cdecl; external REALSENSE_DLL;

function rs2_get_raw_log_message(const msg: pRS2_log_message; error: pRS2_error): PChar;
  cdecl; external REALSENSE_DLL;

function rs2_get_full_log_message(const msg: pRS2_log_message; error: pRS2_error): PChar;
  cdecl; external REALSENSE_DLL;
{
  Add custom message into librealsense log
  param[in] severity  The log level for the message to be written under
  param[in] message   Message to be logged
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_log(severity: Trs2_log_severity; const message: PChar; error: pRS2_error);
  cdecl; external REALSENSE_DLL;

{
 Given the 2D depth coordinate (x,y) provide the corresponding depth in metric units
 param[in] frame_ref  2D depth pixel coordinates (Left-Upper corner origin)
 param[in] x,y  2D depth pixel coordinates (Left-Upper corner origin)
 param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
function rs2_depth_frame_get_distance(const frame_ref: pRS2_frame;
  x: integer; y: integer; error: pRS2_error): single; cdecl; external REALSENSE_DLL;

{
  return the time at specific time point
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return            the time at specific time point, in live and record mode it will return the system time and in playback mode it will return the recorded time
}
function rs2_get_time(error: pRS2_error): Trs2_time_t; cdecl; external REALSENSE_DLL;

implementation

function RS2_API_VERSION_STR: string;
begin
  Result := Format('%d.%d.%d', [RS2_API_MAJOR_VERSION, RS2_API_MINOR_VERSION,
    RS2_API_PATCH_VERSION]);
end;

function RS2_API_FULL_VERSION_STR: string;
begin
  Result := Format('%d.%d.%d.%d', [RS2_API_MAJOR_VERSION, RS2_API_MINOR_VERSION,
    RS2_API_PATCH_VERSION, RS2_API_BUILD_VERSION]);
end;


end.
