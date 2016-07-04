// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_DEVICE_H
#define LIBREALSENSE_DEVICE_H

#include "uvc.h"
#include "stream.h"
#include <chrono>


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

        void activate() { if (!active) { struct_ = reader(); active = true; } }
        template<class U> double get(U T::* field) { activate(); return static_cast<double>(struct_.*field); }
        template<class U, class V> void set(U T::* field, V value) { activate(); struct_.*field = static_cast<U>(value); }
        void commit() { if (active) writer(struct_); }
    };

    template<class T, class R, class W> struct_interface<T, R, W> make_struct_interface(R r, W w) { return{ r,w }; }
        
    struct frame_timestamp_reader
    {
        virtual bool validate_frame(const subdevice_mode & mode, const void * frame) const = 0;
        virtual int get_frame_timestamp(const subdevice_mode & mode, const void * frame) = 0;
        virtual int get_frame_counter(const subdevice_mode &, const void * frame) = 0;
    };

    namespace motion_module
    {
        struct motion_module_parser;
    }
    
}

struct rs_device_base : rs_device
{
private:
    const std::shared_ptr<rsimpl::uvc::device>  device;
protected:
    rsimpl::device_config                       config;
private:
    rsimpl::native_stream                       depth, color, infrared, infrared2, fisheye;
    rsimpl::point_stream                        points;
    rsimpl::rectified_stream                    rect_color;
    rsimpl::aligned_stream                      color_to_depth, depth_to_color, depth_to_rect_color, infrared2_to_depth, depth_to_infrared2;
    rsimpl::native_stream *                     native_streams[RS_STREAM_NATIVE_COUNT];
    rsimpl::stream_interface *                  streams[RS_STREAM_COUNT];

    bool                                        capturing;
    bool                                        data_acquisition_active;
    std::chrono::high_resolution_clock::time_point capture_started;

    std::shared_ptr<rsimpl::syncronizing_archive> archive;
protected:
    const rsimpl::uvc::device &                 get_device() const { return *device; }
    rsimpl::uvc::device &                       get_device() { return *device; }

    virtual void                                start_video_streaming();
    virtual void                                stop_video_streaming();
    virtual void                                start_motion_tracking();
    virtual void                                stop_motion_tracking();

public:
                                                rs_device_base(std::shared_ptr<rsimpl::uvc::device> device, const rsimpl::static_device_info & info);
                                                virtual ~rs_device_base();

    const rsimpl::stream_interface &            get_stream_interface(rs_stream stream) const override { return *streams[stream]; }

    const char *                                get_name() const override { return config.info.name.c_str(); }
    const char *                                get_serial() const override { return config.info.serial.c_str(); }
    const char *                                get_firmware_version() const override { return config.info.firmware_version.c_str(); }
    float                                       get_depth_scale() const override { return config.depth_scale; }

    void                                        enable_stream(rs_stream stream, int width, int height, rs_format format, int fps, rs_output_buffer_format output) override;
    void                                        enable_stream_preset(rs_stream stream, rs_preset preset) override;
    void                                        disable_stream(rs_stream stream) override;

    void                                        enable_motion_tracking() override;
    void                                        set_stream_callback(rs_stream stream, void(*on_frame)(rs_device * device, rs_frame_ref * frame, void * user), void * user) override;
    void                                        set_stream_callback(rs_stream stream, rs_frame_callback * callback) override;
    void                                        disable_motion_tracking() override;

    void                                        set_motion_callback(rs_motion_callback * callback) override;
    void                                        set_motion_callback(void(*on_event)(rs_device * device, rs_motion_data data, void * user), void * user) override;
    void                                        set_timestamp_callback(void(*on_event)(rs_device * device, rs_timestamp_data data, void * user), void * user) override;
    void                                        set_timestamp_callback(rs_timestamp_callback * callback) override;

    virtual void                                start(rs_source source) override;
    virtual void                                stop(rs_source source) override;

    bool                                        is_capturing() const override { return capturing; }
    int                                         is_motion_tracking_active() const override { return data_acquisition_active; }
    
    void                                        wait_all_streams() override;
    bool                                        poll_all_streams() override;

    rs_frameset *                               wait_all_streams_safe() override;
    bool                                        poll_all_streams_safe(rs_frameset ** frames) override;
    void                                        release_frames(rs_frameset * frameset) override;
    rs_frameset *                               clone_frames(rs_frameset * frameset) override;
    
    virtual bool                                supports(rs_capabilities capability) const override;

    virtual bool                                supports_option(rs_option option) const override;
    virtual void                                get_option_range(rs_option option, double & min, double & max, double & step, double & def) override;
    virtual void                                disable_auto_option(int subdevice, rs_option auto_opt) override;

    virtual void                                on_before_start(const std::vector<rsimpl::subdevice_mode_selection> & selected_modes) = 0;
    virtual rs_stream                           select_key_stream(const std::vector<rsimpl::subdevice_mode_selection> & selected_modes) = 0;
    virtual std::shared_ptr<rsimpl::frame_timestamp_reader>  create_frame_timestamp_reader() const = 0;
    rs_frame_ref *                              detach_frame(rs_frameset * fs, rs_stream stream) override;
    void                                        release_frame(rs_frame_ref * ref) override;
    const char *                                get_usb_port_id() const override;
    rs_frame_ref *                              clone_frame(rs_frame_ref * frame) override;
};

#endif
