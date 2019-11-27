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

        playback_sensor(device_interface& parent_device, const device_serializer::sensor_snapshot& sensor_description);
        virtual ~playback_sensor();

        stream_profiles get_stream_profiles(int tag = profile_tag::PROFILE_TAG_ANY) const override;
        void open(const stream_profiles& requests) override;
        void close() override;
        void register_notifications_callback(notifications_callback_ptr callback) override;
        notifications_callback_ptr get_notifications_callback() const override;
        void start(frame_callback_ptr callback) override;
        void stop() override;
        bool is_streaming() const override;
        bool extend_to(rs2_extension extension_type, void** ext) override;
        device_interface& get_device() override;
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
        bool streams_contains_one_frame_or_more();
        virtual processing_blocks get_recommended_processing_blocks() const override
        {
            auto processing_blocks_snapshot = m_sensor_description.get_sensor_extensions_snapshots().find(RS2_EXTENSION_RECOMMENDED_FILTERS);
            if (processing_blocks_snapshot == nullptr)
            {
                throw invalid_value_exception("Recorded file does not contain sensor processing blocks");
            }
            auto processing_blocks_api = As<recommended_proccesing_blocks_interface>(processing_blocks_snapshot);
            if (processing_blocks_api == nullptr)
            {
                throw invalid_value_exception("Failed to get options interface from sensor snapshots");
            }
            return processing_blocks_api->get_recommended_processing_blocks();
        }
    protected:
        void set_active_streams(const stream_profiles& requests);

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
        device_interface& m_parent_device;
        stream_profiles m_available_profiles;
        stream_profiles m_active_streams;
        mutable std::mutex m_active_profile_mutex;
        const unsigned int _default_queue_size;

    public:
        //handle frame use 3 lambda functions that determines if and when a frame should be published.
        //calc_sleep - calculates the duration that the sensor should wait before publishing the frame,
        // the start point for this calculation is the last playback resume.
        //is_paused - check if the playback was paused while waiting for the frame publish time.
        //update_last_pushed_frame - lets the playback device know that a specific frame was published,
        // the playback device will use this info to determine which frames should be played next in a pause/resume scenario.
        template <class T, class K, class P>
        void handle_frame(frame_holder frame, bool is_real_time, T calc_sleep, K is_paused, P update_last_pushed_frame)
        {
            if (frame == nullptr)
            {
                throw invalid_value_exception("null frame passed to handle_frame");
            }
            if (m_is_started)
            {
                frame->get_owner()->set_sensor(shared_from_this());
                auto type = frame->get_stream()->get_stream_type();
                auto index = static_cast<uint32_t>(frame->get_stream()->get_stream_index());
                frame->set_stream(m_streams[std::make_pair(type, index)]);
                frame->set_sensor(shared_from_this());
                auto stream_id = frame.frame->get_stream()->get_unique_id();
                //TODO: Ziv, remove usage of shared_ptr when frame_holder is cpoyable
                auto pf = std::make_shared<frame_holder>(std::move(frame));

                auto callback = [this, is_real_time, stream_id, pf, calc_sleep, is_paused, update_last_pushed_frame](dispatcher::cancellable_timer t)
                {
                    device_serializer::nanoseconds sleep_for = calc_sleep();
                    if (sleep_for.count() > 0)
                        t.try_sleep(sleep_for.count() * 1e-6);
                    if(is_paused())
                        return;

                    frame_interface* pframe = nullptr;
                    std::swap((*pf).frame, pframe);

                    m_user_callback->on_frame((rs2_frame*)pframe);
                    update_last_pushed_frame();
                };
                m_dispatchers.at(stream_id)->invoke(callback, !is_real_time);
            }
        }
    };
}
