// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "ds5-device.h"
#include "ds/ds-motion-common.h"

namespace librealsense
{
    class ds5_motion_base : public virtual ds5_device
    {
    public:
        rs2_motion_device_intrinsic get_motion_intrinsics(rs2_stream) const;

        std::shared_ptr<auto_exposure_mechanism> register_auto_exposure_options(synthetic_sensor* ep,
            const platform::extension_unit* fisheye_xu);

    private:
        friend class ds_motion_sensor;
        friend class ds_fisheye_sensor;
        friend class ds_motion_common;

        std::shared_ptr<lazy<ds::imu_intrinsic>> _accel_intrinsic;
        std::shared_ptr<lazy<ds::imu_intrinsic>> _gyro_intrinsic;
        std::shared_ptr<lazy<rs2_extrinsics>>   _depth_to_imu;                  // Mechanical installation pose

    protected:
        ds5_motion_base(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group);

        std::shared_ptr<ds_motion_common> _ds_motion_common;

        std::shared_ptr<stream_interface> _accel_stream;
        std::shared_ptr<stream_interface> _gyro_stream;

        uint16_t _pid;    // product PID
        std::shared_ptr<mm_calib_handler>        _mm_calib;
        optional_value<uint8_t> _motion_module_device_idx;
    };

    class ds5_motion : public ds5_motion_base
    {
    public:
        ds5_motion(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group);

        std::shared_ptr<synthetic_sensor> create_hid_device(std::shared_ptr<context> ctx,
            const std::vector<platform::hid_device_info>& all_hid_infos,
            const firmware_version& camera_fw_version);

        rs2_motion_device_intrinsic get_motion_intrinsics(rs2_stream) const;

    protected:
        friend class ds_motion_common;
        friend class ds_fisheye_sensor;
        friend class ds_motion_sensor;


    private:
        void register_fisheye_options();
        void register_fisheye_metadata();

        void register_stream_to_extrinsic_group(const stream_interface& stream, uint32_t group_index);

        void initialize_fisheye_sensor(std::shared_ptr<context> ctx, const platform::backend_device_group& group);

        optional_value<uint8_t> _fisheye_device_idx;
        optional_value<uint8_t> _motion_module_device_idx;

    };

    class ds5_motion_uvc : public ds5_motion_base
    {
    public:
        ds5_motion_uvc(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group);

        std::shared_ptr<synthetic_sensor> create_uvc_device(std::shared_ptr<context> ctx,
            const std::vector<platform::uvc_device_info>& all_uvc_infos,
            const firmware_version& camera_fw_version);
    };
}
