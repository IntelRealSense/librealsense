// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#pragma once

#include <memory>
#include <vector>

#include "../device.h"
#include "../core/video.h"
#include "../core/motion.h"
#include "TrackingManager.h"
#include "../media/playback/playback_device.h"


namespace librealsense
{
    class tm2_sensor;

    class tm2_device : public virtual device, public tm2_extensions
    {
    public:
        tm2_device(std::shared_ptr<perc::TrackingManager> manager,
            perc::TrackingDevice* dev,
            std::shared_ptr<context> ctx,
            const platform::backend_device_group& group);
        virtual ~tm2_device();

        void enable_loopback(const std::string& source_file) override;
        void disable_loopback() override;
        bool is_enabled() const override;
        void connect_controller(const std::array<uint8_t, 6>& mac_address) override;
        void disconnect_controller(int id) override;
        std::vector<tagged_profile> get_profiles_tags() const override
        {
            return std::vector<tagged_profile>();
        };
        bool compress_while_record() const override { return false; }

    protected:
        void register_stream_to_extrinsic_group(const stream_interface& stream, uint32_t group_index);

    private:
        static const char* tm2_device_name()
        {
            return "Intel RealSense T265";
        }
        std::shared_ptr<perc::TrackingManager> _manager;
        perc::TrackingDevice* _dev;
        std::shared_ptr<tm2_sensor> _sensor;

    };

    class tm2_sensor : public sensor_base, public video_sensor_interface, public wheel_odometry_interface,
                       public pose_sensor_interface, public tm2_sensor_interface, public perc::TrackingDevice::Listener
    {
    public:
        tm2_sensor(tm2_device* owner, perc::TrackingDevice* dev);
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
        rs2_extrinsics get_extrinsics(const stream_profile_interface & profile, perc::SensorId & reference_sensor_id) const;

        // Tracking listener
        ////////////////////
        void onVideoFrame(perc::TrackingData::VideoFrame& tm_frame) override;
        void onAccelerometerFrame(perc::TrackingData::AccelerometerFrame& tm_frame) override;
        void onGyroFrame(perc::TrackingData::GyroFrame& tm_frame) override;
        void onPoseFrame(perc::TrackingData::PoseFrame& tm_frame) override;
        void onControllerDiscoveryEventFrame(perc::TrackingData::ControllerDiscoveryEventFrame& frame) override;
        void onControllerDisconnectedEventFrame(perc::TrackingData::ControllerDisconnectedEventFrame& frame) override;
        void onControllerFrame(perc::TrackingData::ControllerFrame& frame) override;
        void onControllerConnectedEventFrame(perc::TrackingData::ControllerConnectedEventFrame& frame) override;
        void onLocalizationDataEventFrame(perc::TrackingData::LocalizationDataFrame& frame) override;
        void onRelocalizationEvent(perc::TrackingData::RelocalizationEvent& evt) override;

        void enable_loopback(std::shared_ptr<playback_device> input);
        void disable_loopback();
        bool is_loopback_enabled() const;
        void attach_controller(const std::array<uint8_t, 6>& mac_addr);
        void detach_controller(int id);
        void dispose();
        perc::TrackingData::Temperature get_temperature();
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
        async_op_state perform_async_transfer(std::function<perc::Status()> transfer_activator,
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

    private:
        void handle_imu_frame(perc::TrackingData::TimestampedData& tm_frame_ts, unsigned long long frame_number, rs2_stream stream_type, int index, float3 imu_data, float temperature);
        void pass_frames_to_fw(frame_holder fref);
        void raise_relocalization_event(const std::string& msg, double timestamp);
        void raise_hardware_event(const std::string& msg, const std::string& serialized_data, double timestamp);
        void raise_error_notification(const std::string& msg);

        dispatcher                      _dispatcher;
        perc::TrackingDevice*           _tm_dev;
        mutable std::mutex              _tm_op_lock;
        std::shared_ptr<playback_device>_loopback;
        perc::TrackingData::Profile     _tm_supported_profiles;
        perc::TrackingData::Profile     _tm_active_profiles;
        perc::SIXDOF_MODE               _tm_mode = perc::SIXDOF_MODE_ENABLE_MAPPING | perc::SIXDOF_MODE_ENABLE_RELOCALIZATION;
        mutable std::condition_variable _async_op;
        mutable async_op_state          _async_op_status;
        mutable std::vector<uint8_t>    _async_op_res_buffer;

        float last_exposure = 200.f;
        float last_gain = 1.f;
        bool manual_exposure = false;

        template <perc::SIXDOF_MODE flag, perc::SIXDOF_MODE depends_on, bool invert> friend class tracking_mode_option;
    };
}
