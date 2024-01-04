// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "d500-device.h"
#include "ds/ds-color-common.h"
#include <src/color-sensor.h>

#include "stream.h"

#include <rsutils/lazy.h>
#include <map>


namespace librealsense
{
    class d500_color : public virtual d500_device
    {
    public:
        d500_color( std::shared_ptr< const d500_info > const &, rs2_format native_format );

        synthetic_sensor& get_color_sensor()
        {
            return dynamic_cast<synthetic_sensor&>(get_sensor(_color_device_idx));
        }

        std::shared_ptr< uvc_sensor > get_raw_color_sensor()
        {
            synthetic_sensor & color_sensor = get_color_sensor();
            return std::dynamic_pointer_cast< uvc_sensor >( color_sensor.get_raw_sensor() );
        }

    protected:
        void register_color_features();

        std::shared_ptr<stream_interface> _color_stream;
        std::shared_ptr<ds_color_common> _ds_color_common;
        rs2_format const _native_format;

    private:
        void register_options();
        void register_metadata();
        void register_color_processing_blocks();

        void register_stream_to_extrinsic_group(const stream_interface& stream, uint32_t group_index);

        void create_color_device(std::shared_ptr<context> ctx,
            const platform::backend_device_group& group);
        void init();

        friend class d500_color_sensor;

        uint8_t _color_device_idx = -1;
        bool _separate_color;
        rsutils::lazy< std::vector< uint8_t > > _color_calib_table_raw;
        std::shared_ptr< rsutils::lazy< rs2_extrinsics > > _color_extrinsic;
    };

    class d500_color_sensor : public synthetic_sensor,
                             public video_sensor_interface,
                             public roi_sensor_base,
                             public color_sensor
    {
    public:
        explicit d500_color_sensor(d500_color* owner,
            std::shared_ptr<uvc_sensor> uvc_sensor,
            std::map<uint32_t, rs2_format> d500_color_fourcc_to_rs2_format,
            std::map<uint32_t, rs2_stream> d500_color_fourcc_to_rs2_stream)
            : synthetic_sensor("RGB Camera", uvc_sensor, owner, d500_color_fourcc_to_rs2_format, d500_color_fourcc_to_rs2_stream),
            _owner(owner)
        {}

        rs2_intrinsics get_intrinsics(const stream_profile& profile) const override;
        stream_profiles init_stream_profiles() override;
        processing_blocks get_recommended_processing_blocks() const override;

    protected:
        const d500_color* _owner;
    };

}
