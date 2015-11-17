#pragma once
#ifndef LIBREALSENSE_STREAM_H
#define LIBREALSENSE_STREAM_H

#include "types.h"

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

    rs_extrinsics                               get_extrinsics_to(const stream_interface & r) const;
    virtual rsimpl::pose                        get_pose() const = 0;
    virtual float                               get_depth_scale() const = 0;

    virtual bool                                is_enabled() const = 0;
    virtual rs_intrinsics                       get_intrinsics() const = 0;
    virtual rs_format                           get_format() const = 0;
    virtual int                                 get_framerate() const = 0;

    virtual int                                 get_frame_number() const = 0;
    virtual const rsimpl::byte *                get_frame_data() const = 0;    
};

struct native_stream : public stream_interface
{
    const device_config &                       config;
    const rs_stream                             stream;
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

class rectified_stream : public stream_interface
{
    std::shared_ptr<const stream_interface>     source;
    mutable std::vector<int>                    table;
    mutable std::vector<rsimpl::byte>           image;
    mutable int                                 number;
public:
                                                rectified_stream(std::shared_ptr<const stream_interface> source) : source(source), number() {}

    rsimpl::pose                                get_pose() const { return {{{1,0,0},{0,1,0},{0,0,1}}, source->get_pose().position}; }
    float                                       get_depth_scale() const { return source->get_depth_scale(); }

    bool                                        is_enabled() const { return source->is_enabled(); }
    rs_intrinsics                               get_intrinsics() const { auto i = source->get_intrinsics(); i.model = RS_DISTORTION_NONE; for(auto & f : i.coeffs) f = 0; return i; }
    rs_format                                   get_format() const { return source->get_format(); }
    int                                         get_framerate() const { return source->get_framerate(); }

    int                                         get_frame_number() const { return source->get_frame_number(); }
    const rsimpl::byte *                        get_frame_data() const;
};

class aligned_stream : public stream_interface
{
    std::shared_ptr<const stream_interface>     from, to;
    mutable std::vector<rsimpl::byte>           image;
    mutable int                                 number;
public:
                                                aligned_stream(std::shared_ptr<const stream_interface> from, std::shared_ptr<const stream_interface> to) : from(from), to(to), number() {}

    rsimpl::pose                                get_pose() const { return to->get_pose(); }
    float                                       get_depth_scale() const { return to->get_depth_scale(); }

    bool                                        is_enabled() const { return from->is_enabled() && to->is_enabled(); }
    rs_intrinsics                               get_intrinsics() const { return to->get_intrinsics(); }
    rs_format                                   get_format() const { return from->get_format(); }
    int                                         get_framerate() const { return from->get_framerate(); }

    int                                         get_frame_number() const { return from->get_frame_number(); }
    const rsimpl::byte *                        get_frame_data() const;
};

#endif