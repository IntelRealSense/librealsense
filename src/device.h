#pragma once
#ifndef LIBREALSENSE_DEVICE_H
#define LIBREALSENSE_DEVICE_H

#include "uvc.h"
#include <chrono>

struct device_config
{
    const rsimpl::static_device_info            info;
    rsimpl::intrinsics_buffer                   intrinsics;
    rsimpl::stream_request                      requests[RS_STREAM_NATIVE_COUNT];  // Modified by enable/disable_stream calls

                                                device_config(const rsimpl::static_device_info & info) : info(info) { for(auto & req : requests) req = rsimpl::stream_request(); }

    std::vector<rsimpl::subdevice_mode>         select_modes() const { return info.select_modes(requests); }
};

struct stream_interface
{
    virtual                                     ~stream_interface() = default;

    virtual rsimpl::pose                        get_pose() const = 0;
    virtual float                               get_depth_scale() const = 0;

    virtual bool                                is_enabled() const = 0;
    virtual rs_intrinsics                       get_intrinsics() const = 0;
    virtual rs_format                           get_format() const = 0;
    virtual int                                 get_framerate() const = 0;

    virtual int                                 get_frame_number() const = 0;
    virtual const rsimpl::byte *                get_frame_data() const = 0;
};

struct native_stream : stream_interface
{
    device_config &                             config;
    rs_stream                                   stream;
    std::shared_ptr<rsimpl::stream_buffer>      buffer;

                                                native_stream(device_config & config, rs_stream stream) : config(config), stream(stream) {}

    rsimpl::pose                                get_pose() const { return config.info.stream_poses[stream]; }
    float                                       get_depth_scale() const { return config.info.depth_scale; }

    bool                                        is_enabled() const { return static_cast<bool>(buffer); }
    rsimpl::stream_mode                         get_mode() const;
    rs_intrinsics                               get_intrinsics() const { return config.intrinsics.get(get_mode().intrinsics_index); }
    rs_format                                   get_format() const { return get_mode().format; }
    int                                         get_framerate() const { return get_mode().fps; }

    int                                         get_frame_number() const { return buffer->get_front_number(); }
    const rsimpl::byte *                        get_frame_data() const { return buffer->get_front_data(); }
};

struct rectified_stream : stream_interface
{
    std::shared_ptr<stream_interface>           source;
    mutable std::vector<int>                    table;
    mutable std::vector<rsimpl::byte>           image;
    mutable int                                 number;

                                                rectified_stream(std::shared_ptr<stream_interface> source) : source(source), number() {}

    rsimpl::pose                                get_pose() const { return {{{1,0,0},{0,1,0},{0,0,1}}, source->get_pose().position}; }
    float                                       get_depth_scale() const { return source->get_depth_scale(); }

    bool                                        is_enabled() const { return source->is_enabled(); }
    rs_intrinsics                               get_intrinsics() const { auto i = source->get_intrinsics(); i.model = RS_DISTORTION_NONE; for(auto & f : i.coeffs) f = 0; return i; }
    rs_format                                   get_format() const { return source->get_format(); }
    int                                         get_framerate() const { return source->get_framerate(); }

    int                                         get_frame_number() const { return source->get_frame_number(); }
    const rsimpl::byte *                        get_frame_data() const;
};

struct aligned_stream : stream_interface
{
    std::shared_ptr<stream_interface>           from, to;
    mutable std::vector<rsimpl::byte>           image;
    mutable int                                 number;

                                                aligned_stream(std::shared_ptr<stream_interface> from, std::shared_ptr<stream_interface> to) : from(from), to(to), number() {}

    rsimpl::pose                                get_pose() const { return to->get_pose(); }
    float                                       get_depth_scale() const { return to->get_depth_scale(); }

    bool                                        is_enabled() const { return from->is_enabled() && to->is_enabled(); }
    rs_intrinsics                               get_intrinsics() const { return to->get_intrinsics(); }
    rs_format                                   get_format() const { return from->get_format(); }
    int                                         get_framerate() const { return from->get_framerate(); }

    int                                         get_frame_number() const { return from->get_frame_number(); }
    const rsimpl::byte *                        get_frame_data() const;
};

struct rs_device
{
private:
    const std::shared_ptr<rsimpl::uvc::device>  device;
    device_config                               config;

    std::shared_ptr<native_stream>              native_streams[RS_STREAM_NATIVE_COUNT];
    std::shared_ptr<stream_interface>           streams[RS_STREAM_COUNT];

    bool                                        capturing;
    std::chrono::high_resolution_clock::time_point capture_started;  

    int64_t                                     base_timestamp;
    int                                         last_stream_timestamp;
protected:
    const rsimpl::uvc::device &                 get_device() const { return *device; }
    rsimpl::uvc::device &                       get_device() { return *device; }
    void                                        set_intrinsics_thread_safe(std::vector<rs_intrinsics> new_intrinsics) { config.intrinsics.set(move(new_intrinsics)); }
public:
                                                rs_device(std::shared_ptr<rsimpl::uvc::device> device, const rsimpl::static_device_info & info);
                                                ~rs_device();

    const char *                                get_name() const { return config.info.name.c_str(); }
    const char *                                get_serial() const { return config.info.serial.c_str(); }
    const char *                                get_firmware_version() const { return config.info.firmware_version.c_str(); }
    rsimpl::pose                                get_pose(rs_stream stream) const;
    rs_extrinsics                               get_extrinsics(rs_stream from, rs_stream to) const;
    float                                       get_depth_scale() const { return config.info.depth_scale; }
    bool                                        supports_option(rs_option option) const { return config.info.option_supported[option]; }
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
    const rsimpl::byte *                        get_frame_data(rs_stream stream) const;

    void                                        set_option(rs_option option, int value);
    int                                         get_option(rs_option option) const;

    virtual void                                on_before_start(const std::vector<rsimpl::subdevice_mode> & selected_modes) {}
    virtual void                                set_xu_option(rs_option option, int value) = 0;
    virtual int                                 get_xu_option(rs_option option) const = 0;
    virtual int                                 convert_timestamp(int64_t timestamp) const = 0;
};

#endif
