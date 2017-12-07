// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#pragma once

#include "device.h"
#include "core\motion.h"
#include "TrackingManager.h"

#include <memory>
#include <vector>

namespace librealsense
{
    class tm2_device : public virtual device
    {
    public:
        tm2_device(std::shared_ptr<perc::TrackingManager> manager,
            perc::TrackingDevice* dev,
            std::shared_ptr<context> ctx,
            const platform::backend_device_group& group);
        void enable_loopback(const std::string& source_file);
        void disble_loopback();

    private:
        std::shared_ptr<perc::TrackingManager> _manager;
        perc::TrackingDevice* _dev;
    };

    class tm2_sensor : public sensor_base, public video_sensor_interface, public perc::TrackingDevice::Listener
    {
    public:
        tm2_sensor(tm2_device* owner, perc::TrackingDevice* dev);
        ~tm2_sensor();
        // sensor interface
        ////////////////////
        stream_profiles init_stream_profiles() override;
        void open(const stream_profiles& requests) override;
        void close() override;
        void start(frame_callback_ptr callback) override;
        void stop() override;
        rs2_intrinsics get_intrinsics(const stream_profile& profile) const override;
        rs2_motion_device_intrinsic get_motion_intrinsics(const motion_stream_profile_interface& profile) const;

        // Tracking listener
        ////////////////////
        void onVideoFrame(perc::TrackingData::VideoFrame& tm_frame) override;
        void onAccelerometerFrame(perc::TrackingData::AccelerometerFrame& tm_frame) override;
        void onGyroFrame(perc::TrackingData::GyroFrame& tm_frame) override;
        void onPoseFrame(perc::TrackingData::PoseFrame& tm_frame) override;
        void onControllerDiscoveryEventFrame(perc::TrackingData::ControllerDiscoveryEventFrame& frame) override;
        void onControllerDisconnectedEventFrame(perc::TrackingData::ControllerDisconnectedEventFrame& frame) override;
        void onControllerFrame(perc::TrackingData::ControllerFrame& frame) override;

        void handle_imu_frame(perc::TrackingData::TimestampedData& tm_frame_ts, unsigned long long frame_number, rs2_stream stream_type, int index, float3 imu_data, float temperature);
    private:
        perc::TrackingDevice* _tm_dev;
        std::mutex _configure_lock;
        perc::TrackingData::Profile _tm_supported_profiles;
        perc::TrackingData::Profile _tm_active_profiles;
    };
}