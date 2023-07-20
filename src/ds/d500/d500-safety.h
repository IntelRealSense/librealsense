// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "d500-device.h"
#include "core/video.h"

namespace librealsense
{
    class d500_safety_sensor;

    class d500_safety : public virtual d500_device
    {
    public:
        std::shared_ptr<synthetic_sensor> create_safety_device(std::shared_ptr<context> ctx,
            const std::vector<platform::uvc_device_info>& safety_devices_info);

        d500_safety(std::shared_ptr<context> ctx,
                   const platform::backend_device_group& group);

        synthetic_sensor& get_safety_sensor()
        {
            return dynamic_cast<synthetic_sensor&>(get_sensor(_safety_device_idx));
        }

    private:

        friend class d500_safety_sensor;

        void register_options(std::shared_ptr<d500_safety_sensor> safety_ep, std::shared_ptr<uvc_sensor> raw_safety_sensor);
        void register_metadata(std::shared_ptr<uvc_sensor> safety_ep);
        void register_processing_blocks(std::shared_ptr<d500_safety_sensor> safety_ep);

    protected:
        std::shared_ptr<stream_interface> _safety_stream;
        uint8_t _safety_device_idx;
    };

    class d500_safety_sensor : public synthetic_sensor,
                              public video_sensor_interface,
                              public safety_sensor
    {
    public:
        explicit d500_safety_sensor(d500_safety* owner,
            std::shared_ptr<uvc_sensor> uvc_sensor,
            std::map<uint32_t, rs2_format> safety_fourcc_to_rs2_format,
            std::map<uint32_t, rs2_stream> safety_fourcc_to_rs2_stream)
            : synthetic_sensor("Safety Camera", uvc_sensor, owner, safety_fourcc_to_rs2_format, safety_fourcc_to_rs2_stream),
            _owner(owner)
        {}

        rs2_intrinsics get_intrinsics(const stream_profile& profile) const override;
        stream_profiles init_stream_profiles() override;

        void set_safety_preset(int index, const rs2_safety_preset& sp) const override;
        rs2_safety_preset get_safety_preset(int index) const override;
    protected:
        const d500_safety* _owner;
    };

    // subdevice[h] unit[fw], node[h] guid[fw]
    const platform::extension_unit safety_xu = { 4, 0xC, 2,
    { 0xf6c3c3d1, 0x5cde, 0x4477,{ 0xad, 0xf0, 0x41, 0x33, 0xf5, 0x8d, 0xa6, 0xf4 } } };
}
