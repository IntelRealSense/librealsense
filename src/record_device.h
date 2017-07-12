// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <core/roi.h>
#include <core/extension.h>
#include <core/serialization.h>
#include "core/streaming.h"
#include "archive.h"
#include "concurrency.h"
#include "sensor.h"

namespace librealsense
{
    class record_sensor : public sensor_interface,
                          public extendable_interface,  //Allows extension for any of the given device's extensions
                          public info_container,//TODO: Ziv, does it make sense to inherit here?, maybe construct the item as recordable
                          public options_container//TODO: Ziv, does it make sense to inherit here?
    {
    public:
        using frame_interface_callback_t = std::function<void(frame_holder)>;

        record_sensor(sensor_interface& sensor, frame_interface_callback_t on_frame);
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
        bool extend_to(rs2_extension_type extension_type, void** ext) override;
    private:
        sensor_interface& m_sensor;
        librealsense::notifications_callback_ptr m_user_notification_callback;
        frame_interface_callback_t m_record_callback;
        bool m_is_recording;
        bool m_is_pause;
        frame_callback_ptr m_frame_callback;
    };

    class record_device : public device_interface,
                          public extendable_interface,
                          public info_container//TODO: Ziv, does it make sense to inherit from container
    {
    public:
        static const uint64_t MAX_CACHED_DATA_SIZE = 1920 * 1080 * 4 * 30; // ~1 sec of HD video @ 30 FPS

        record_device(std::shared_ptr<device_interface> device, std::shared_ptr<device_serializer::writer> serializer);
        virtual ~record_device();

        sensor_interface& get_sensor(size_t i) override;
        size_t get_sensors_count() const override;
        const std::string& get_info(rs2_camera_info info) const override;
        bool supports_info(rs2_camera_info info) const override;
        const sensor_interface& get_sensor(size_t i) const override;
        void hardware_reset() override;
        rs2_extrinsics get_extrinsics(size_t from,
                                      rs2_stream from_stream,
                                      size_t to,
                                      rs2_stream to_stream) const override;
		bool extend_to(rs2_extension_type extension_type, void** ext) override;

    private:
        void write_header();
        std::chrono::nanoseconds get_capture_time();
        void write_data(size_t sensor_index, frame_holder f/*, notifications_callback_ptr& sensor_notification_handler*/);
        std::vector<std::shared_ptr<record_sensor>> create_record_sensors(std::shared_ptr<device_interface> m_device);
        //std::function<void(dispatcher::cancellable_timer)> create_write_task(frame_holder&& frame, size_t sensor_index,
        //    std::chrono::nanoseconds capture_time, uint64_t data_size);
    private:
        std::shared_ptr<device_interface> m_device;
        std::vector<std::shared_ptr<record_sensor>> m_sensors;

        lazy<std::shared_ptr<dispatcher>> m_write_thread;
        std::shared_ptr<device_serializer::writer> m_ros_writer;

        std::chrono::high_resolution_clock::time_point m_capture_time_base;
        std::chrono::high_resolution_clock::duration m_record_pause_time;
        std::chrono::high_resolution_clock::time_point m_time_of_pause;

        friend struct mylambda;

        std::mutex m_mutex;
        bool m_is_recording;
        std::once_flag m_first_frame_flag;

        uint64_t m_cached_data_size;
        std::once_flag m_first_call_flag;
        template <typename T>
        std::vector<std::shared_ptr<extension_snapshot>> get_extensions_snapshots(T* extendable);
    };
}


