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
        record_sensor(const device_interface& device,
                      sensor_interface& sensor);
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
        notifications_callback_ptr get_notifications_callback() const override;
        void start(frame_callback_ptr callback) override;
        void stop() override;
        bool is_streaming() const override;
        bool extend_to(rs2_extension extension_type, void** ext) override;
        const device_interface& get_device() override;
        signal<record_sensor, const notification&> on_notification;
        signal<record_sensor, frame_holder> on_frame;
        signal<record_sensor, rs2_extension, std::shared_ptr<extension_snapshot>> on_extension_change;
        void stop_with_error(const std::string& message);
        void disable_recording();
    private /*methods*/:
        template <typename T> void record_snapshot(rs2_extension extension_type, const T& snapshot);
        template <rs2_extension E, typename P> bool extend_to_aux(P* p, void** ext);
        void record_frame(frame_holder holder);
        
    private /*members*/:
        sensor_interface& m_sensor;
        std::set<rs2_option> m_recording_options;
        librealsense::notifications_callback_ptr m_user_notification_callback;
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