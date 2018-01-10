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
#include "record_sensor.h"

namespace librealsense
{
    class record_device : public device_interface,
                          public extendable_interface,
                          public info_container
    {
    public:
        static const uint64_t MAX_CACHED_DATA_SIZE = 1920 * 1080 * 4 * 30; // ~1 sec of HD video @ 30 FPS

        record_device(std::shared_ptr<device_interface> device, std::shared_ptr<device_serializer::writer> serializer);
        virtual ~record_device();

        std::shared_ptr<context> get_context() const override;

        sensor_interface& get_sensor(size_t i) override;
        size_t get_sensors_count() const override;
        const std::string& get_info(rs2_camera_info info) const override;
        bool supports_info(rs2_camera_info info) const override;
        const sensor_interface& get_sensor(size_t i) const override;
        void hardware_reset() override;


        bool extend_to(rs2_extension extension_type, void** ext) override;
        virtual std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;

        void pause_recording();
        void resume_recording();
        const std::string& get_filename() const;
        platform::backend_device_group get_device_data() const override;
        std::pair<uint32_t, rs2_extrinsics> get_extrinsics(const stream_interface& stream) const override;
        bool is_valid() const override;

    private:
        template <typename T> void write_device_extension_changes(const T& ext);
        template <rs2_extension E, typename P> bool extend_to_aux(std::shared_ptr<P> p, void** ext);

        void write_header();
        std::chrono::nanoseconds get_capture_time() const;
        void write_data(size_t sensor_index, frame_holder f, std::function<void(std::string const&)> on_error);
        void write_sensor_extension_snapshot(size_t sensor_index, rs2_extension ext, std::shared_ptr<extension_snapshot> snapshot, std::function<void(std::string const&)> on_error);
        void write_notification(size_t sensor_index, const notification& n);
        std::vector<std::shared_ptr<record_sensor>> create_record_sensors(std::shared_ptr<device_interface> m_device);
        template <typename T> device_serializer::snapshot_collection get_extensions_snapshots(T* extendable);
        template <typename T, typename Ext> void try_add_snapshot(T* extendable, device_serializer::snapshot_collection& snapshots);
        std::shared_ptr<device_interface> m_device;
        std::vector<std::shared_ptr<record_sensor>> m_sensors;

        lazy<std::shared_ptr<dispatcher>> m_write_thread;
        std::shared_ptr<device_serializer::writer> m_ros_writer;

        std::chrono::high_resolution_clock::time_point m_capture_time_base;
        std::chrono::high_resolution_clock::duration m_record_pause_time;
        std::chrono::high_resolution_clock::time_point m_time_of_pause;

        std::mutex m_mutex;
        bool m_is_recording;
        std::once_flag m_first_frame_flag;
        int m_on_notification_token;
        int m_on_frame_token;
        int m_on_extension_change_token;
        uint64_t m_cached_data_size;
        std::once_flag m_first_call_flag;
        void initialize_recording();
        void stop_gracefully(to_string error_msg);
    };

    MAP_EXTENSION(RS2_EXTENSION_RECORD, record_device);
}
