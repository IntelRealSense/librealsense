#pragma once
#ifndef LIBREALSENSE_DEVICE_H
#define LIBREALSENSE_DEVICE_H

#include "uvc.h"

struct rs_device
{
protected:
    class subdevice_handle;

    rsimpl::uvc::device                     device;
    const rsimpl::static_device_info        device_info;

    rsimpl::stream_request                  requests[RS_STREAM_COUNT];  // Indexed by RS_DEPTH, RS_COLOR, ...
    std::shared_ptr<rsimpl::stream_buffer>  streams[RS_STREAM_COUNT];   // Indexed by RS_DEPTH, RS_COLOR, ...

    bool                                    capturing;
    std::chrono::high_resolution_clock::time_point capture_started;  

    int64_t                                 base_timestamp;
    int                                     last_stream_timestamp;
public:
                                            rs_device(rsimpl::uvc::device device, const rsimpl::static_device_info & device_info);
                                            ~rs_device();

    const char *                            get_name() const { return device_info.name.c_str(); }
    bool                                    supports_option(rs_option option) const { return device_info.option_supported[option]; }
    rs_extrinsics                           get_stream_extrinsics(rs_stream from, rs_stream to) const;
    float                                   get_depth_scale() const { return device_info.depth_scale; }

    void                                    enable_stream(rs_stream stream, int width, int height, rs_format format, int fps);
    void                                    enable_stream_preset(rs_stream stream, rs_preset preset);    
    bool                                    is_stream_enabled(rs_stream stream) const { return requests[stream].enabled; }
    rsimpl::stream_mode                     get_stream_mode(rs_stream stream) const;
    rs_intrinsics                           get_stream_intrinsics(rs_stream stream) const { return device_info.intrinsics[get_stream_mode(stream).intrinsics_index]; }
    rs_format                               get_stream_format(rs_stream stream) const { return get_stream_mode(stream).format; }
    int                                     get_stream_framerate(rs_stream stream) const { return get_stream_mode(stream).fps; }

    void                                    start();
    void                                    stop();
    bool                                    is_capturing() const { return capturing; }
    
    void                                    wait_all_streams();
    int                                     get_frame_timestamp(rs_stream stream) const;
    const void *                            get_frame_data(rs_stream stream) const { if(!streams[stream]) throw std::runtime_error("stream not enabled"); return streams[stream]->get_front_data(); } 

    virtual void                            set_stream_intent() = 0;
    virtual void                            set_option(rs_option option, int value) = 0;
    virtual int                             get_option(rs_option option) const = 0;
    virtual int                             convert_timestamp(int64_t timestamp) const = 0;
};

#endif
