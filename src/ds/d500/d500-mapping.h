// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "d500-device.h"
#include "core/video.h"

namespace librealsense
{
    class d500_mapping_sensor;

    class d500_mapping : public virtual d500_device
    {
    public:
        std::shared_ptr<synthetic_sensor> create_mapping_device(std::shared_ptr<context> ctx,
            const std::vector<platform::uvc_device_info>& mapping_devices_info);

        d500_mapping(std::shared_ptr<context> ctx,
                   const platform::backend_device_group& group);

    private:

        friend class d500_mapping_sensor;

        void register_options(std::shared_ptr<d500_mapping_sensor> mapping_ep, std::shared_ptr<uvc_sensor> raw_mapping_sensor);
        void register_metadata(std::shared_ptr<uvc_sensor> mapping_ep);
        void register_processing_blocks(std::shared_ptr<d500_mapping_sensor> mapping_ep);
        
    protected:
        std::shared_ptr<stream_interface> _occupancy_stream;
        std::shared_ptr<stream_interface> _point_cloud_stream;
        uint8_t _mapping_device_idx;
    };

    class d500_mapping_sensor : public synthetic_sensor,
                              public video_sensor_interface,
                              public mapping_sensor
    {
    public:
        explicit d500_mapping_sensor(d500_mapping* owner,
            std::shared_ptr<uvc_sensor> uvc_sensor,
            std::map<uint32_t, rs2_format> mapping_fourcc_to_rs2_format,
            std::map<uint32_t, rs2_stream> mapping_fourcc_to_rs2_stream)
            : synthetic_sensor("Mapping Camera", uvc_sensor, owner, mapping_fourcc_to_rs2_format, mapping_fourcc_to_rs2_stream),
            _owner(owner)
        {}

        rs2_intrinsics get_intrinsics(const stream_profile& profile) const override;
        stream_profiles init_stream_profiles() override;

    protected:
        const d500_mapping* _owner;
    };
}
