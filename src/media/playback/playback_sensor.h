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
#include "types.h"

namespace librealsense
{
    class playback_sensor : public sensor_interface,
        public extendable_interface,
        public info_container,
        public options_container,
        public std::enable_shared_from_this<playback_sensor>
    {
    public:
        using frame_interface_callback_t = std::function<void(frame_holder)>;
        signal<playback_sensor, uint32_t, frame_callback_ptr> started;
        signal<playback_sensor, uint32_t, bool> stopped;
        signal<playback_sensor, const std::vector<device_serializer::stream_identifier>& > opened;
        signal<playback_sensor, const std::vector<device_serializer::stream_identifier>& > closed;

        playback_sensor(const device_interface& parent_device, const device_serializer::sensor_snapshot& sensor_description);
        virtual ~playback_sensor();

        stream_profiles get_stream_profiles() const override;
        void open(const stream_profiles& requests) override;
        void close() override;
        void register_notifications_callback(notifications_callback_ptr callback) override;
        notifications_callback_ptr get_notifications_callback() const override;
        void start(frame_callback_ptr callback) override;
        void stop() override;
        bool is_streaming() const override;
        bool extend_to(rs2_extension extension_type, void** ext) override;
        const device_interface& get_device() override;
        void handle_frame(frame_holder frame, bool is_real_time);
        void update_option(rs2_option id, std::shared_ptr<option> option);
        void stop(bool invoke_required);
        void flush_pending_frames();
        void update(const device_serializer::sensor_snapshot& sensor_snapshot);
        frame_callback_ptr get_frames_callback() const override;
        void set_frames_callback(frame_callback_ptr callback) override;
        stream_profiles get_active_streams() const override;
        int register_before_streaming_changes_callback(std::function<void(bool)> callback) override;
        void unregister_before_start_callback(int token) override;
        void raise_notification(const notification& n);
    private:
        void register_sensor_streams(const stream_profiles& vector);
        void register_sensor_infos(const device_serializer::sensor_snapshot& sensor_snapshot);
        void register_sensor_options(const device_serializer::sensor_snapshot& sensor_snapshot);

        frame_callback_ptr m_user_callback;
        notifications_processor _notifications_processor;
        using stream_unique_id = int;
        std::map<stream_unique_id, std::shared_ptr<dispatcher>> m_dispatchers;
        std::atomic<bool> m_is_started;
        device_serializer::sensor_snapshot m_sensor_description;
        uint32_t m_sensor_id;
        std::mutex m_mutex;
        std::map<std::pair<rs2_stream, uint32_t>, std::shared_ptr<stream_profile_interface>> m_streams;
        const device_interface& m_parent_device;
        stream_profiles m_available_profiles;
        stream_profiles m_active_streams;
    };
}
