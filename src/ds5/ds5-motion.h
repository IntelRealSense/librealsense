// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "ds5-device.h"

namespace librealsense
{
    class ds5_motion : public virtual ds5_device
    {
    public:
        std::shared_ptr<hid_sensor> create_hid_device(std::shared_ptr<context> ctx,
                                                      const std::vector<platform::hid_device_info>& all_hid_infos,
                                                      const firmware_version& camera_fw_version);

        ds5_motion(std::shared_ptr<context> ctx,
                   const platform::backend_device_group& group);

        rs2_motion_device_intrinsic get_motion_intrinsics(rs2_stream) const;

        std::shared_ptr<auto_exposure_mechanism> register_auto_exposure_options(uvc_sensor* uvc_ep,
                                                                                const platform::extension_unit* fisheye_xu);

    private:

        friend class ds5_fisheye_sensor;
        friend class ds5_hid_sensor;

        void initialize_fisheye_sensor(std::shared_ptr<context> ctx, const platform::backend_device_group& group);

        optional_value<uint8_t> _fisheye_device_idx;
        optional_value<uint8_t> _motion_module_device_idx;

        lazy<std::vector<uint8_t>>      _tm1_eeprom_raw;
        lazy<ds::tm1_eeprom>            _tm1_eeprom;
        lazy<ds::imu_intrinsics>        _accel_intrinsics;
        lazy<ds::imu_intrinsics>        _gyro_intrinsics;
        std::shared_ptr<lazy<rs2_extrinsics>> _fisheye_to_imu;

        ds::tm1_eeprom        get_tm1_eeprom() const;
        std::vector<uint8_t>  get_tm1_eeprom_raw() const;

        lazy<std::vector<uint8_t>>      _fisheye_calibration_table_raw;

#ifdef _WIN32
        // Bandwidth parameters from BOSCH BMI 055 spec'
        std::vector<std::pair<std::string, stream_profile>> sensor_name_and_hid_profiles =
        {{ "HID Sensor Class Device: Gyroscope",     {RS2_STREAM_GYRO,  0, 1, 1, 200, RS2_FORMAT_MOTION_XYZ32F}},
         { "HID Sensor Class Device: Gyroscope",     {RS2_STREAM_GYRO,  0, 1, 1, 400, RS2_FORMAT_MOTION_XYZ32F}},
         { "HID Sensor Class Device: Accelerometer", {RS2_STREAM_ACCEL, 0, 1, 1, 63, RS2_FORMAT_MOTION_XYZ32F}},
         { "HID Sensor Class Device: Accelerometer", {RS2_STREAM_ACCEL, 0, 1, 1, 250, RS2_FORMAT_MOTION_XYZ32F}}};

        // Translate frequency to SENSOR_PROPERTY_CURRENT_REPORT_INTERVAL
        std::map<rs2_stream, std::map<unsigned, unsigned>> fps_and_sampling_frequency_per_rs2_stream =
                                                           {{RS2_STREAM_ACCEL,{{63,   1000},
                                                                               {250,  400}}},
                                                            {RS2_STREAM_GYRO, {{200,  500},
                                                                               {400,  250}}}};

#else                                                                  
        // Bandwidth parameters from BOSCH BMI 055 spec'
        std::vector<std::pair<std::string, stream_profile>> sensor_name_and_hid_profiles =
        {{"gyro_3d",  {RS2_STREAM_GYRO,  0, 1, 1, 200,  RS2_FORMAT_MOTION_RAW}},
         {"gyro_3d",  {RS2_STREAM_GYRO,  0, 1, 1, 400,  RS2_FORMAT_MOTION_RAW}},
         {"gyro_3d",  {RS2_STREAM_GYRO,  0, 1, 1, 200,  RS2_FORMAT_MOTION_XYZ32F}},
         {"gyro_3d",  {RS2_STREAM_GYRO,  0, 1, 1, 400,  RS2_FORMAT_MOTION_XYZ32F}},
         {"accel_3d", {RS2_STREAM_ACCEL, 0, 1, 1, 63,  RS2_FORMAT_MOTION_RAW}},
         {"accel_3d", {RS2_STREAM_ACCEL, 0, 1, 1, 250,  RS2_FORMAT_MOTION_RAW}},
         {"accel_3d", {RS2_STREAM_ACCEL, 0, 1, 1, 63,  RS2_FORMAT_MOTION_XYZ32F}},
         {"accel_3d", {RS2_STREAM_ACCEL, 0, 1, 1, 250,  RS2_FORMAT_MOTION_XYZ32F}}};

        // The frequency selector is vendor and model-specific
        std::map<rs2_stream, std::map<unsigned, unsigned>> fps_and_sampling_frequency_per_rs2_stream =
                                                         {{RS2_STREAM_ACCEL, {{63,   1},
                                                                              {250,  3}}},
                                                          {RS2_STREAM_GYRO,  {{200,  2},
                                                                              {400,  4}}}};
#endif

    protected:
        std::shared_ptr<stream_interface> _fisheye_stream;
        std::shared_ptr<stream_interface> _accel_stream;
        std::shared_ptr<stream_interface> _gyro_stream;
    };
}
