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
                          public extendable_interface,//Allows extension for any of the given device's extensions
                          public info_container,//TODO: Ziv, does it make sense to inherit here?, maybe construct the item as recordable
                          public options_container//TODO: Ziv, does it make sense to inherit here?
    {
    public:
        using frame_interface_callback_t = std::function<void(std::shared_ptr<librealsense::frame_interface>)>;

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
        void* extend_to(rs2_extension_type extension_type) override;
    private:
        sensor_interface& m_sensor;
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
        static const uint64_t MAX_CACHED_DATA_SIZE = 1920 * 1080 * 4 * 30; // ~1 sec of HD video @ 30 FPS
        void* extend_to(rs2_extension_type extension_type) override;

    private:
        void write_header();
        std::chrono::nanoseconds get_capture_time();
        void write_data(size_t sensor_index, std::shared_ptr<librealsense::frame_interface> f);

    private:
        std::shared_ptr<device_interface> m_device;
        std::vector<std::shared_ptr<record_sensor>> m_sensors;

        lazy<std::shared_ptr<dispatcher>> m_write_thread;
        std::shared_ptr<device_serializer::writer> m_ros_writer;

        std::chrono::high_resolution_clock::time_point m_capture_time_base;
        std::chrono::high_resolution_clock::duration m_record_pause_time;
        std::chrono::high_resolution_clock::time_point m_time_of_pause;

        std::mutex m_mutex;
        bool m_is_recording;


        uint64_t m_cached_data_size;
        bool m_is_first_event;
        std::once_flag m_first_call_flag;
        template <typename T>
        std::vector<std::shared_ptr<extension_snapshot>> get_extensions_snapshots(T* extendable);
    };

//    class extension_snapshot_frame : public frame_interface
//    {
//        sensor_interface& m_sensor;
//        std::shared_ptr<extension_snapshot> m_ext;
//
//    public:
//        extension_snapshot_frame(sensor_interface& s, std::shared_ptr<extension_snapshot> e) :m_sensor(s), m_ext(e)
//        {
//        }
//        double get_timestamp() const override
//        {
//            return 9;
//        }
//        rs2_timestamp_domain get_timestamp_domain() const override
//        {
//            return rs2_timestamp_domain::RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME;
//        }
//        unsigned int get_stream_index() const override
//        {
//            return 0;
//        }
//        const uint8_t* get_data() const override
//        {
//            return nullptr;
//        }
//        size_t get_data_size() const override
//        {
//            return 0;
//        }
//        const sensor_interface& get_sensor() const override
//        {
//            return m_sensor;
//        }
//    };

    class mock_frame : public frame_interface
    {
    public:
        mock_frame()
        {

        }
        uint64_t get_frame_data_size() const override
        {
            return 0;
        }
        rs2_metadata_t get_frame_metadata(const rs2_frame_metadata& frame_metadata) const override
        {
            return rs2_frame_metadata::RS2_FRAME_METADATA_ACTUAL_EXPOSURE;
        }
        bool supports_frame_metadata(const rs2_frame_metadata& frame_metadata) const override
        {
            return false;
        }
        const byte* get_frame_data() const override
        {
            return nullptr;
        }
        rs2_time_t get_frame_timestamp() const override
        {
            return 0;
        }
        rs2_timestamp_domain get_frame_timestamp_domain() const override
        {
            return rs2_timestamp_domain::RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME;
        }
        void set_timestamp(double new_ts) override
        {

        }
        unsigned long long int get_frame_number() const override
        {
            return 0;
        }
        void set_timestamp_domain(rs2_timestamp_domain timestamp_domain) override
        {

        }
        rs2_time_t get_frame_system_time() const override
        {
            return 0;
        }
        rs2_format get_format() const override
        {
            return rs2_format::RS2_FORMAT_XYZ32F;
        }
        rs2_stream get_stream_type() const override
        {
            return rs2_stream::RS2_STREAM_FISHEYE;
        }
        int get_framerate() const override
        {
            return 0;
        }
        rs2_time_t get_frame_callback_start_time_point() const override
        {
            return 0;
        }
        void update_frame_callback_start_ts(rs2_time_t ts) override
        {

        }
        void acquire() override
        {

        }
        void release() override
        {

        }
        frame_interface* publish(std::shared_ptr<archive_interface> new_owner) override
        {
            return nullptr;
        }
        void attach_continuation(frame_continuation&& continuation) override
        {

        }
        void disable_continuation() override
        {

        }
        archive_interface* get_owner() const override
        {
            return nullptr;
        }
    };
}


