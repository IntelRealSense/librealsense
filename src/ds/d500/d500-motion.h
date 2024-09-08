// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "d500-device.h"
#include "ds/ds-motion-common.h"

namespace librealsense
{
    class d500_motion : public virtual d500_device
    {
    public:
        std::shared_ptr<synthetic_sensor> create_hid_device( std::shared_ptr<context> ctx,
                                                             const std::vector<platform::hid_device_info>& all_hid_infos );

        d500_motion( std::shared_ptr< const d500_info > const & );

        rs2_motion_device_intrinsic get_motion_intrinsics(rs2_stream) const;

        double get_gyro_default_scale() const override;

    protected:
        friend class ds_motion_common;
        friend class ds_fisheye_sensor;
        friend class ds_motion_sensor;

        std::shared_ptr<ds_motion_common> _ds_motion_common;

    private:
        void register_stream_to_extrinsic_group(const stream_interface& stream, uint32_t group_index);

        optional_value<uint8_t> _motion_module_device_idx;
    };
}
