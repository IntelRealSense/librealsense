// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include "core/roi.h"
#include "core/extension.h"
#include "core/serialization.h"
#include "core/streaming.h"
#include "archive.h"
#include "concurrency.h"
#include "sensor.h"

namespace librealsense
{
    class record_sensor : public sensor_interface,
                          public extendable_interface,  //Allows extension for any of the given device's extensions
                          public info_container,
                          public options_container
    {
    public:
        using frame_interface_callback_t = std::function<void(frame_holder, std::function<void(const std::string&)>)>;
        using snapshot_callback_t = std::function<void(rs2_extension, const std::shared_ptr<extension_snapshot>&, std::function<void(const std::string&)>)>;

        record_sensor(const device_interface& device,
                      sensor_interface& sensor,
                      frame_interface_callback_t on_frame,
                      snapshot_callback_t on_snapshot);
        virtual ~record_sensor();

        stream_profiles get_stream_profiles() const override;
        void open(const stream_profiles& requests) override;
        void close() override;
        option& get_option(rs2_option id) override;
        const option& get_option(rs2_option id) const override;
        const std::string& get_info(rs2_camera_info info) const override;
        bool supports_info(rs2_camera_info info) const override;
        bool supports_option(rs2_option id) const override;
        void register_notifications_callback(notifications_callback_ptr callback) override;
        void start(frame_callback_ptr callback) override;
        void stop() override;
        bool is_streaming() const override;
        bool extend_to(rs2_extension extension_type, void** ext) override;
        const device_interface& get_device() override;

    private /*methods*/:
        void raise_user_notification(const std::string& str);
        template <typename T> void record_snapshot(rs2_extension extension_type, const  recordable<T>& snapshot);
        template <rs2_extension E, typename P> bool extend_to_aux(P* p, void** ext);
        void stop_with_error(const std::string& basic_string);
        void record_frame(frame_holder holder);

    private /*members*/:
        snapshot_callback_t m_device_record_snapshot_handler;
        sensor_interface& m_sensor;
        std::set<rs2_option> m_recording_options;
        librealsense::notifications_callback_ptr m_user_notification_callback;
        frame_interface_callback_t m_record_callback;
        std::atomic_bool m_is_recording;
        bool m_is_pause;
        frame_callback_ptr m_frame_callback;
        const device_interface& m_parent_device;

    };

    class notification_callback : public rs2_notifications_callback
    {
        std::function<void(rs2_notification*)> on_notification_function;
    public:
        notification_callback(std::function<void(rs2_notification*)>  on_notification) : on_notification_function(on_notification) {}

        void on_notification(rs2_notification* _notification) override
        {
            on_notification_function(_notification);
        }
        void release() override { delete this; }
    };
    class frame_holder_callback : public rs2_frame_callback
    {
        std::function<void(frame_holder)> on_frame_function;
    public:
        explicit frame_holder_callback(std::function<void(frame_holder)> on_frame) : on_frame_function(on_frame) {}

        void on_frame(rs2_frame * fref) override
        {
            on_frame_function({ (frame_interface*)fref });
        }

        void release() override { delete this; }
    };

}