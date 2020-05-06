// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#pragma once

#include <memory>
#include <vector>

#include "../device.h"
#include "../core/video.h"
#include "../core/motion.h"
#include "../media/playback/playback_device.h"

#include "../usb/usb-device.h"
#include "../usb/usb-messenger.h"

#include "t265-messages.h"

namespace librealsense
{
    class tm2_sensor;

    class tm2_device : public virtual device, public tm2_extensions
    {
    public:
        tm2_device(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group,
            bool register_device_notifications);

        virtual ~tm2_device();

        void hardware_reset() override;

        void enable_loopback(const std::string& source_file) override;
        void disable_loopback() override;
        bool is_enabled() const override;
        void connect_controller(const std::array<uint8_t, 6>& mac_address) override {};
        void disconnect_controller(int id) override {};

        std::vector<tagged_profile> get_profiles_tags() const override
        {
            return std::vector<tagged_profile>();
        };
        bool compress_while_record() const override { return false; }

        std::shared_ptr<tm2_sensor> get_tm2_sensor() { return _sensor; }

    protected:
        void register_stream_to_extrinsic_group(const stream_interface& stream, uint32_t group_index);

    private:
        static const char* tm2_device_name()
        {
            return "Intel RealSense T265";
        }
        std::shared_ptr<tm2_sensor> _sensor;

        platform::usb_device_info usb_info;
        platform::rs_usb_device usb_device;
        platform::rs_usb_messenger usb_messenger;

        platform::rs_usb_endpoint endpoint_msg_out, endpoint_msg_in;
        platform::rs_usb_endpoint endpoint_bulk_out, endpoint_bulk_in;
        platform::rs_usb_endpoint endpoint_int_out, endpoint_int_in;

        std::mutex bulk_mutex;
        template<typename Request, typename Response> platform::usb_status bulk_request_response(const Request &request, Response &response, size_t max_response_size = 0, bool assert_success = true);

        platform::rs_usb_request interrupt_read_request(std::vector<uint8_t> & buffer, std::shared_ptr<platform::usb_request_callback> callback);

        std::mutex stream_mutex;
        platform::usb_status stream_write(const t265::bulk_message_request_header * request);

        platform::rs_usb_request stream_read_request(std::vector<uint8_t> & buffer, std::shared_ptr<platform::usb_request_callback> callback);

        void submit_request(platform::rs_usb_request request);
        bool cancel_request(platform::rs_usb_request request);

        friend class tm2_sensor;
    };

    class tm2_sensor : public sensor_base, public video_sensor_interface, public wheel_odometry_interface,
                       public pose_sensor_interface, public tm2_sensor_interface
    {
    public:
        tm2_sensor(tm2_device* owner);
        virtual ~tm2_sensor();

        // sensor interface
        ////////////////////
        stream_profiles init_stream_profiles() override;
        void open(const stream_profiles& requests) override;
        void close() override;
        void start(frame_callback_ptr callback) override;
        void stop() override;
        rs2_intrinsics get_intrinsics(const stream_profile& profile) const override;
        rs2_motion_device_intrinsic get_motion_intrinsics(const motion_stream_profile_interface& profile) const;
        rs2_extrinsics get_extrinsics(const stream_profile_interface & profile, int sensor_id) const;

        void enable_loopback(std::shared_ptr<playback_device> input);
        void disable_loopback();
        bool is_loopback_enabled() const;
        void dispose();
        t265::sensor_temperature get_temperature(int sensor_id);
        void set_exposure_and_gain(float exposure_ms, float gain);
        void set_exposure(float value);
        float get_exposure() const;
        void set_gain(float value);
        float get_gain() const;
        bool is_manual_exposure() const { return manual_exposure; }
        void set_manual_exposure(bool manual);

        // Pose interfaces
        bool export_relocalization_map(std::vector<uint8_t>& lmap_buf) const override;
        bool import_relocalization_map(const std::vector<uint8_t>& lmap_buf) const override;
        bool set_static_node(const std::string& guid, const float3& pos, const float4& orient_quat) const override;
        bool get_static_node(const std::string& guid, float3& pos, float4& orient_quat) const override;
        bool remove_static_node(const std::string& guid) const override;

        // Wheel odometer
        bool load_wheel_odometery_config(const std::vector<uint8_t>& odometry_config_buf) const override;
        bool send_wheel_odometry(uint8_t wo_sensor_id, uint32_t frame_num, const float3& translational_velocity) const override;

        enum async_op_state {
            _async_init     = 1 << 0,
            _async_progress = 1 << 1,
            _async_success  = 1 << 2,
            _async_fail     = 1 << 3,
            _async_max      = 1 << 4
        };

        // Async operations handler
        async_op_state perform_async_transfer(std::function<bool()> transfer_activator,
            std::function<void()> on_success, const std::string& op_description) const;
        // Recording interfaces
        virtual void create_snapshot(std::shared_ptr<pose_sensor_interface>& snapshot) const override {}
        virtual void enable_recording(std::function<void(const pose_sensor_interface&)> record_action) override {}
        virtual void create_snapshot(std::shared_ptr<wheel_odometry_interface>& snapshot) const override {}
        virtual void enable_recording(std::function<void(const wheel_odometry_interface&)> record_action) override {}

        //calibration write interface
        static const uint16_t ID_OEM_CAL = 6;
        void set_intrinsics(const stream_profile_interface& stream_profile, const rs2_intrinsics& intr) override;
        void set_extrinsics(const stream_profile_interface& from_profile, const stream_profile_interface& to_profile, const rs2_extrinsics& extr) override;
        void set_motion_device_intrinsics(const stream_profile_interface& stream_profile, const rs2_motion_device_intrinsic& intr) override;
        void reset_to_factory_calibration() override;
        void write_calibration() override;
        void set_extrinsics_to_ref(rs2_stream stream_type, int stream_index, const rs2_extrinsics& extr);

        t265::SIXDOF_MODE               _tm_mode = t265::SIXDOF_MODE_ENABLE_MAPPING | t265::SIXDOF_MODE_ENABLE_RELOCALIZATION;

    private:
        void handle_imu_frame(unsigned long long tm_frame_ts, unsigned long long frame_number, rs2_stream stream_type, int index, float3 imu_data, float temperature);
        void pass_frames_to_fw(frame_holder fref);
        void raise_relocalization_event(const std::string& msg, double timestamp);
        void raise_error_notification(const std::string& msg);

        mutable std::mutex              _tm_op_lock;
        std::shared_ptr<playback_device>_loopback;
        mutable std::condition_variable _async_op;
        mutable async_op_state          _async_op_status;
        mutable std::vector<uint8_t>    _async_op_res_buffer;

        std::vector<t265::supported_raw_stream_libtm_message> _supported_raw_streams;
        std::vector<t265::supported_raw_stream_libtm_message> _active_raw_streams;
        bool _pose_output_enabled{false};
        tm2_device * _device;

        void print_logs(const std::unique_ptr<t265::bulk_message_response_get_and_clear_event_log> & log);

        bool start_stream();
        bool start_interrupt();
        void stop_stream();
        void stop_interrupt();
        void time_sync();
        void log_poll();
        bool log_poll_once(std::unique_ptr<t265::bulk_message_response_get_and_clear_event_log> & log_buffer);
        std::thread _time_sync_thread;
        std::thread _log_poll_thread;
        std::atomic<bool> _time_sync_thread_stop;
        std::atomic<bool> _log_poll_thread_stop;

        platform::rs_usb_request          _interrupt_request;
        platform::rs_usb_request_callback _interrupt_callback;

        platform::rs_usb_request          _stream_request;
        platform::rs_usb_request_callback _stream_callback;

        float last_exposure = 200.f;
        float last_gain = 1.f;
        bool manual_exposure = false;

        std::atomic<int64_t> device_to_host_ns;
        class coordinated_ts {
            public:
            std::chrono::duration<double, std::milli> device_ts;
            std::chrono::duration<double, std::milli> global_ts;
            std::chrono::duration<double, std::milli> arrival_ts;
        };
        coordinated_ts last_ts;

        coordinated_ts get_coordinated_timestamp(uint64_t device_nanoseconds);

        template <t265::SIXDOF_MODE flag, t265::SIXDOF_MODE depends_on, bool invert> friend class tracking_mode_option;

        // threaded dispatch
        std::shared_ptr<dispatcher> _data_dispatcher;
        void dispatch_threaded(frame_holder frame);

        // interrupt endpoint receive
        void receive_pose_message(const t265::interrupt_message_get_pose & message);
        void receive_accel_message(const t265::interrupt_message_accelerometer_stream & message);
        void receive_gyro_message(const t265::interrupt_message_gyro_stream & message);
        void receive_set_localization_data_complete(const t265::interrupt_message_set_localization_data_stream & message);

        // stream endpoint receive
        void receive_video_message(const t265::bulk_message_video_stream * message);
        void receive_localization_data_chunk(const t265::interrupt_message_get_localization_data_stream * chunk);
    };
}
