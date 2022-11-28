// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "ds6-device.h"
#include "ds/ds-motion-common.h"

namespace librealsense
{
    // Enforce complile-time verification of all the assigned FPS profiles
    enum class IMU_OUTPUT_DATA_RATES : uint16_t
    {
        IMU_FPS_63  = 63,
        IMU_FPS_100 = 100,
        IMU_FPS_200 = 200,
        IMU_FPS_250 = 250,
        IMU_FPS_400 = 400
    };

    using odr = IMU_OUTPUT_DATA_RATES;

#ifdef _WIN32
    static const std::string gyro_sensor_name = "HID Sensor Class Device: Gyroscope";
    static const std::string accel_sensor_name = "HID Sensor Class Device: Accelerometer";
    static const std::map<odr, uint16_t> hid_fps_translation =
    {  //FPS   Value to send to the Driver
        {odr::IMU_FPS_63,   1000},
        {odr::IMU_FPS_100,  1000},
        {odr::IMU_FPS_200,  500},
        {odr::IMU_FPS_250,  400},
        {odr::IMU_FPS_400,  250} };

#else
    static const std::string gyro_sensor_name = "gyro_3d";
    static const std::string accel_sensor_name = "accel_3d";
    static const std::map<IMU_OUTPUT_DATA_RATES, unsigned> hid_fps_translation =
    {  //FPS   Value to send to the Driver
        {odr::IMU_FPS_63,   1},
        {odr::IMU_FPS_100,  1},
        {odr::IMU_FPS_200,  2},
        {odr::IMU_FPS_250,  3},
        {odr::IMU_FPS_400,  4} };
#endif

    class ds6_motion : public virtual ds6_device
    {
    public:
        std::shared_ptr<synthetic_sensor> create_hid_device(std::shared_ptr<context> ctx,
                                                      const std::vector<platform::hid_device_info>& all_hid_infos,
                                                      const firmware_version& camera_fw_version);

        ds6_motion(std::shared_ptr<context> ctx,
                   const platform::backend_device_group& group);

        rs2_motion_device_intrinsic get_motion_intrinsics(rs2_stream) const;

        std::shared_ptr<auto_exposure_mechanism> register_auto_exposure_options(synthetic_sensor* ep,
                                                                                const platform::extension_unit* fisheye_xu);

    private:

        friend class ds6_fisheye_sensor;
        friend class ds6_hid_sensor;

        void initialize_fisheye_sensor(std::shared_ptr<context> ctx, const platform::backend_device_group& group);

        optional_value<uint8_t> _fisheye_device_idx;
        optional_value<uint8_t> _motion_module_device_idx;

        std::shared_ptr<mm_calib_handler>        _mm_calib;
        std::shared_ptr<lazy<ds::imu_intrinsic>> _accel_intrinsic;
        std::shared_ptr<lazy<ds::imu_intrinsic>> _gyro_intrinsic;
        lazy<std::vector<uint8_t>>              _fisheye_calibration_table_raw;
        std::shared_ptr<lazy<rs2_extrinsics>>   _depth_to_imu;                  // Mechanical installation pose

        uint16_t _pid;    // product PID

        // Bandwidth parameters required for HID sensors
        // The Acceleration configuration will be resolved according to the IMU sensor type at run-time
        std::vector<std::pair<std::string, stream_profile>> sensor_name_and_hid_profiles =
        { { gyro_sensor_name,     {RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_GYRO, 0, 1, 1, int(odr::IMU_FPS_200)}},
          { gyro_sensor_name,     {RS2_FORMAT_MOTION_XYZ32F, RS2_STREAM_GYRO, 0, 1, 1, int(odr::IMU_FPS_400)}}};

        // Translate frequency to SENSOR_PROPERTY_CURRENT_REPORT_INTERVAL.
        std::map<rs2_stream, std::map<unsigned, unsigned>> fps_and_sampling_frequency_per_rs2_stream =
        { { RS2_STREAM_GYRO,     {{unsigned(odr::IMU_FPS_200),  hid_fps_translation.at(odr::IMU_FPS_200)},
                                 { unsigned(odr::IMU_FPS_400),  hid_fps_translation.at(odr::IMU_FPS_400)}}} };

    protected:
        std::shared_ptr<stream_interface> _fisheye_stream;
        std::shared_ptr<stream_interface> _accel_stream;
        std::shared_ptr<stream_interface> _gyro_stream;
    };
}
