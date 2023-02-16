// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "core/extension.h"

namespace librealsense {
    class mapping_sensor : public recordable<mapping_sensor>
    {
    public:
        virtual ~mapping_sensor() = default;

        void create_snapshot(std::shared_ptr<mapping_sensor>& snapshot) const override;
        void enable_recording(std::function<void(const mapping_sensor&)> recording_function) override {};
        //virtual void set_mapping_preset(int index, const rs2_mapping_preset& sp) const = 0;
        //virtual rs2_mapping_preset get_mapping_preset(int index) const = 0;
    };
    MAP_EXTENSION(RS2_EXTENSION_MAPPING_SENSOR, librealsense::mapping_sensor);

    class mapping_sensor_snapshot : public virtual mapping_sensor, public extension_snapshot
    {
    public:
        mapping_sensor_snapshot() {}

        void update(std::shared_ptr<extension_snapshot> ext) override
        {
            //empty
        }

        void create_snapshot(std::shared_ptr<mapping_sensor>& snapshot) const  override
        {
            snapshot = std::make_shared<mapping_sensor_snapshot>(*this);
        }
        void enable_recording(std::function<void(const mapping_sensor&)> recording_function) override
        {
            //empty
        }

        //void set_mapping_preset(int index, const rs2_mapping_preset& sp)  const {};
        //rs2_mapping_preset get_mapping_preset(int index) const
        //{
        //    return rs2_mapping_preset();
        //}
    };
}
