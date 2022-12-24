program RsColor;

uses
  SysUtils,
  rs,
  rs_types,
  rs_device,
  rs_context,
  rs_sensor,
  rs_pipeline,
  rs_config,
  rs_frame, rs_processing, rs_option, rs_internal, rs_record_playback, 
rs_advanced_mode_command, rs_advanced_mode;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                     These parameters are reconfigurable                                        //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const
  STREAM = RS2_STREAM_COLOR;
  // Trs2_stream is a types of data provided by RealSense device
  STFORMAT = RS2_FORMAT_RGB8;
  // Trs2_format identifies how binary data is encoded within a frame
  Width = 640;
  // Defines the number of columns for each frame
  Height = 480;
  // Defines the number of lines for each frame
  FPS = 30;
  // Defines the rate of frames per second
  STREAM_INDEX = 0;
  // Defines the stream index, used for multiple streams of the same type //
  ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
var
  e: PRS2_error;
  ctx: pRS2_context;
  device_list: pRS2_device_list;
  dev_count, NumOfFrames, I, FrameNumber, j: integer;
  dev: pRS2_device;
  pipeline: prs2_pipeline;
  config: prs2_config;
  PipeLineProfile: pRS2_pipeline_profile;
  frames, frame: pRS2_frame;
  p: pVoid;
  rgb_frame_data: uint8;
  FrameTimeStampDomain: Trs2_timestamp_domain;
  FrameTimeStampDomainStr: PChar;
  frameMetadataTimeOfArrival: Trs2_metadata_type;
  prgb_frame_data: ^uint8;
  FrameTimeStamp: Trs2_time_t;

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
  writeln(Format('There are %d connected RealSense devices.', [dev_count]));

  if dev_count = 0 then
  begin
    Exit;
  end;

  // Get the first connected device
  // The returned object should be released with rs2_delete_device(...)
  dev := rs2_create_device(device_list, 0, e);
  CheckError(e);
  PrintDevice(dev);

  // Create a pipeline to configure, start and stop camera streaming
  // The returned object should be released with rs2_delete_pipeline(...)
  pipeline := rs2_create_pipeline(ctx, e);
  CheckError(e);

  // Create a config instance, used to specify hardware configuration
  // The retunred object should be released with rs2_delete_config(...)
  config := rs2_create_config(e);
  CheckError(e);

  // Request a specific configuration
  rs2_config_enable_stream(config, STREAM, STREAM_INDEX, Width, Height,
    STFORMAT, FPS, e);
  CheckError(e);

  // Start the pipeline streaming
  // The retunred object should be released with rs2_delete_pipeline_profile(...)
  PipeLineProfile := rs2_pipeline_start_with_config(pipeline, config, e);
  if e <> nil then
  begin
    WriteLn('The connected device doesnot support color streaming!');
    Exit;
  end;

  repeat

    // This call waits until a new composite_frame is available
    // composite_frame holds a set of frames. It is used to prevent frame drops
    // The returned object should be released with rs2_release_frame(...)
    frames := rs2_pipeline_wait_for_frames(pipeline, RS2_DEFAULT_TIMEOUT, e);
    CheckError(e);
    // Returns the number of frames embedded within the composite frame
    NumOfFrames := rs2_embedded_frames_count(frames, e);
    CheckError(e);

    for I := 0 to NumOfFrames - 1 do
    begin
      // The retunred object should be released with rs2_release_frame(...)
      frame := rs2_extract_frame(frames, i, e);
      CheckError(e);

      prgb_frame_data := PUInt8(rs2_get_frame_data(frame, e));
      CheckError(e);

      FrameNumber := rs2_get_frame_data_size(frame, e);
      CheckError(e);

       FrameTimeStamp := rs2_get_frame_timestamp(frame,e);
       CheckError(e);

      // Specifies the clock in relation to which the frame timestamp was measured
      FrameTimeStampDomain := rs2_get_frame_timestamp_domain(frame, e);
      CheckError(e);
      FrameTimeStampDomainStr := rs2_timestamp_domain_to_string(FrameTimeStampDomain);

      frameMetadataTimeOfArrival :=
        rs2_get_frame_metadata(frame, RS2_FRAME_METADATA_TIME_OF_ARRIVAL, e);
      CheckError(e);

      WriteLn('RGB frame arrived');
      writeln('First 10 bytes');
      for j := 0 to 9 do
      begin
        Write(prgb_frame_data[j]);
      end;
      writeln('');
      writeln('Frame No' + IntToStr(FrameNumber));
       WriteLn('Timestamp: '+FloatToStr(FrameTimeStamp));
      writeln ( 'Timestamp domain: '+FrameTimeStampDomainstr);
      WriteLn('Time of arrival: '+FloatToStr(double(frameMetadataTimeOfArrival)));
      rs2_release_frame(frame);
    end;
    rs2_release_frame(frames);
  until True;


  rs2_delete_pipeline_profile(PipeLineProfile);
  rs2_delete_config(config);
  rs2_delete_pipeline(pipeline);
  rs2_delete_device(dev);
  rs2_delete_context(ctx);
  writeln('Press any key to continue');
  ReadLn;
end.
