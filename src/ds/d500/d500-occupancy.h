// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "d500-device.h"
#include "core/video.h"

namespace librealsense
{
    class d500_occupancy_sensor;

    class d500_occupancy : public virtual d500_device
    {
    public:
        std::shared_ptr<synthetic_sensor> create_occupancy_device(std::shared_ptr<context> ctx,
            const std::vector<platform::uvc_device_info>& occupancy_devices_info);

        d500_occupancy(std::shared_ptr<context> ctx,
                   const platform::backend_device_group& group);

    private:

        friend class d500_occupancy_sensor;

        void register_options(std::shared_ptr<d500_occupancy_sensor> occupancy_ep, std::shared_ptr<uvc_sensor> raw_occupancy_sensor);
        void register_metadata(std::shared_ptr<uvc_sensor> occupancy_ep);
        void register_processing_blocks(std::shared_ptr<d500_occupancy_sensor> occupancy_ep);

    protected:
        std::shared_ptr<stream_interface> _occupancy_stream;
        uint8_t _occupancy_device_idx;
    };

    class d500_occupancy_sensor : public synthetic_sensor,
                              public video_sensor_interface,
                              public occupancy_sensor
    {
    public:
        explicit d500_occupancy_sensor(d500_occupancy* owner,
            std::shared_ptr<uvc_sensor> uvc_sensor,
            std::map<uint32_t, rs2_format> occupancy_fourcc_to_rs2_format,
            std::map<uint32_t, rs2_stream> occupancy_fourcc_to_rs2_stream)
            : synthetic_sensor("Occupancy Camera", uvc_sensor, owner, occupancy_fourcc_to_rs2_format, occupancy_fourcc_to_rs2_stream),
            _owner(owner)
        {}

        rs2_intrinsics get_intrinsics(const stream_profile& profile) const override;
        stream_profiles init_stream_profiles() override;

    protected:
        const d500_occupancy* _owner;
    };

    // subdevice[h] unit[fw], node[h] guid[fw]
    //const platform::extension_unit occupancy_xu = { 4, 0xC, 2,
    //{ 0xf6c3c3d1, 0x5cde, 0x4477,{ 0xad, 0xf0, 0x41, 0x33, 0xf5, 0x8d, 0xa6, 0xf4 } } };
}
