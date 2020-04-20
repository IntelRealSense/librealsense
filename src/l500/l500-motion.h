// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include <vector>
#include <string>
#include "device.h"
#include "stream.h"
#include "l500/l500-device.h"

namespace librealsense
{
    class l500_motion : public virtual l500_device
    {
    public:
        std::shared_ptr<synthetic_sensor> create_hid_device(std::shared_ptr<context> ctx,
            const std::vector<platform::hid_device_info>& all_hid_infos);

        l500_motion(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group);

        std::vector<tagged_profile> get_profiles_tags() const override;

        rs2_motion_device_intrinsic get_motion_intrinsics(rs2_stream) const;

    private:

        friend class l500_hid_sensor;

        optional_value<uint8_t> _motion_module_device_idx;

    protected:
        std::shared_ptr<stream_interface> _accel_stream;
        std::shared_ptr<stream_interface> _gyro_stream;
    };

}
