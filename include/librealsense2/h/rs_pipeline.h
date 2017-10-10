/* License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2017 Intel Corporation. All Rights Reserved. */

/** \file rs2_processing.h
* \brief
* Exposes RealSense processing-block functionality for C compilers
*/


#ifndef LIBREALSENSE_RS2_PIPELINE_H
#define LIBREALSENSE_RS2_PIPELINE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "rs_types.h"

    /**
    * Create a pipeline instance
    * The pipeline simplifies the user interaction with the device and computer vision processing modules.
    * The class abstracts the camera configuration and streaming, and the vision modules triggering and threading.
    * It lets the application focus on the computer vision output of the modules, or the device output data.
    * The pipeline can manage computer vision modules, which are implemented as a processing blocks.
    * The pipeline is the consumer of the processing block interface, while the application consumes the
    * computer vision interface.
    * \param[in]  ctx    context
    * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
    */
    rs2_pipeline* rs2_create_pipeline(rs2_context* ctx, rs2_error ** error);

    /**
    * Stop the pipeline streaming.
    * The pipeline stops delivering samples to the attached computer vision modules and processing blocks, stops the device streaming
    * and releases the device resources used by the pipeline. It is the application's responsibility to release any frame reference it owns.
    * The method takes effect only after \c start() was called, otherwise an exception is raised.
    * \param[in] pipe  pipeline
    * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
    */
    void rs2_pipeline_stop(rs2_pipeline* pipe, rs2_error ** error);

    /**
    * Wait until a new set of frames becomes available.
    * The frames set includes time-synchronized frames of each enabled stream in the pipeline.
    * The method blocks the calling thread, and fetches the latest unread frames set.
    * Device frames, which were produced while the function wasn't called, are dropped. To avoid frame drops, this method should be called
    * as fast as the device frame rate.
    * The application can maintain the frames handles to defer processing. However, if the application maintains too long history, the device
    * may lack memory resources to produce new frames, and the following call to this method shall fail to retrieve new frames, until resources
    * are retained.
    * \param[in] pipe the pipeline
    * \param[in] timeout_ms   Max time in milliseconds to wait until an exception will be thrown
    * \return Set of coherent frames
    */
    rs2_frame* rs2_pipeline_wait_for_frames(rs2_pipeline* pipe, unsigned int timeout_ms, rs2_error ** error);

    /**
    * Check if a new set of frames is available and retrieve the latest undelivered set.
    * The frames set includes time-synchronized frames of each enabled stream in the pipeline.
    * The method returns without blocking the calling thread, with status of new frames available or not. If available, it fetches the
    * latest frames set.
    * Device frames, which were produced while the function wasn't called, are dropped. To avoid frame drops, this method should be called
    * as fast as the device frame rate.
    * The application can maintain the frames handles to defer processing. However, if the application maintains too long history, the device
    * may lack memory resources to produce new frames, and the following calls to this method shall return no new frames, until resources are
    * retained.
    * \param[in] pipe the pipeline
    * \param[out] output_frame frame handle to be released using rs2_release_frame
    * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
    * \return true if new frame was stored to output_frame
    */
    int rs2_pipeline_poll_for_frames(rs2_pipeline* pipe, rs2_frame** output_frame, rs2_error ** error);


    /**
    * Delete a pipeline instance.
    * Upon destruction, the pipeline will implicitly stop itself
    * \param[in] pipeline to delete
    */
    void rs2_delete_pipeline(rs2_pipeline* pipe);

    /**
    * Start the pipeline streaming with its default configuration.
    * The pipeline streaming loop captures samples from the device, and delivers them to the attached computer vision modules
    * and processing blocks, according to each module requirements and threading model.
    * During the loop execution, the application can access the camera streams by calling \c wait_for_frames() or \c poll_for_frames().
    * The streaming loop runs until the pipeline is stopped.
    * Starting the pipeline is possible only when it is not started. If the pipeline was started, an exception is raised.
    *
    * \param[in] pipe    a pointer to an instance of the pipeline
    * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
    * \return             The actual pipeline device and streams profile, which was successfully configured to the streaming device.
    */
    rs2_pipeline_profile* rs2_pipeline_start(rs2_pipeline* pipe, rs2_error ** error);


    /**
    * Start the pipeline streaming according to the configuraion.
    * The pipeline streaming loop captures samples from the device, and delivers them to the attached computer vision modules
    * and processing blocks, according to each module requirements and threading model.
    * During the loop execution, the application can access the camera streams by calling \c wait_for_frames() or \c poll_for_frames().
    * The streaming loop runs until the pipeline is stopped.
    * Starting the pipeline is possible only when it is not started. If the pipeline was started, an exception is raised.
    * The pipeline selects and activates the device upon start, according to configuration or a default configuration.
    * When the rs2::config is provided to the method, the pipeline tries to activate the config \c resolve() result. If the application
    * requests are conflicting with pipeline computer vision modules or no matching device is available on the platform, the method fails.
    * Available configurations and devices may change between config \c resolve() call and pipeline start, in case devices are connected
    * or disconnected, or another application acquires ownership of a device.
    *
    * \param[in] pipe    a pointer to an instance of the pipeline
    * \param[in] config   A rs2::config with requested filters on the pipeline configuration. By default no filters are applied.
    * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
    * \return             The actual pipeline device and streams profile, which was successfully configured to the streaming device.
    */
    rs2_pipeline_profile* rs2_pipeline_start_with_config(rs2_pipeline* pipe, rs2_config* config, rs2_error ** error);

    /**
    * Return the active device and streams profiles, used by the pipeline.
    * The pipeline streams profiles are selected during \c start(). The method returns a valid result only when the pipeline is active -
    * between calls to \c start() and \c stop().
    * After \c stop() is called, the pipeline doesn't own the device, thus, the pipeline selected device may change in subsequent activations.
    *
    * \param[in] pipe    a pointer to an instance of the pipeline
    * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
    * \return  The actual pipeline device and streams profile, which was successfully configured to the streaming device on start.
    */
    rs2_pipeline_profile* rs2_pipeline_get_active_profile(rs2_pipeline* pipe, rs2_error ** error);

    /**
    * Retrieve the device used by the pipeline.
    * The device class provides the application access to control camera additional settings -
    * get device information, sensor options information, options value query and set, sensor specific extensions.
    * Since the pipeline controls the device streams configuration, activation state and frames reading, calling
    * the device API functions, which execute those operations, results in unexpected behavior.
    * The pipeline streaming device is selected during pipeline \c start(). Devices of profiles, which are not returned by
    * pipeline \c start() or \c get_active_profile(), are not guaranteed to be used by the pipeline.
    *
    * \param[in] pipe    A pointer to an instance of a pipeline profile
    * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
    * \return rs2_device* The pipeline selected device
    */
    rs2_device* rs2_pipeline_profile_get_device(rs2_pipeline_profile* profile, rs2_error ** error);

    /**
    * Return the selected streams profiles, which are enabled in this profile.
    *
    * \param[in] pipe    A pointer to an instance of a pipeline profile
    * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
    * \return   list of stream profiles
    */
    rs2_stream_profile_list* rs2_pipeline_profile_get_streams(rs2_pipeline_profile* profile, rs2_error** error);

    /**
    * Deletes an instance of a pipeline profile
    *
    * \param[in] pipe    A pointer to an instance of a pipeline profile
    */
    void rs2_delete_pipeline_profile(rs2_pipeline_profile* profile);

    /**
    * Create a config instance
    * The config allows pipeline users to request filters for the pipeline streams and device selection and configuration.
    * This is an optional step in pipeline creation, as the pipeline resolves its streaming device internally.
    * Config provides its users a way to set the filters and test if there is no conflict with the pipeline requirements
    * from the device. It also allows the user to find a matching device for the config filters and the pipeline, in order to
    * select a device explicitly, and modify its controls before streaming starts.
    *
    * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
    * \return rs2_config*  A pointer to a new config instance
    */
    rs2_config* rs2_create_config(rs2_error** error);

    /**
    * Deletes an instance of a config
    *
    * \param[in] pipe    A pointer to an instance of a config
    */
    void rs2_delete_config(rs2_config* config);

    /**
    * Enable a device stream explicitly, with selected stream parameters.
    * The method allows the application to request a stream with specific configuration. If no stream is explicitly enabled, the pipeline
    * configures the device and its streams according to the attached computer vision modules and processing blocks requirements, or
    * default configuration for the first available device.
    * The application can configure any of the input stream parameters according to its requirement, or set to 0 for don't care value.
    * The config accumulates the application calls for enable configuration methods, until the configuration is applied. Multiple enable
    * stream calls for the same stream with conflicting parameters override each other, and the last call is maintained.
    * Upon calling \c resolve(), the config checks for conflicts between the application configuration requests and the attached computer
    * vision modules and processing blocks requirements, and fails if conflicts are found. Before \c resolve() is called, no conflict
    * check is done.
    *
    * \param[in] config    A pointer to an instance of a config
    * \param[in] stream    Stream type to be enabled
    * \param[in] index     Stream index, used for multiple streams of the same type. -1 indicates any.
    * \param[in] width     Stream image width - for images streams. 0 indicates any.
    * \param[in] height    Stream image height - for images streams. 0 indicates any.
    * \param[in] format    Stream data format - pixel format for images streams, of data type for other streams. RS2_FORMAT_ANY indicates any.
    * \param[in] framerate Stream frames per second. 0 indicates any.
    * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
    */
    void rs2_config_enable_stream(rs2_config* config,
        rs2_stream stream,
        int index,
        int width,
        int height,
        rs2_format format,
        int framerate,
        rs2_error** error);

    /**
    * Enable all device streams explicitly.
    * The conditions and behavior of this method are similar to those of \c enable_stream().
    * This filter enables all raw streams of the selected device. The device is either selected explicitly by the application,
    * or by the pipeline requirements or default. The list of streams is device dependent.
    *
    * \param[in] config    A pointer to an instance of a config
    * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
    */
    void rs2_config_enable_all_stream(rs2_config* config, rs2_error ** error);

    /**
    * Select a specific device explicitly by its serial number, to be used by the pipeline.
    * The conditions and behavior of this method are similar to those of \c enable_stream().
    * This method is required if the application needs to set device or sensor settings prior to pipeline streaming, to enforce
    * the pipeline to use the configured device.
    *
    * \param[in] config    A pointer to an instance of a config
    * \param[in] Serial device serial number, as returned by RS2_CAMERA_INFO_SERIAL_NUMBER
    * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
    */
    void rs2_config_enable_device(rs2_config* config, const char* serial, rs2_error ** error);

    /**
    * Select a recorded device from a file, to be used by the pipeline through playback.
    * The device available streams are as recorded to the file, and \c resolve() considers only this device and configuration
    * as available.
    * This request cannot be used if enable_record_to_file() is called for the current config, and vise versa
    *
    * \param[in] config    A pointer to an instance of a config
    * \param[in] file_name  The playback file of the device
    * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
    */
    void rs2_config_enable_device_from_file(rs2_config* config, const char* file, rs2_error ** error);

    /**
    * Requires that the resolved device would be recorded to file
    * This request cannot be used if enable_device_from_file() is called for the current config, and vise versa
    *
    * \param[in] config    A pointer to an instance of a config
    * \param[in] file_name  The desired file for the output record
    * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
    */
    void rs2_config_enable_record_to_file(rs2_config* config, const char* file, rs2_error ** error);


    /**
    * Disable a device stream explicitly, to remove any requests on this stream type.
    * The stream can still be enabled due to pipeline computer vision module request. This call removes any filter on the
    * stream configuration.
    *
    * \param[in] config    A pointer to an instance of a config
    * \param[in] stream    Stream type, for which the filters are cleared
    * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
    */
    void rs2_config_disable_stream(rs2_config* config, rs2_stream stream, rs2_error ** error);

    /**
    * Disable a device stream explicitly, to remove any requests on this stream profile.
    * The stream can still be enabled due to pipeline computer vision module request. This call removes any filter on the
    * stream configuration.
    *
    * \param[in] config    A pointer to an instance of a config
    * \param[in] stream    Stream type, for which the filters are cleared
    * \param[in] index     Stream index, for which the filters are cleared
    * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
    */
    void rs2_config_disable_indexed_stream(rs2_config* config, rs2_stream stream, int index, rs2_error ** error);

    /**
    * Disable all device stream explicitly, to remove any requests on the streams profiles.
    * The streams can still be enabled due to pipeline computer vision module request. This call removes any filter on the
    * streams configuration.
    *
    * \param[in] config    A pointer to an instance of a config
    * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
    */
    void rs2_config_disable_all_streams(rs2_config* config, rs2_error ** error);

    /**
    * Resolve the configuration filters, to find a matching device and streams profiles.
    * The method resolves the user configuration filters for the device and streams, and combines them with the requirements of
    * the computer vision modules and processing blocks attached to the pipeline. If there are no conflicts of requests, it looks
    * for an available device, which can satisfy all requests, and selects the first matching streams configuration. In the absence
    * of any request, the rs2::config selects the first available device and the first color and depth streams configuration.
    * The pipeline profile selection during \c start() follows the same method. Thus, the selected profile is the same, if no
    * change occurs to the available devices occurs.
    * Resolving the pipeline configuration provides the application access to the pipeline selected device for advanced control.
    * The returned configuration is not applied to the device, so the application doesn't own the device sensors. However, the
    * application can call \c enable_device(), to enforce the device returned by this method is selected by pipeline \c start(),
    * and configure the device and sensors options or extensions before streaming starts.
    *
    * \param[in] config    A pointer to an instance of a config
    * \param[in] p  The pipeline for which the selected filters are applied
    * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
    * \return       A matching device and streams profile, which satisfies the filters and pipeline requests.
    */
    rs2_pipeline_profile* rs2_config_resolve(rs2_config* config, rs2_pipeline* pipe, rs2_error ** error);

    /**
    * Check if the config can resolve the configuration filters, to find a matching device and streams profiles.
    * The resolution conditions are as described in \c resolve().
    *
    * \param[in] config    A pointer to an instance of a config
    * \param[in] p  The pipeline for which the selected filters are applied
    * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
    * \return       True if a valid profile selection exists, false if no selection can be found under the config filters and the available devices.
    */
    int rs2_config_can_resolve(rs2_config* config, rs2_pipeline* pipe, rs2_error ** error);

#ifdef __cplusplus
}
#endif
#endif
