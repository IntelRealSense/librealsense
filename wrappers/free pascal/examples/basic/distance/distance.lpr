program distance;

uses
  SysUtils,
  rs,
  rs_frame,
  rs_option,
  rs_pipeline,
  rs_types,
  rs_context,
  rs_device,
  rs_sensor,
  rs_config;

const
  STREAM = RS2_STREAM_DEPTH;
  // Trs2_stream is a types of data provided by RealSense device
  STFORMAT = RS2_FORMAT_Z16;
  // Trs2_format identifies how binary data is encoded within a frame
  Width = 640;
  // Defines the number of columns for each frame
  Height = 0;
  // Defines the number of lines for each frame or zero for auto resolve
  FPS = 30;
  // Defines the rate of frames per second
  STREAM_INDEX = 0;
  // Defines the stream index, used for multiple streams of the same type //
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
var
  e: pRS2_error;
  ctx: pRS2_context;
  device_list: pRS2_device_list;
  dev_count, num_of_frames, I, awidth, aheight: integer;
  dev: pRS2_device;
  pipeline: pRS2_pipeline;
  config: pRS2_config;
  pipeline_profile: pRS2_pipeline_profile;
  frames, frame: pRS2_frame;
  aFORMAT: Trs2_format;
  dist_to_center: single;

  procedure CheckError(var aError: pRS2_error);
  var
    Cadena: string;
  begin
    if aError <> nil then
    begin
      Cadena := format('Function: %s, Arguments: %s, message: %s',
        [rs2_get_failed_function(aError), rs2_get_failed_args(aError),
        rs2_get_error_message(aError)]);
      WriteLn(Cadena);
      rs2_free_error(aError);
    end;
  end;

  procedure PrintDevice(dev: pRS2_device);
  begin
    WriteLn(Format('Using device 0, an %s', [rs2_get_device_info(
      dev, RS2_CAMERA_INFO_NAME, e)]));
    CheckError(e);
    Writeln(Format('     Serial number: %s',
      [rs2_get_device_info(dev, RS2_CAMERA_INFO_SERIAL_NUMBER, e)]));
    CheckError(e);
    Writeln(Format('     Firmware version: %s', [rs2_get_device_info(
      dev, RS2_CAMERA_INFO_FIRMWARE_VERSION, e)]));
    CheckError(e);
  end;

begin
  // Create a context object. This object owns the handles to all connected realsense devices.
  // The returned object should be released with rs2_delete_context(...)
  ctx := rs2_create_context(RS2_API_VERSION, e);
  CheckError(e);

  // Get a list of all the connected devices.
  // The returned object should be released with rs2_delete_device_list(...)
  device_list := rs2_query_devices(ctx, e);
  CheckError(e);

  dev_count := rs2_get_device_count(device_list, e);
  CheckError(e);

  if dev_count = 0 then
  begin
    Writeln('Fail, no device found');
    Halt(-1);
  end;

  writeln(Format('Found %i devices', [dev_count]));
  // Get the first connected device
  // The returned object should be released with rs2_delete_device(...)
  dev := rs2_create_device(device_list, 0, e);
  CheckError(e);
  PrintDevice(dev);

  // Create a pipeline to configure, start and stop camera streaming
  // The returned object should be released with rs2_delete_pipeline(...)
  pipeline := rs2_create_pipeline(ctx, e);
  checkerror(e);

  // Create a config instance, used to specify hardware configuration
  // The retunred object should be released with rs2_delete_config(...)
  config := rs2_create_config(e);
  checkerror(e);

  // Request a specific configuration
  rs2_config_enable_stream(config, STREAM, STREAM_INDEX, Width,
    Height, aFORMAT, FPS, e);
  checkerror(e);

  // Start the pipeline streaming
  // The retunred object should be released with rs2_delete_pipeline_profile(...)
  pipeline_profile := rs2_pipeline_start_with_config(pipeline, config, e);
  if e <> nil then
  begin
    WriteLn('Device doesnot suppor depth frame');
    halt(-2);
  end;

  while True do
  begin

    // This call waits until a new composite_frame is available
    // composite_frame holds a set of frames. It is used to prevent frame drops
    // The returned object should be released with rs2_release_frame(...)
    frames := rs2_pipeline_wait_for_frames(pipeline, RS2_DEFAULT_TIMEOUT, e);
    checkerror(e);

    // Returns the number of frames embedded within the composite frame
    num_of_frames := rs2_embedded_frames_count(frames, e);
    checkerror(e);

    for  I := 0 to num_of_frames do
    begin
      // The retunred object should be released with rs2_release_frame(...)
      frame := rs2_extract_frame(frames, i, e);
      CheckError(e);
      // Check if the given frame can be extended to depth frame interface
      // Accept only depth frames and skip other frames
      if (rs2_is_frame_extendable_to(frame, RS2_EXTENSION_DEPTH_FRAME, e) = 0) then
      begin
        Continue;
      end;

      // Get the depth frame's dimensions
      awidth := rs2_get_frame_width(frame, e);
      checkerror(e);
      aheight := rs2_get_frame_height(frame, e);
      checkerror(e);

      dist_to_center := rs.rs2_depth_frame_get_distance(frame, awidth div
        2, aheight div 2, e);
      CheckError(e);
      Writeln(Format('The camera is facing an object %f meters away.', [dist_to_center]));
      rs2_release_frame(frame);
    end;
    rs2_release_frame(frames);
  end;
  // Stop the pipeline streaming
  rs2_pipeline_stop(pipeline, e);
  checkerror(e);

  // Release resources
  rs2_delete_pipeline_profile(pipeline_profile);
  rs2_delete_config(config);
  rs2_delete_pipeline(pipeline);
  rs2_delete_device(dev);
  rs2_delete_device_list(device_list);
  rs2_delete_context(ctx);
end.
