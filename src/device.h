/*
    INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
    terms of a license agreement or nondisclosure agreement with Intel Corporation
    and may not be copied or disclosed except in accordance with the terms of that
    agreement.
    Copyright(c) 2015 Intel Corporation. All Rights Reserved.
*/

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
    rsimpl::point_stream                        points;
    rsimpl::rectified_stream                    rect_color;
    rsimpl::aligned_stream                      color_to_depth, depth_to_color, depth_to_rect_color, infrared2_to_depth, depth_to_infrared2;
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

    void                                        enable_stream(rs_stream stream, int width, int height, rs_format format, int fps);
    void                                        enable_stream_preset(rs_stream stream, rs_preset preset);    
    void                                        disable_stream(rs_stream stream);

    void                                        start();
    void                                        stop();
    bool                                        is_capturing() const { return capturing; }
    
    void                                        wait_all_streams();
    int                                         get_frame_timestamp(rs_stream stream) const;
    const rsimpl::byte *                        get_frame_data(rs_stream stream) const;
    
    virtual bool                                supports_option(rs_option option) const;
    virtual void                                get_option_range(rs_option option, double & min, double & max, double & step);
    virtual void                                set_options(const rs_option options[], int count, const double values[]) {}
    virtual void                                get_options(const rs_option options[], int count, double values[]) {}

    virtual void                                on_before_start(const std::vector<rsimpl::subdevice_mode_selection> & selected_modes) {}
    virtual int                                 convert_timestamp(int64_t timestamp) const = 0;
};

namespace rsimpl
{
    // This class is used to buffer up several writes to a structure-valued XU control, and send the entire structure all at once
    // Additionally, it will ensure that any fields not set in a given struct will retain their original values
    template<class T, class R, class W> struct struct_interface
    {
        T struct_;
        R reader;
        W writer;        
        bool active;

        struct_interface(R r, W w) : reader(r), writer(w), active(false) {}

        void activate() { if(!active) { struct_ = reader(); active = true; } }
        template<class U> double get(U T::* field) { activate(); return struct_.*field; }
        template<class U> void set(U T::* field, double value) { activate(); struct_.*field = static_cast<U>(value); }
        void commit() { if(active) writer(struct_); }
    };

    template<class T, class R, class W> struct_interface<T,R,W> make_struct_interface(R r, W w) { return {r,w}; }
}

#endif
