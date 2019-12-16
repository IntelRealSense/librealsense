// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "ds5-device.h"

#include <map>

#include "stream.h"

namespace librealsense
{
    class ds5_color : public virtual ds5_device
    {
    public:
        std::shared_ptr<synthetic_sensor> create_color_device(std::shared_ptr<context> ctx,
                                                        const std::vector<platform::uvc_device_info>& all_device_infos);

        ds5_color(std::shared_ptr<context> ctx,
                  const platform::backend_device_group& group);

    protected:
        std::shared_ptr<stream_interface> _color_stream;

    private:
        friend class ds5_color_sensor;
        friend class rs435i_device;

        uint8_t _color_device_idx = -1;


        lazy<std::vector<uint8_t>> _color_calib_table_raw;
        std::shared_ptr<lazy<rs2_extrinsics>> _color_extrinsic;
    };

    class ds5_color_sensor : public synthetic_sensor,
                             public video_sensor_interface,
                             public roi_sensor_base,
                             public color_sensor
    {
    public:
        explicit ds5_color_sensor(ds5_color* owner,
            std::shared_ptr<uvc_sensor> uvc_sensor,
            std::map<uint32_t, rs2_format> ds5_color_fourcc_to_rs2_format,
            std::map<uint32_t, rs2_stream> ds5_color_fourcc_to_rs2_stream)
            : synthetic_sensor("RGB Camera", uvc_sensor, owner, ds5_color_fourcc_to_rs2_format, ds5_color_fourcc_to_rs2_stream),
            _owner(owner)
        {}

        rs2_intrinsics get_intrinsics(const stream_profile& profile) const override;
        stream_profiles init_stream_profiles() override;
        processing_blocks get_recommended_processing_blocks() const override;

    private:
        const ds5_color* _owner;
    };

}
