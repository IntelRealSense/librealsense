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
    enum playback_status
    {
        playing, /**< One or more sensors were started, playback is reading and raising data */
        paused,  /**< One or more sensors were started, but playback paused reading and paused raising data*/
        stopped  /**< All sensors were stopped, or playback has ended (all data was read). This is the initial playback status*/
    };

    class playback_sensor : public sensor_interface,
        public extendable_interface,
        public info_container,    
        public options_container
    {
    public:
        using frame_interface_callback_t = std::function<void(frame_holder)>;
        signal<playback_sensor, uint32_t, frame_callback_ptr> started;
        signal<playback_sensor, uint32_t, bool> stopped;
        signal<playback_sensor, uint32_t, const std::vector<stream_profile>&> opened;
        signal<playback_sensor, uint32_t> closed;
        

        playback_sensor(const device_interface& parent_device, const sensor_snapshot& sensor_description, uint32_t sensor_id);
        virtual ~playback_sensor();

        stream_profiles get_stream_profiles() override;
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

        void handle_frame(frame_holder frame, bool is_real_time);
        void stop(bool invoke_required);

        void flush_pending_frames();
    private:
        frame_callback_ptr m_user_callback;
        librealsense::notifications_callback_ptr m_user_notification_callback;
        std::map<rs2_stream, std::shared_ptr<dispatcher>> m_dispatchers;
        std::atomic<bool> m_is_started;
        sensor_snapshot m_sensor_description;
        uint32_t m_sensor_id;
        std::mutex m_mutex;
        const device_interface& m_parent_device;
    };
}