{

  Exposes RealSense context functionality
}
unit rs_context;

interface

uses rs_types;

const

  RS2_PRODUCT_LINE_ANY = $0FF;
  RS2_PRODUCT_LINE_ANY_INTEL = $FE;
  RS2_PRODUCT_LINE_NON_INTEL = $01;
  RS2_PRODUCT_LINE_D400 = $02;
  RS2_PRODUCT_LINE_SR300 = $04;
  RS2_PRODUCT_LINE_L500 = $08;
  RS2_PRODUCT_LINE_T200 = $10;
  RS2_PRODUCT_LINE_DEPTH = (RS2_PRODUCT_LINE_L500 or RS2_PRODUCT_LINE_SR300 or
    RS2_PRODUCT_LINE_D400);
  RS2_PRODUCT_LINE_TRACKING = RS2_PRODUCT_LINE_T200;

  {

    brief Creates RealSense context that is required for the rest of the API.
    param[in] api_version Users are expected to pass their version of \c RS2_API_VERSION to make sure they are running the correct librealsense version.
    param[out] error  If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
    return            Context object

  }
function rs2_create_context(api_version: integer; var Error: PRS2_error)
  : pRS2_context; cdecl; external REALSENSE_DLL;

{
  brief Frees the relevant context object.
  param[in] context Object that is no longer needed
}
procedure rs2_delete_context(Ctx: pRS2_context); cdecl; external REALSENSE_DLL;

{ /**
  * set callback to get devices changed events
  * these events will be raised by the context whenever new RealSense device is connected or existing device gets disconnected
  * \param context     Object representing librealsense session
  * \param[in] callback function pointer to register as per-notifications callback
  * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  */
}
procedure rs2_set_devices_changed_callback(Context: pRS2_context;
  Devices_Changed_Callback: rs2_devices_changed_callback; User: pVoid;
  Error: PRS2_error); cdecl; external REALSENSE_DLL;
{
  Create a new device and add it to the context
  \param ctx   The context to which the new device will be added
  \param file  The file from which the device should be created
  \param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
  @return  A pointer to a device that plays data from the file, or null in case of failure
}

function rs2_context_add_device(Ctx: pRS2_context; FileName: PChar;
  Error: PRS2_error): prs2_device; cdecl; external REALSENSE_DLL;

{
  Add an instance of software device to the context
  param ctx   The context to which the new device will be added
  param dev   Instance of software device to register into the context
  param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_context_add_software_device(Ctx: pRS2_context; Dev: prs2_device;
  Error: PRS2_error); cdecl; external REALSENSE_DLL;

{
  Removes a playback device from the context, if exists
  \param[in]  ctx       The context from which the device should be removed
  \param[in]  file      The file name that was used to add the device
  \param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_context_remove_device(Ctx: pRS2_context; FileName: PChar;
  Error: PRS2_error); cdecl; external REALSENSE_DLL;

{
  Removes tracking module.
  function query_devices() locks the tracking module in the tm_context object.
  If the tracking module device is not used it should be removed using this function, so that other applications could find it.
  This function can be used both before the call to query_device() to prevent enabling tracking modules or afterwards to
  release them.
}
procedure rs2_context_unload_tracking_module(Ctx: pRS2_context;
  Error: PRS2_error); cdecl; external REALSENSE_DLL;

{
  create a static snapshot of all connected devices at the time of the call
  \param context     Object representing librealsense session
  \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  \return            the list of devices, should be released by rs2_delete_device_list
}
function rs2_query_devices(Context: pRS2_context; var Error: PRS2_error)
  : prs2_device_list; cdecl; external REALSENSE_DLL;

{
  create a static snapshot of all connected devices at the time of the call
  \param context     Object representing librealsense session
  \param product_mask Controls what kind of devices will be returned
  \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  \return            the list of devices, should be released by rs2_delete_device_list
}
function rs2_query_devices_ex(Context: pRS2_context; Product_Mask: integer;
  Error: PRS2_error): prs2_device_list; cdecl; external REALSENSE_DLL;

{
  brief Creates RealSense device_hub .
  \param[in] context The context for the device hub
  \param[out] error  If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
  \return            Device hub object
}
function rs2_create_device_hub(Context: pRS2_context; Error: PRS2_error)
  : prs2_device_hub; cdecl; external REALSENSE_DLL;

{
  If any device is connected return it, otherwise wait until next RealSense device connects.
  Calling this method multiple times will cycle through connected devices
  \param[in] ctx The context to creat the device
  \param[in] hub The device hub object
  \param[out] error  If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
  \return            device object

}

function rs2_device_hub_wait_for_device(Hub: prs2_device_hub; Error: PRS2_error)
  : prs2_device; cdecl; external REALSENSE_DLL;

{
  Checks if device is still connected
  \param[in] hub The device hub object
  \param[in] device The device
  \param[out] error  If non-null, receives any error that occurs during this call, otherwise, errors are ignored.
  \return            1 if the device is connected, 0 otherwise
}
function rs2_device_hub_is_device_connected(Hub: prs2_device_hub;
  Device: prs2_device; Error: PRS2_error): integer; cdecl;
  external REALSENSE_DLL;

implementation

end.
