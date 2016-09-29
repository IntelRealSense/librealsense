// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RSCORE_HPP
#define LIBREALSENSE_RSCORE_HPP

#include "rs.h"
#include <cstdint>
#include <cstddef>
#include <mutex>

// this interface is an entry point for extending librealsense with custom devices while keeping librealsense public API
// the interfaces in this file are expected to be inherited from, never used directly, rs.hpp provides safer and simpler wrapper
// an example usage would be playback and record. this API allows to integrate custom playback&record implementation while keeping librealsense API

struct rs_stream_interface
{
    virtual                                 ~rs_stream_interface() {}

    virtual rs_extrinsics                   get_extrinsics_to(const rs_stream_interface & r) const = 0;
    virtual float                           get_depth_scale() const = 0;
    
    virtual rs_intrinsics                   get_intrinsics() const = 0;
    virtual rs_intrinsics                   get_rectified_intrinsics() const = 0;
    virtual rs_format                       get_format() const = 0;
    virtual int                             get_framerate() const = 0;

    virtual double                          get_frame_metadata(rs_frame_metadata frame_metadata) const = 0;
    virtual bool                            supports_frame_metadata(rs_frame_metadata frame_metadata) const = 0;
    virtual int                             get_frame_stride() const = 0;
    virtual int                             get_frame_bpp() const = 0;
    virtual unsigned long long              get_frame_number() const = 0;
    virtual double                          get_frame_timestamp() const = 0;
    virtual long long                       get_frame_system_time() const = 0;
    virtual const uint8_t *                 get_frame_data() const = 0;

    virtual int                             get_mode_count() const = 0;
    virtual void                            get_mode(int mode, int * w, int * h, rs_format * f, int * fps) const = 0;

    virtual bool                            is_enabled() const = 0;
    virtual rs_stream                       get_stream_type() const = 0;
};

struct rs_frame_ref
{
    virtual                                 ~rs_frame_ref() {}
    virtual const uint8_t*                  get_frame_data() const = 0;
    virtual double                          get_frame_timestamp() const = 0;
    virtual rs_timestamp_domain             get_frame_timestamp_domain() const = 0;
    virtual unsigned long long              get_frame_number() const = 0;
    virtual long long                       get_frame_system_time() const = 0;
    virtual int                             get_frame_width() const = 0;
    virtual int                             get_frame_height() const = 0;
    virtual int                             get_frame_framerate() const = 0;
    virtual int                             get_frame_stride() const = 0;
    virtual int                             get_frame_bpp() const = 0;
    virtual rs_format                       get_frame_format() const = 0;
    virtual rs_stream                       get_stream_type() const = 0;
    virtual double                          get_frame_metadata(rs_frame_metadata frame_metadata) const = 0;
    virtual bool                            supports_frame_metadata(rs_frame_metadata frame_metadata) const = 0;
};

// realsense device public interface
struct rs_device
{
    virtual                                 ~rs_device() {}
    virtual const rs_stream_interface &     get_stream_interface(rs_stream stream) const = 0;

    virtual const char *                    get_name() const = 0;
    virtual const char *                    get_serial() const = 0;
    virtual const char *                    get_firmware_version() const = 0;
    virtual const char *                    get_camera_info(rs_camera_info) const = 0;
    virtual float                           get_depth_scale() const = 0;
                                            
    virtual void                            enable_stream(rs_stream stream, int width, int height, rs_format format, int fps, rs_output_buffer_format output) = 0;
    virtual void                            enable_stream_preset(rs_stream stream, rs_preset preset) = 0;
    virtual void                            disable_stream(rs_stream stream) = 0;
                                            
    virtual void                            enable_motion_tracking() = 0;
    virtual void                            set_stream_callback(rs_stream stream, void(*on_frame)(rs_device * device, rs_frame_ref * frame, void * user), void * user) = 0;
    virtual void                            set_stream_callback(rs_stream stream, rs_frame_callback * callback) = 0;
    virtual void                            disable_motion_tracking() = 0;

    virtual rs_motion_intrinsics            get_motion_intrinsics() const = 0;
    virtual rs_extrinsics                   get_motion_extrinsics_from(rs_stream from) const = 0;
    virtual void                            set_motion_callback(void(*on_event)(rs_device * device, rs_motion_data data, void * user), void * user) = 0;
    virtual void                            set_motion_callback(rs_motion_callback * callback) = 0;
    virtual void                            set_timestamp_callback(void(*on_event)(rs_device * device, rs_timestamp_data data, void * user), void * user) = 0;
    virtual void                            set_timestamp_callback(rs_timestamp_callback * callback) = 0;
                                            
    virtual void                            start(rs_source source) = 0;
    virtual void                            stop(rs_source source) = 0;

    virtual void                            start_fw_logger(char fw_log_op_code, int grab_rate_in_ms, std::timed_mutex& mutex) = 0;
    virtual void                            stop_fw_logger() = 0;

    virtual bool                            is_capturing() const = 0;
    virtual int                             is_motion_tracking_active() const = 0;
                                            
    virtual void                            wait_all_streams() = 0;
    virtual bool                            poll_all_streams() = 0;
                                            
    virtual bool                            supports(rs_capabilities capability) const = 0;
    virtual bool                            supports(rs_camera_info info_param) const = 0;

    virtual bool                            supports_option(rs_option option) const = 0;
    virtual void                            get_option_range(rs_option option, double & min, double & max, double & step, double & def) = 0;
    virtual void                            set_options(const rs_option options[], size_t count, const double values[]) = 0;
    virtual void                            get_options(const rs_option options[], size_t count, double values[]) = 0;
    virtual const char *                    get_option_description(rs_option option) const = 0;

    virtual void                            release_frame(rs_frame_ref * ref) = 0;
    virtual rs_frame_ref *                  clone_frame(rs_frame_ref * frame) = 0;

    virtual const char *                    get_usb_port_id() const = 0;
};

struct rs_context
{
    virtual size_t                          get_device_count() const = 0;
    virtual rs_device *                     get_device(int index) const = 0;
    virtual                                 ~rs_context() {}
};

struct rs_motion_callback
{
    virtual void                            on_event(rs_motion_data e) = 0;
    virtual void                            release() = 0;
    virtual                                 ~rs_motion_callback() {}
};

struct rs_frame_callback
{
    virtual void                            on_frame(rs_device * device, rs_frame_ref * f) = 0;
    virtual void                            release() = 0;
    virtual                                 ~rs_frame_callback() {}
};

struct rs_timestamp_callback
{
    virtual void                            on_event(rs_timestamp_data data) = 0;
    virtual void                            release() = 0;
    virtual                                 ~rs_timestamp_callback() {}
};

struct rs_log_callback
{
    virtual void                            on_event(rs_log_severity severity, const char * message) = 0;
    virtual void                            release() = 0;
    virtual                                 ~rs_log_callback() {}
};

#endif
