// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "ds6-device.h"

namespace librealsense
{
    class ds6_safety_sensor;

    class ds6_safety : public virtual ds6_device
    {
    public:
        std::shared_ptr<synthetic_sensor> create_safety_device(std::shared_ptr<context> ctx,
            const std::vector<platform::uvc_device_info>& safety_devices_info);

        ds6_safety(std::shared_ptr<context> ctx,
                   const platform::backend_device_group& group);

    private:

        friend class ds6_safety_sensor;

        void register_options(std::shared_ptr<ds6_safety_sensor> safety_ep);
        void register_metadata(std::shared_ptr<uvc_sensor> safety_ep);
        void register_processing_blocks(std::shared_ptr<ds6_safety_sensor> safety_ep);

    protected:
        std::shared_ptr<stream_interface> _safety_stream;
        optional_value<uint8_t> _safety_device_idx;
    };

    class ds6_safety_sensor : public synthetic_sensor,
                              public safety_sensor
    {
    public:
        explicit ds6_safety_sensor(ds6_safety* owner,
            std::shared_ptr<uvc_sensor> uvc_sensor,
            std::map<uint32_t, rs2_format> safety_fourcc_to_rs2_format,
            std::map<uint32_t, rs2_stream> safety_fourcc_to_rs2_stream)
            : synthetic_sensor("Safety Camera", uvc_sensor, owner, safety_fourcc_to_rs2_format, safety_fourcc_to_rs2_stream),
            _owner(owner)
        {}

        // TODO REMI - check if needed
        // rs2_intrinsics get_intrinsics(const stream_profile& profile) const override;
        stream_profiles init_stream_profiles() override;

        void set_safety_preset(int index, const rs2_safety_preset& sp) const override;
        rs2_safety_preset get_safety_preset(int index) const override;
    protected:
        const ds6_safety* _owner;
    };
}
