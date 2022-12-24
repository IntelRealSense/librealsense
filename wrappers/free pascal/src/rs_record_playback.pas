{
 Exposes record and playback functionality
}
unit rs_record_playback;

{$mode ObjFPC}{$H+}

interface

uses
  rs_types;

type
  Trs2_playback_status = (
    RS2_PLAYBACK_STATUS_UNKNOWN, //< Unknown state */
    RS2_PLAYBACK_STATUS_PLAYING,
    //< One or more sensors were started, playback is reading and raising data */
    RS2_PLAYBACK_STATUS_PAUSED,
    //< One or more sensors were started, but playback paused reading and paused raising data*/
    RS2_PLAYBACK_STATUS_STOPPED,
    //< All sensors were stopped, or playback has ended (all data was read). This is the initial playback status*/
    RS2_PLAYBACK_STATUS_COUNT);

  PRS2_playback_status = ^Trs2_playback_status;

function rs2_playback_status_to_string(status: Trs2_playback_status): PChar;
  cdecl; external REALSENSE_DLL;

type
  pRS2_playback_status_changed_callback_ptr =
    ^rs2_playback_status_changed_callback_ptr;

  rs2_playback_status_changed_callback_ptr =
    procedure(aTrs2_playback_status: Trs2_playback_status); cdecl;

{
  Creates a recording device to record the given device and save it to the given file
  param[in]  device    The device to record
  param[in]  file      The desired path to which the recorder should save the data
  param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return A pointer to a device that records its data to file, or null in case of failure
}

function rs2_create_record_device(const device: pRS2_device;
  const FileName: PChar; error: pRS2_error): pRS2_device; cdecl; external REALSENSE_DLL;
{
  Creates a recording device to record the given device and save it to the given file
  param[in]  device                The device to record
  param[in]  file                  The desired path to which the recorder should save the data
  param[in]  compression_enabled   Indicates if compression is enabled, 0 means false, otherwise true
  param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return A pointer to a device that records its data to file, or null in case of failure
}
function rs2_create_record_device_ex(const device: pRS2_device;
  const FileName: PChar; compression_enabled: integer; error: pRS2_error): pRS2_device;
  cdecl; external REALSENSE_DLL;
{
  Pause the recording device without stopping the actual device from streaming.
  Pausing will cause the device to stop writing new data to the file, in particular, frames and changes to extensions
  param[in]  device    A recording device
  param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_record_device_pause(const device: pRS2_device; error: pRS2_error);
  cdecl; external REALSENSE_DLL;
{
  Unpause the recording device. Resume will cause the device to continue writing new data to the file, in particular, frames and changes to extensions
  param[in]  device    A recording device
  param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_record_device_resume(const device: pRS2_device; error: pRS2_device);
  cdecl; external REALSENSE_DLL;
{
  Gets the name of the file to which the recorder is writing
  param[in]  device    A recording device
  param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return The  name of the file to which the recorder is writing
}
function rs2_record_device_filename(const device: pRS2_device; error: pRS2_error): PChar;
  cdecl; external REALSENSE_DLL;
{
  Creates a playback device to play the content of the given file
  param[in]  file      Path to the file to play
  param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return A pointer to a device that plays data from the file, or null in case of failure
}

function rs2_create_playback_device(const FileName: PChar;
  error: pRS2_error): pRS2_device; cdecl; external REALSENSE_DLL;
{
  Gets the path of the file used by the playback device
  param[in] device A playback device
  param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return Path to the file used by the playback device
}
function rs2_playback_device_get_file_path(const device: pRS2_device;
  error: pRS2_error): PChar; cdecl; external REALSENSE_DLL;
{
  Gets the total duration of the file in units of nanoseconds
  param[in] device     A playback device
  param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return Total duration of the file in units of nanoseconds
}
function rs2_playback_get_duration(const device: pRS2_device; error: pRS2_error): uint64;
  cdecl external REALSENSE_DLL;
{
  Set the playback to a specified time point of the played data
  param[in] device     A playback device.
  param[in] time       The time point to which playback should seek, expressed in units of nanoseconds (zero value = start)
  param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_playback_seek(const Device: pRS2_device; time: int64; error: pRS2_error);
  cdecl; external REALSENSE_DLL;
{
  Gets the current position of the playback in the file in terms of time. Units are expressed in nanoseconds
  param[in] device     A playback device
  param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return Current position of the playback in the file in terms of time. Units are expressed in nanoseconds
}
function rs2_playback_get_position(const device: pRS2_device; error: pRS2_error): uint64;
  cdecl; external REALSENSE_DLL;
{
  Pauses the playback
  Calling pause() in "Paused" status does nothing
  If pause() is called while playback status is "Playing" or "Stopped", the playback will not play until resume() is called
  param[in] device A playback device
  param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_playback_device_resume(const device: pRS2_device; error: pRS2_error);
  cdecl; external REALSENSE_DLL;
{
  Un-pauses the playback
  Calling resume() while playback status is "Playing" or "Stopped" does nothing
  param[in] device A playback device
  param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_playback_device_pause(const device: pRS2_device; error: pRS2_error);
  cdecl; external REALSENSE_DLL;
{
  Set the playback to work in real time or non real time

  In real time mode, playback will play the same way the file was recorded.
  In real time mode if the application takes too long to handle the callback, frames may be dropped.
  In non real time mode, playback will wait for each callback to finish handling the data before
  reading the next frame. In this mode no frames will be dropped, and the application controls the
  frame rate of the playback (according to the callback handler duration).
  param[in] device A playback device
  param[in] real_time  Indicates if real time is requested, 0 means false, otherwise true
  param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_playback_device_set_real_time(const device: pRS2_device;
  real_time: integer; error: pRS2_error); cdecl; external REALSENSE_DLL;
{
  Indicates if playback is in real time mode or non real time
  param[in] device A playback device
  param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return True iff playback is in real time mode. 0 means false, otherwise true
}
function rs2_playback_device_is_real_time(const device: pRS2_device;
  real_time: integer; error: pRS2_error): integer; cdecl; external REALSENSE_DLL;
{
  Register to receive callback from playback device upon its status changes

  Callbacks are invoked from the reading thread, any heavy processing in the callback handler will affect
  the reading thread and may cause frame drops\ high latency
  param[in] device     A playback device
  param[in] callback   A callback handler that will be invoked when the playback status changes
  param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_playback_device_set_status_changed_callback(const pRS2_device;
  Callback: pRS2_playback_status_changed_callback_ptr; error: pRS2_error);
  cdecl; external;
{
  Returns the current state of the playback device
  param[in] device     A playback device
  param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return Current state of the playback
}
function rs2_playback_device_get_current_status(const device: pRS2_device;
  error: pRS2_error): PRS2_playback_status; cdecl; external REALSENSE_DLL;
{
  Set the playing speed

  param[in] device A playback device
  param[in] speed  Indicates a multiplication of the speed to play (e.g: 1 = normal, 0.5 twice as slow)
  param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_playback_device_set_playback_speed(const device: pRS2_device;
  speed: single; error: pRS2_error); cdecl; external REALSENSE_DLL;
{
 Stops the playback
 Calling stop() will stop all streaming playbakc sensors and will reset the playback (returning to beginning of file)
 param[in] device A playback device
 param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_playback_device_stop(const device: pRS2_device;
  error: pRS2_error); cdecl; external REALSENSE_DLL;

implementation

end.
