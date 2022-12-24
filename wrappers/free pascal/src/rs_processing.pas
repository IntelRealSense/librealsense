{
 Exposes RealSense processing-block functionality
}
unit rs_processing;

{$mode ObjFPC}{$H+}

interface

uses
  rs_types,
  rs_sensor,
  rs_option;

{
  Creates Depth-Colorizer processing block that can be used to quickly visualize the depth data
  This block will accept depth frames as input and replace them by depth frames with format RGB8
  Non-depth frames are passed through
  Further customization will be added soon (format, color-map, histogram equalization control)
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}

function rs2_create_colorizer(error: pRS2_error): pRS2_processing_block;
  cdecl; external REALSENSE_DLL;
{
  Creates Sync processing block. This block accepts arbitrary frames and output composite frames of best matches
  Some frames may be released within the syncer if they are waiting for match for too long
  Syncronization is done (mostly) based on timestamps so good hardware timestamps are a pre-condition
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
function rs2_create_sync_processing_block(error: pRS2_error): pRS2_processing_block;
  cdecl; external REALSENSE_DLL;
{
  Creates Point-Cloud processing block. This block accepts depth frames and outputs Points frames
  In addition, given non-depth frame, the block will align texture coordinate to the non-depth stream
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
function rs2_create_pointcloud(error: pRS2_error): pRS2_processing_block;
  cdecl; external REALSENSE_DLL;
{
  Creates YUY decoder processing block. This block accepts raw YUY frames and outputs frames of other formats.
  YUY is a common video format used by a variety of web-cams. It benefits from packing pixels into 2 bytes per pixel
  without signficant quality drop. YUY representation can be converted back to more usable RGB form,
  but this requires somewhat costly conversion.
  The SDK will automatically try to use SSE2 and AVX instructions and CUDA where available to get
  best performance. Other implementations (using GLSL, OpenCL, Neon and NCS) should follow.
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
function rs2_create_yuy_decoder(error: pRS2_error): pRS2_processing_block;
  cdecl; external REALSENSE_DLL;
{
  Creates y411 decoder processing block. This block accepts raw y411 frames and outputs frames in RGB8.
  https://www.fourcc.org/pixel-format/yuv-y411/
  Y411 is disguised as NV12 to allow Linux compatibility. Both are 12bpp encodings that allow high-resolution
  modes in the camera to still fit within the USB3 limits (YUY wasn't enough).

  The SDK will automatically try to use SSE2 and AVX instructions and CUDA where available to get
  best performance. Other implementations (using GLSL, OpenCL, Neon and NCS) should follow.

  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
function rs2_create_y411_decoder(error: pRS2_error): pRS2_processing_block;
  cdecl; external REALSENSE_DLL;
{
  Creates depth thresholding processing block
  By controlling min and max options on the block, one could filter out depth values
  that are either too large or too small, as a software post-processing step
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
function rs2_create_threshold(error: pRS2_error): pRS2_processing_block;
  cdecl; external REALSENSE_DLL;

{
  Creates depth units transformation processing block
  All of the pixels are transformed from depth units into meters.
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
function rs2_create_units_transform(error: pRS2_error): pRS2_processing_block;
  cdecl; external REALSENSE_DLL;
{
  This method creates new custom processing block. This lets the users pass frames between module boundaries for processing
  This is an infrastructure function aimed at middleware developers, and also used by provided blocks such as sync, colorizer, etc..
  param proc       Processing function to be applied to every frame entering the block
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return           new processing block, to be released by rs2_delete_processing_block
}
function rs2_create_processing_block(proc: pRS2_frame_processor_callback;
  error: pRS2_error): pRS2_processing_block;
  cdecl; external REALSENSE_DLL;
{
  This method creates new custom processing block from function pointer. This lets the users pass frames between module boundaries for processing
  This is an infrastructure function aimed at middleware developers, and also used by provided blocks such as sync, colorizer, etc..
  param proc       Processing function pointer to be applied to every frame entering the block
  param context    User context (can be anything or null) to be passed later as ctx param of the callback
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return           new processing block, to be released by rs2_delete_processing_block
}
function rs2_create_processing_block_fptr(proc: pRS2_frame_processor_callback_ptr;
  context: pvoid; error: pRS2_error): pRS2_processing_block;
  cdecl; external REALSENSE_DLL;
{
  This method adds a custom option to a custom processing block. This is a simple float that can be accessed via rs2_set_option and rs2_get_option
  This is an infrastructure function aimed at middleware developers, and also used by provided blocks such as save_to_ply, etc..
  param[in] block      Processing block
  param[in] option_id  an int ID for referencing the option
  param[in] min     the minimum value which will be accepted for this option
  param[in] max     the maximum value which will be accepted for this option
  param[in] step    the granularity of options which accept discrete values, or zero if the option accepts continuous values
  param[in] def     the default value of the option. This will be the initial value.
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return            true if adding the option succeeds. false if it fails e.g. an option with this id is already registered
}
function rs2_processing_block_register_simple_option(block: pRS2_processing_block;
  option: Trs2_option; min: single; max: single; step: single; def: single;
  error: pRS2_error): integer; cdecl; external REALSENSE_DLL;

{
  This method is used to direct the output from the processing block to some callback or sink object
  param[in] block          Processing block
  param[in] on_frame       Callback to be invoked every time the processing block calls frame_ready
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_start_processing(block: pRS2_processing_block;
  on_frame: pRS2_frame_processor_callback; error: pRS2_error); cdecl;
  external REALSENSE_DLL;
{
  This method is used to direct the output from the processing block to some callback or sink object
  param[in] block          Processing block
  param[in] on_frame       Callback function to be invoked every time the processing block calls frame_ready
  param[in] user           User context for the callback (can be anything or null)
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_start_processing_fptr(block: pRS2_processing_block;
  on_frame: pRS2_frame_callback_ptr; user: pvoid; error: pRS2_error);
  cdecl; external REALSENSE_DLL;
{
  This method is used to direct the output from the processing block to a dedicated queue object
  param[in] block          Processing block
  param[in] queue          Queue to place the processed frames to
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_start_processing_queue(block: pRS2_processing_block;
  queue: pRS2_frame_queue; error: pRS2_error); cdecl; external REALSENSE_DLL;

{
  This method is used to pass frame into a processing block
  param[in] block          Processing block
  param[in] frame          Frame to process, ownership is moved to the block object
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
procedure rs2_process_frame(block: pRS2_processing_block; frame: pRS2_frame;
  error: pRS2_error); cdecl; external REALSENSE_DLL;
{
  Deletes the processing block
  param[in] block          Processing block
}
procedure rs2_delete_processing_block(block: pRS2_processing_block);
  cdecl; external REALSENSE_DLL;
{
 create frame queue. frame queues are the simplest x-platform synchronization primitive provided by librealsense
 to help developers who are not using async APIs
 param[in] capacity max number of frames to allow to be stored in the queue before older frames will start to get dropped
 param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 return handle to the frame queue, must be released using rs2_delete_frame_queue
}
function rs2_create_frame_queue(capacity: integer; error: pRS2_error): pRS2_frame_queue;
  cdecl; external REALSENSE_DLL;
{
 deletes frame queue and releases all frames inside it
 param[in] queue queue to delete
}
procedure rs2_delete_frame_queue(queue: pRS2_frame_queue); cdecl; external REALSENSE_DLL;

{
  queries the number of frames
  param[in] queue to delete
  returns the number of frames currently stored in queue
}
function rs2_frame_queue_size(queue: pRS2_frame_queue; error: pRS2_error): integer;
  cdecl; external REALSENSE_DLL;
{
  wait until new frame becomes available in the queue and dequeue it
  param[in] queue the frame queue data structure
  param[in] timeout_ms   max time in milliseconds to wait until an exception will be thrown
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return frame handle to be released using rs2_release_frame
}
function rs2_wait_for_frame(queue: pRS2_frame_queue; timeout_ms: cardinal;
  error: pRS2_error): pRS2_frame; cdecl; external REALSENSE_DLL;
{
  poll if a new frame is available and dequeue if it is
  param[in] queue the frame queue data structure
  param[out] output_frame frame handle to be released using rs2_release_frame
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return true if new frame was stored to output_frame
}
function rs2_poll_for_frame(queue: pRS2_frame; output_frame: pRS2_frame;
  error: pRS2_error): integer; cdecl; external REALSENSE_DLL;
{
  wait until new frame becomes available in the queue and dequeue it
  param[in] queue          the frame queue data structure
  param[in] timeout_ms     max time in milliseconds to wait until a frame becomes available
  param[out] output_frame  frame handle to be released using rs2_release_frame
  param[out] error         if non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return true if new frame was stored to output_frame
}
function rs2_try_wait_for_frame(queue: pRS2_frame; timeout_ms: cardinal;
  output_frame: pRS2_frame; error: pRS2_error): integer; cdecl; external REALSENSE_DLL;
{
  enqueue new frame into a queue
  param[in] frame frame handle to enqueue (this operation passed ownership to the queue)
  param[in] queue the frame queue data structure
}
procedure rs2_enqueue_frame(frame: pRS2_frame; queue: pvoid); cdecl;
  external REALSENSE_DLL;
{
 Creates Align processing block.
 param[in] align_to   stream type to be used as the target of frameset alignment
 param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
function rs2_create_align(align_to: Trs2_stream;
  error: pRS2_error): pRS2_processing_block;
  cdecl; external REALSENSE_DLL;
{
 Creates Depth post-processing filter block. This block accepts depth frames, applies decimation filter and plots modified prames
 Note that due to the modifiedframe size, the decimated frame repaces the original one
 param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}

function rs2_create_decimation_filter_block(error: pRS2_error): pRS2_processing_block;
  cdecl; external REALSENSE_DLL;

{
  Creates Depth post-processing filter block. This block accepts depth frames, applies temporal filter
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
function rs2_create_temporal_filter_block(error: pRS2_error): pRS2_processing_block;
  cdecl; external REALSENSE_DLL;
{
  Creates Depth post-processing spatial filter block. This block accepts depth frames, applies spatial filters and plots modified prames
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
function rs2_create_spatial_filter_block(error: pRS2_error): pRS2_processing_block;
  cdecl; external REALSENSE_DLL;
{
  Creates a post processing block that provides for depth<->disparity domain transformation for stereo-based depth modules
  param[in] transform_to_disparity flag select the transform direction:  true = depth->disparity, and vice versa
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
function rs2_create_disparity_transform_block(transform_to_disparity: byte;
  error: pRS2_error): pRS2_processing_block; cdecl; external REALSENSE_DLL;
{
 Creates Depth post-processing hole filling block. The filter replaces empty pixels with data from adjacent pixels based on the method selected
 param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
function rs2_create_hole_filling_filter_block(error: pRS2_error): pRS2_processing_block;
  cdecl; external REALSENSE_DLL;
{
 Creates a rates printer block. The printer prints the actual FPS of the invoked frame stream.
 The block ignores reapiting frames and calculats the FPS only if the frame number of the relevant frame was changed.
 param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
function rs2_create_rates_printer_block(error: pRS2_error): pRS2_processing_block;
  cdecl; external REALSENSE_DLL;
{
 Creates Depth post-processing zero order fix block. The filter invalidates pixels that has a wrong value due to zero order effect
 param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
 return               zero order fix processing block
}
function rs2_create_zero_order_invalidation_block(error: pRS2_error):
  pRS2_processing_block; cdecl; external REALSENSE_DLL;
{
  Creates Depth frame decompression module. Decoded frames compressed and transmitted with Z16H variable-lenght Huffman code to
  standartized Z16 Depth data format. Using the compression allows to reduce the Depth frames bandwidth by more than 50 percent
  param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
  return               Huffman-code decompression processing block
}
function rs2_create_huffman_depth_decompress_block(error: pRS2_error):
  pRS2_processing_block; cdecl; external REALSENSE_DLL;
{
 Creates a hdr_merge processing block.
 The block merges between two depth frames with different exposure values
 param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
function rs2_create_hdr_merge_processing_block(error: pRS2_error):
  pRS2_processing_block;
  cdecl; external REALSENSE_DLL;
{
  Creates a sequence_id_filter processing block.
  The block lets frames with the selected sequence id pass and blocks frames with other values
  param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
}
function rs2_create_sequence_id_filter(error: pRS2_error): pRS2_processing_block;
  cdecl; external REALSENSE_DLL;
{
 Retrieve processing block specific information, like name.
 param[in]  block     The processing block
 param[in]  info      processing block info type to retrieve
 param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
 return               The requested processing block info string, in a format specific to the device model
}
function rs2_get_processing_block_info(const block: pRS2_processing_block;
  info: Trs2_camera_info; error: pRS2_error): PChar; cdecl; external REALSENSE_DLL;
{
 Check if a processing block supports a specific info type.
 param[in]  block     The processing block to check
 param[in]  info      The parameter to check for support
 param[out] error     If non-null, receives any error that occurs during this call, otherwise, errors are ignored
 return               True if the parameter both exist and well-defined for the specific device
}
function rs2_supports_processing_block_info(block: pRS2_processing_block;
  info: Trs2_camera_info; error: pRS2_error): integer; cdecl; external REALSENSE_DLL;
{
 Test if the given processing block can be extended to the requested extension
 param[in] block processing block
 param[in] extension The extension to which the sensor should be tested if it is extendable
 param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
 return non-zero value iff the processing block can be extended to the given extension
}
function rs2_is_processing_block_extendable_to(const block: pRS2_processing_block;
  extension_type: Trs2_extension; error: pRS2_error): integer;
  cdecl; external REALSENSE_DLL;

implementation


end.
