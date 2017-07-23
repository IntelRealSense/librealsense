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
#include "playback_sensor.h"

namespace librealsense
{
    class playback_device : public device_interface,
        public extendable_interface,
        public info_container//TODO: Ziv, does it make sense to inherit from container
    {
    public:
        playback_device(std::shared_ptr<device_serializer::reader> serializer);
        virtual ~playback_device();

        sensor_interface& get_sensor(size_t i) override;
        size_t get_sensors_count() const override;
        const std::string& get_info(rs2_camera_info info) const override;
        bool supports_info(rs2_camera_info info) const override;
        const sensor_interface& get_sensor(size_t i) const override;
        void hardware_reset() override;
        rs2_extrinsics get_extrinsics(size_t from,rs2_stream from_stream,size_t to,rs2_stream to_stream) const override;
        bool extend_to(rs2_extension extension_type, void** ext) override;
        std::shared_ptr<matcher> create_matcher(rs2_stream stream) const override;

        bool set_frame_rate(double rate);
        bool seek_to_time(uint64_t time);
        playback_status get_current_status() const;
        bool get_duration(uint64_t& duration_microseconds) const;
        bool pause();
        bool resume();
        bool set_real_time(bool real_time);
        bool is_real_time() const;
        const std::string& get_file_name() const;
        
    private:
        void update_time_base(uint64_t base_timestamp);
        int64_t calc_sleep_time(const uint64_t& timestamp) const;
        void start();
        void stop();
        void try_looping();
        template <typename T> void do_loop(T op);
        std::map<uint32_t, std::shared_ptr<playback_sensor>> create_playback_sensors(const device_snapshot& device_description);
        void set_filter(int32_t id, const std::vector<stream_profile>& requested_profiles);
    private:
        lazy<std::shared_ptr<dispatcher>> m_read_thread;
        std::shared_ptr<device_serializer::reader> m_reader;
        device_snapshot m_device_description;
        bool m_is_started;
        bool m_is_paused;
        std::chrono::high_resolution_clock::time_point m_base_sys_time;
        uint64_t m_base_timestamp;
        std::map<uint32_t, std::shared_ptr<playback_sensor>> m_sensors;
        std::map<uint32_t, std::shared_ptr<playback_sensor>> m_active_sensors;
        //TODO: Use rs_notification for signaling playback status changes?
        std::atomic<double> m_sample_rate;
        std::atomic_bool m_real_time;
    };

    MAP_EXTENSION(RS2_EXTENSION_PLAYBACK, playback_device);
}


