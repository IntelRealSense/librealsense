// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include "core/roi.h"
#include "core/extension.h"
#include "core/serialization.h"
#include "archive.h"
#include "sensor.h"

#include <set>


namespace librealsense
{
    class record_sensor : public sensor_interface,
                          public extendable_interface,  //Allows extension for any of the given device's extensions
                          public info_container,
                          public options_container
    {
    public:
        record_sensor(device_interface& device,
                      sensor_interface& sensor);
        virtual ~record_sensor();

        void on_notification( std::function< void( const notification & ) > && callback )
            { _on_notification = std::move( callback ); }
        void on_frame( std::function< void( frame_holder ) > && callback )
            { _on_frame = std::move( callback ); }
        void on_extension_change( std::function< void( rs2_extension, std::shared_ptr< extension_snapshot > ) > && callback )
            { _on_extension_change = std::move( callback ); }

        void init();
        stream_profiles get_stream_profiles(int tag = profile_tag::PROFILE_TAG_ANY) const override;
        void open(const stream_profiles& requests) override;
        void close() override;
        option& get_option(rs2_option id) override;
        const option& get_option(rs2_option id) const override;
        const std::string& get_info(rs2_camera_info info) const override;
        bool supports_info(rs2_camera_info info) const override;
        bool supports_option(rs2_option id) const override;
        void register_notifications_callback( rs2_notifications_callback_sptr callback ) override;
        rs2_notifications_callback_sptr get_notifications_callback() const override;
        void start( rs2_frame_callback_sptr callback ) override;
        void stop() override;
        bool is_streaming() const override;
        bool extend_to(rs2_extension extension_type, void** ext) override;
        device_interface& get_device() override;
        rs2_frame_callback_sptr get_frames_callback() const override;
        void set_frames_callback( rs2_frame_callback_sptr callback ) override;
        stream_profiles get_active_streams() const override;
        stream_profiles const & get_raw_stream_profiles() const override;
        int register_before_streaming_changes_callback(std::function<void(bool)> callback) override;
        void unregister_before_start_callback(int token) override;
        void stop_with_error(const std::string& message);
        void disable_recording();
        virtual processing_blocks get_recommended_processing_blocks() const override;

        rsutils::subscription register_options_changed_callback( options_watcher::callback && cb ) override
        {
            throw not_implemented_exception( "Registering options value changed callback is not implemented for record sensor" );
        }

    private /*methods*/:
        std::function< void( const notification & ) > _on_notification;
        std::function< void( frame_holder ) > _on_frame;
        std::function< void( rs2_extension, std::shared_ptr< extension_snapshot > ) > _on_extension_change;

        template <typename T> void record_snapshot(rs2_extension extension_type, const librealsense::recordable<T>& snapshot);
        void record_frame(frame_holder holder);
        void enable_sensor_hooks();
        void disable_sensor_hooks();
        void hook_sensor_callbacks();
        rs2_frame_callback_sptr wrap_frame_callback( rs2_frame_callback_sptr callback );
        void unhook_sensor_callbacks();
        void enable_sensor_options_recording();
        void disable_sensor_options_recording();
        void wrap_streams();

    private /*members*/:

        sensor_interface& m_sensor;
        std::set<int> m_recorded_streams_ids;
        std::set<rs2_option> m_recording_options;
        rs2_notifications_callback_sptr m_user_notification_callback;
        std::atomic_bool m_is_recording;
        rs2_frame_callback_sptr m_frame_callback;
        rs2_frame_callback_sptr m_original_callback;
        int m_before_start_callback_token;
        device_interface& m_parent_device;
        bool m_is_sensor_hooked;
        bool m_register_notification_to_base;
        std::mutex m_mutex;
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

}
