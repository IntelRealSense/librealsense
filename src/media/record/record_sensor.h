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
        using frame_interface_callback_t = std::function<void(frame_holder)>;

        record_sensor(const device_interface& device, sensor_interface& sensor, frame_interface_callback_t on_frame, std::function<void(rs2_extension, const std::shared_ptr<extension_snapshot>&)> on_snapshot);
        virtual ~record_sensor();

        std::vector<stream_profile> get_principal_requests() override;
        void open(const std::vector<stream_profile>& requests) override;
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
        rs2_extrinsics get_extrinsics_to(rs2_stream from, const sensor_interface& other, rs2_stream to) const  override;
        const std::vector<platform::stream_profile>& get_curr_configurations() const  override;
    private:
        void raise_user_notification(const std::string& str);
        void record_snapshot(rs2_extension extension_type, const std::shared_ptr<extension_snapshot>& snapshot);
        std::vector<platform::stream_profile> convert_profiles(const std::vector<stream_profile>& vector);
        
        std::function<void(rs2_extension, const std::shared_ptr<extension_snapshot>&)> m_device_record_snapshot_handler;
        sensor_interface& m_sensor;
        librealsense::notifications_callback_ptr m_user_notification_callback;
        frame_interface_callback_t m_record_callback;
        bool m_is_recording;
        bool m_is_pause;
        frame_callback_ptr m_frame_callback;
        std::vector<platform::stream_profile> m_curr_configurations;
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