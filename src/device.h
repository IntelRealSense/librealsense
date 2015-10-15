#pragma once
#ifndef LIBREALSENSE_DEVICE_H
#define LIBREALSENSE_DEVICE_H

#include "uvc.h"

struct rs_device
{
private:
    const std::shared_ptr<rsimpl::uvc::device>  device;
    const rsimpl::static_device_info            device_info;
    std::vector<rs_intrinsics>                  intrinsics;
    mutable std::mutex                          intrinsics_mutex;           // Controls access to intrinsics, mutable so that it can be locked from const methods which only read the value of intrinsics

    rsimpl::stream_request                      requests[RS_STREAM_NATIVE_COUNT];  // Modified by enable/disable_stream calls
    std::shared_ptr<rsimpl::stream_buffer>      streams[RS_STREAM_NATIVE_COUNT];   // Set up during start(), maintains buffers to receive frames from callback

    bool                                        capturing;
    std::chrono::high_resolution_clock::time_point capture_started;  

    int64_t                                     base_timestamp;
    int                                         last_stream_timestamp;

    mutable std::vector<uint8_t>                synthetic_images[RS_STREAM_COUNT - RS_STREAM_NATIVE_COUNT];
    mutable int                                 synthetic_timestamps[RS_STREAM_COUNT - RS_STREAM_NATIVE_COUNT];
protected:
    rsimpl::stream_mode                         get_current_stream_mode(rs_stream stream) const;
    const rsimpl::uvc::device &                 get_device() const { return *device; }
    rsimpl::uvc::device &                       get_device() { return *device; }
    void                                        set_intrinsics_thread_safe(std::vector<rs_intrinsics> new_intrinsics); // Thread-safe
public:
                                                rs_device(std::shared_ptr<rsimpl::uvc::device> device, const rsimpl::static_device_info & info);
                                                ~rs_device();

    const char *                                get_name() const { return device_info.name.c_str(); }
    const char *                                get_serial() const { return device_info.serial.c_str(); }
    const char *                                get_firmware_version() const { device_info.firmware_version.c_str(); }
    rs_extrinsics                               get_extrinsics(rs_stream from, rs_stream to) const;
    float                                       get_depth_scale() const { return device_info.depth_scale; }
    bool                                        supports_option(rs_option option) const { return device_info.option_supported[option]; }
    int                                         get_stream_mode_count(rs_stream stream) const;
    void                                        get_stream_mode(rs_stream stream, int mode, int * width, int * height, rs_format * format, int * framerate) const;

    void                                        enable_stream(rs_stream stream, int width, int height, rs_format format, int fps);
    void                                        enable_stream_preset(rs_stream stream, rs_preset preset);    
    void                                        disable_stream(rs_stream stream);
    bool                                        is_stream_enabled(rs_stream stream) const;

    rs_intrinsics                               get_stream_intrinsics(rs_stream stream) const;
    rs_format                                   get_stream_format(rs_stream stream) const;
    int                                         get_stream_framerate(rs_stream stream) const;

    void                                        start();
    void                                        stop();
    bool                                        is_capturing() const { return capturing; }
    
    void                                        wait_all_streams();
    int                                         get_frame_timestamp(rs_stream stream) const;
    const void *                                get_frame_data(rs_stream stream) const;

    void                                        set_option(rs_option option, int value);
    int                                         get_option(rs_option option) const;

    virtual void                                on_before_start(const std::vector<rsimpl::subdevice_mode> & selected_modes) {}
    virtual void                                set_xu_option(rs_option option, int value) = 0;
    virtual int                                 get_xu_option(rs_option option) const = 0;
    virtual int                                 convert_timestamp(int64_t timestamp) const = 0;
};

#endif
