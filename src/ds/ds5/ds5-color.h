// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "ds5-device.h"
#include "ds/ds-color-common.h"

#include <map>

#include "stream.h"

namespace librealsense
{
    class ds5_color : public virtual ds5_device
    {
    public:
        ds5_color(std::shared_ptr<context> ctx,
                  const platform::backend_device_group& group);

        synthetic_sensor& get_color_sensor()
        {
            return dynamic_cast<synthetic_sensor&>(get_sensor(_color_device_idx));
        }

        uvc_sensor& get_raw_color_sensor()
        {
            synthetic_sensor& color_sensor = get_color_sensor();
            return dynamic_cast<uvc_sensor&>(*color_sensor.get_raw_sensor());
        }

    protected:
        std::shared_ptr<stream_interface> _color_stream;
        std::shared_ptr<ds_color_common> _ds_color_common;

    private:
        void register_options();
        void register_metadata();
        void register_processing_blocks();

        void register_stream_to_extrinsic_group(const stream_interface& stream, uint32_t group_index);

        void create_color_device(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group);
        void init();

        friend class ds5_color_sensor;
        friend class rs435i_device;
        friend class ds_color_common;

        uint8_t _color_device_idx = -1;
        bool _separate_color;
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

    protected:
        const ds5_color* _owner;
    };

}
