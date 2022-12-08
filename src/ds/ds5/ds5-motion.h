// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "ds5-device.h"
#include "ds/ds-motion-common.h"

namespace librealsense
{
    class ds5_motion : public virtual ds5_device
    {
    public:
        std::shared_ptr<synthetic_sensor> create_hid_device(std::shared_ptr<context> ctx,
                                                      const std::vector<platform::hid_device_info>& all_hid_infos,
                                                      const firmware_version& camera_fw_version);

        ds5_motion(std::shared_ptr<context> ctx,
                   const platform::backend_device_group& group);

        rs2_motion_device_intrinsic get_motion_intrinsics(rs2_stream) const;

    protected:
        friend class ds_motion_common;
        friend class ds_fisheye_sensor;
        friend class ds_hid_sensor;

        std::shared_ptr<ds_motion_common> _ds_motion_common;        

    private:
        void register_fisheye_options();
        void register_fisheye_metadata();

        void register_stream_to_extrinsic_group(const stream_interface& stream, uint32_t group_index);

        void initialize_fisheye_sensor(std::shared_ptr<context> ctx, const platform::backend_device_group& group);

        optional_value<uint8_t> _fisheye_device_idx;
        optional_value<uint8_t> _motion_module_device_idx;
    };
}
