// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_DEVICE_H
#define LIBREALSENSE_DEVICE_H

#include "uvc.h"
#include "stream.h"
#include <chrono>
#include <memory>
#include <vector>

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

    template <typename T>
    class wraparound_mechanism
    {
    public:
        wraparound_mechanism(T min_value, T max_value)
            : max_number(max_value - min_value + 1), last_number(min_value), num_of_wraparounds(0)
        {}

        T fix(T number)
        {
            if ((number + (num_of_wraparounds*max_number)) < last_number)
                ++num_of_wraparounds;


            number += (num_of_wraparounds*max_number);
            last_number = number;
            return number;
        }

    private:
        T max_number;
        T last_number;
        unsigned long long num_of_wraparounds;
    };

    struct frame_timestamp_reader
    {
        virtual bool validate_frame(const subdevice_mode & mode, const void * frame) = 0;
        virtual double get_frame_timestamp(const subdevice_mode & mode, const void * frame, double actual_fps) = 0;
        virtual unsigned long long get_frame_counter(const subdevice_mode & mode, const void * frame) = 0;
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
    std::atomic<uint32_t>                       max_publish_list_size;
    std::atomic<uint32_t>                       event_queue_size;
    std::atomic<uint32_t>                       events_timeout;
    std::shared_ptr<rsimpl::syncronizing_archive> archive;

    mutable std::string                         usb_port_id;
    mutable std::mutex                          usb_port_mutex;

    std::shared_ptr<std::thread>                fw_logger;

protected:
    const rsimpl::uvc::device &                 get_device() const { return *device; }
    rsimpl::uvc::device &                       get_device() { return *device; }

    virtual void                                start_video_streaming();
    virtual void                                stop_video_streaming();
    virtual void                                start_motion_tracking();
    virtual void                                stop_motion_tracking();

    virtual void                                disable_auto_option(int subdevice, rs_option auto_opt);
    virtual void                                on_before_callback(rs_stream, rs_frame_ref *, std::shared_ptr<rsimpl::frame_archive>) { }

    bool                                        motion_module_ready;
    std::atomic<bool>                           keep_fw_logger_alive;
    
    std::atomic<int>                            frames_drops_counter;

public:
    rs_device_base(std::shared_ptr<rsimpl::uvc::device> device, const rsimpl::static_device_info & info, rsimpl::calibration_validator validator = rsimpl::calibration_validator());
    virtual ~rs_device_base();

    const rsimpl::stream_interface &            get_stream_interface(rs_stream stream) const override { return *streams[stream]; }

    const char *                                get_name() const override { return config.info.name.c_str(); }
    const char *                                get_serial() const override { return config.info.serial.c_str(); }
    const char *                                get_firmware_version() const override { return config.info.firmware_version.c_str(); }
    float                                       get_depth_scale() const override { return config.depth_scale; }

    const char*                                 get_camera_info(rs_camera_info info) const override;

    void                                        enable_stream(rs_stream stream, int width, int height, rs_format format, int fps, rs_output_buffer_format output) override;
    void                                        enable_stream_preset(rs_stream stream, rs_preset preset) override;
    void                                        disable_stream(rs_stream stream) override;

    rs_motion_intrinsics                        get_motion_intrinsics() const override;
    rs_extrinsics                               get_motion_extrinsics_from(rs_stream from) const override;
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

    virtual void                                start_fw_logger(char fw_log_op_code, int grab_rate_in_ms, std::timed_mutex& mutex) override;
    virtual void                                stop_fw_logger() override;

    bool                                        is_capturing() const override { return capturing; }
    int                                         is_motion_tracking_active() const override { return data_acquisition_active; }

    void                                        wait_all_streams() override;
    bool                                        poll_all_streams() override;

    virtual bool                                supports(rs_capabilities capability) const override;
    virtual bool                                supports(rs_camera_info info_param) const override;

    virtual bool                                supports_option(rs_option option) const override;
    virtual void                                get_option_range(rs_option option, double & min, double & max, double & step, double & def) override;
    virtual void                                set_options(const rs_option options[], size_t count, const double values[]) override;
    virtual void                                get_options(const rs_option options[], size_t count, double values[])override;
    virtual void                                on_before_start(const std::vector<rsimpl::subdevice_mode_selection> & selected_modes) = 0;
    virtual rs_stream                           select_key_stream(const std::vector<rsimpl::subdevice_mode_selection> & selected_modes) = 0;
    virtual std::vector<std::shared_ptr<rsimpl::frame_timestamp_reader>> 
                                                create_frame_timestamp_readers() const = 0;
    void                                        release_frame(rs_frame_ref * ref) override;
    const char *                                get_usb_port_id() const override;
    rs_frame_ref *                              clone_frame(rs_frame_ref * frame) override;

    virtual void                                send_blob_to_device(rs_blob_type /*type*/, void * /*data*/, int /*size*/) { throw std::runtime_error("not supported!"); }
    static void                                 update_device_info(rsimpl::static_device_info& info);

    const char *                                get_option_description(rs_option option) const override;
};

#endif
