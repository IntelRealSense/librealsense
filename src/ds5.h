// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "ds5-private.h"

#include "algo.h"
#include "error-handling.h"

namespace rsimpl2
{
    class ds5_camera;

    class ds5_info : public device_info
    {
    public:
        std::shared_ptr<device> create(const uvc::backend& backend) const override;

        uint8_t get_subdevice_count() const override
        {
            auto depth_pid = _depth.front().pid;
            switch(depth_pid)
            {
            case ds::RS400_PID:
            case ds::RS410_PID:
            case ds::RS430_PID:
            case ds::RS420_PID: return 1;
            case ds::RS415_PID: return 2;
            case ds::RS430_MM_PID:
            case ds::RS420_MM_PID:
                return 3;
            default:
                throw not_implemented_exception(to_string() <<
                    "get_subdevice_count is not implemented for DS5 device of type " <<
                    depth_pid);
            }
        }

        ds5_info(std::shared_ptr<uvc::backend> backend,
                 std::vector<uvc::uvc_device_info> depth,
                 std::vector<uvc::usb_device_info> hwm,
                 std::vector<uvc::hid_device_info> hid)
            : device_info(std::move(backend)), _hwm(std::move(hwm)),
              _depth(std::move(depth)), _hid(std::move(hid)) {}

        static std::vector<std::shared_ptr<device_info>> pick_ds5_devices(
                std::shared_ptr<uvc::backend> backend,
                std::vector<uvc::uvc_device_info>& uvc,
                std::vector<uvc::usb_device_info>& usb,
                std::vector<uvc::hid_device_info>& hid);

    private:
        std::vector<uvc::uvc_device_info> _depth;
        std::vector<uvc::usb_device_info> _hwm;
        std::vector<uvc::hid_device_info> _hid;
    };

    class ds5_camera final : public device
    {
    public:
        rs2_extrinsics get_extrinsics(int from_subdevice, rs2_stream, int to_subdevice, rs2_stream) override;

        std::shared_ptr<hid_endpoint> create_hid_device(const uvc::backend& backend,
                                                        const std::vector<uvc::hid_device_info>& all_hid_infos,
                                                        const firmware_version& camera_fw_version);

        std::shared_ptr<uvc_endpoint> create_depth_device(const uvc::backend& backend,
                                                          const std::vector<uvc::uvc_device_info>& all_device_infos);

        uvc_endpoint& get_depth_endpoint()
        {
            return static_cast<uvc_endpoint&>(get_endpoint(_depth_device_idx));
        }

        ds5_camera(const uvc::backend& backend,
            const std::vector<uvc::uvc_device_info>& dev_info,
            const std::vector<uvc::usb_device_info>& hwm_device,
            const std::vector<uvc::hid_device_info>& hid_info);

        std::vector<uint8_t> send_receive_raw_data(const std::vector<uint8_t>& input) override;
        rs2_intrinsics get_intrinsics(unsigned int subdevice, const stream_profile& profile) const override;
        rs2_motion_device_intrinsic get_motion_intrinsics(rs2_stream) const override;

        std::shared_ptr<auto_exposure_mechanism> register_auto_exposure_options(uvc_endpoint* uvc_ep, const uvc::extension_unit* fisheye_xu);
        void hardware_reset() override;
    private:
        bool is_camera_in_advanced_mode() const;

        const uint8_t _depth_device_idx;
        uint8_t _fisheye_device_idx = -1;
        uint8_t _motion_module_device_idx = -1;

        std::shared_ptr<hw_monitor> _hw_monitor;


        lazy<std::vector<uint8_t>> _coefficients_table_raw;
        lazy<std::vector<uint8_t>> _fisheye_intrinsics_raw;
        lazy<std::vector<uint8_t>> _fisheye_extrinsics_raw;
        lazy<ds::extrinsics_table> _motion_module_extrinsics_raw;
        lazy<ds::imu_intrinsics> _accel_intrinsics;
        lazy<ds::imu_intrinsics> _gyro_intrinsics;

        std::vector<uint8_t> get_raw_calibration_table(ds::calibration_table_id table_id) const;
        std::vector<uint8_t> get_raw_fisheye_intrinsics_table() const;
        std::vector<uint8_t> get_raw_fisheye_extrinsics_table() const;
        ds::imu_calibration_table get_motion_module_calibration_table() const;

        pose get_device_position(unsigned int subdevice) const;

        // Bandwidth parameters from BOSCH BMI 055 spec'
        std::vector<std::pair<std::string, stream_profile>> sensor_name_and_hid_profiles =
            {{"gyro_3d",  {RS2_STREAM_GYRO,  1, 1, 200,  RS2_FORMAT_MOTION_RAW}},
             {"gyro_3d",  {RS2_STREAM_GYRO,  1, 1, 400,  RS2_FORMAT_MOTION_RAW}},
             {"gyro_3d",  {RS2_STREAM_GYRO,  1, 1, 1000, RS2_FORMAT_MOTION_RAW}},
             {"gyro_3d",  {RS2_STREAM_GYRO,  1, 1, 200,  RS2_FORMAT_MOTION_XYZ32F}},
             {"gyro_3d",  {RS2_STREAM_GYRO,  1, 1, 400,  RS2_FORMAT_MOTION_XYZ32F}},
             {"gyro_3d",  {RS2_STREAM_GYRO,  1, 1, 1000, RS2_FORMAT_MOTION_XYZ32F}},
             {"accel_3d", {RS2_STREAM_ACCEL, 1, 1, 125,  RS2_FORMAT_MOTION_RAW}},
             {"accel_3d", {RS2_STREAM_ACCEL, 1, 1, 250,  RS2_FORMAT_MOTION_RAW}},
             {"accel_3d", {RS2_STREAM_ACCEL, 1, 1, 500,  RS2_FORMAT_MOTION_RAW}},
             {"accel_3d", {RS2_STREAM_ACCEL, 1, 1, 1000, RS2_FORMAT_MOTION_RAW}},
             {"accel_3d", {RS2_STREAM_ACCEL, 1, 1, 125,  RS2_FORMAT_MOTION_XYZ32F}},
             {"accel_3d", {RS2_STREAM_ACCEL, 1, 1, 250,  RS2_FORMAT_MOTION_XYZ32F}},
             {"accel_3d", {RS2_STREAM_ACCEL, 1, 1, 500,  RS2_FORMAT_MOTION_XYZ32F}},
             {"accel_3d", {RS2_STREAM_ACCEL, 1, 1, 1000, RS2_FORMAT_MOTION_XYZ32F}},
             {"HID Sensor Class Device: Gyroscope",     {RS2_STREAM_GYRO,  1, 1, 1000, RS2_FORMAT_MOTION_XYZ32F}} ,
             {"HID Sensor Class Device: Accelerometer", {RS2_STREAM_ACCEL, 1, 1, 1000, RS2_FORMAT_MOTION_XYZ32F}},
             {"HID Sensor Class Device: Custom",        {RS2_STREAM_ACCEL, 1, 1, 1000, RS2_FORMAT_MOTION_XYZ32F}}};

        std::map<rs2_stream, std::map<unsigned, unsigned>> fps_and_sampling_frequency_per_rs2_stream =
                                                         {{RS2_STREAM_ACCEL, {{125,  1},
                                                                              {250,  2},
                                                                              {500,  5},
                                                                              {1000, 10}}},
                                                          {RS2_STREAM_GYRO,  {{200,  1},
                                                                              {400,  4},
                                                                              {1000, 10}}}};

        std::unique_ptr<polling_error_handler> _polling_error_handler;

         
    };

    class ds5_notification_decoder :public notification_decoder
    {
    public:
        notification decode(int value) override;
    };
}
