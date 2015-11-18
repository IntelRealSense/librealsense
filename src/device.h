#pragma once
#ifndef LIBREALSENSE_DEVICE_H
#define LIBREALSENSE_DEVICE_H

#include "uvc.h"
#include "stream.h"
#include <chrono>

struct rs_device
{
private:
    const std::shared_ptr<rsimpl::uvc::device>  device;
protected:
    rsimpl::device_config                       config;
private:
    rsimpl::native_stream                       depth, color, infrared, infrared2;
    rsimpl::rectified_stream                    rect_color;
    rsimpl::aligned_stream                      color_to_depth, depth_to_color, depth_to_rect_color;
    rsimpl::native_stream *                     native_streams[RS_STREAM_NATIVE_COUNT];
    rsimpl::stream_interface *                  streams[RS_STREAM_COUNT];

    bool                                        capturing;
    std::chrono::high_resolution_clock::time_point capture_started;  

    int64_t                                     base_timestamp;
    int                                         last_stream_timestamp;
protected:
    const rsimpl::uvc::device &                 get_device() const { return *device; }
    rsimpl::uvc::device &                       get_device() { return *device; }
public:
                                                rs_device(std::shared_ptr<rsimpl::uvc::device> device, const rsimpl::static_device_info & info);
                                                ~rs_device();

    const rsimpl::stream_interface &            get_stream_interface(rs_stream stream) const { return *streams[stream]; }

    const char *                                get_name() const { return config.info.name.c_str(); }
    const char *                                get_serial() const { return config.info.serial.c_str(); }
    const char *                                get_firmware_version() const { return config.info.firmware_version.c_str(); }
    float                                       get_depth_scale() const { return config.depth_scale; }
    bool                                        supports_option(rs_option option) const { return config.info.option_supported[option]; }
    void                                        get_option_range(rs_option option, int * min, int * max) const;

    void                                        enable_stream(rs_stream stream, int width, int height, rs_format format, int fps);
    void                                        enable_stream_preset(rs_stream stream, rs_preset preset);    
    void                                        disable_stream(rs_stream stream);

    void                                        start();
    void                                        stop();
    bool                                        is_capturing() const { return capturing; }
    
    void                                        wait_all_streams();
    int                                         get_frame_timestamp(rs_stream stream) const;
    const rsimpl::byte *                        get_frame_data(rs_stream stream) const;

    void                                        set_option(rs_option option, int value);
    int                                         get_option(rs_option option) const;

    virtual void                                on_before_start(const std::vector<rsimpl::subdevice_mode> & selected_modes) {}
    virtual void                                get_xu_range(rs_option option, int * min, int * max) const = 0;
    virtual void                                set_xu_option(rs_option option, int value) = 0;
    virtual int                                 get_xu_option(rs_option option) const = 0;
    virtual int                                 convert_timestamp(int64_t timestamp) const = 0;
};

#endif
