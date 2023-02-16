// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "core/extension.h"

namespace librealsense {
    class occupancy_sensor : public recordable<occupancy_sensor>
    {
    public:
        virtual ~occupancy_sensor() = default;

        void create_snapshot(std::shared_ptr<occupancy_sensor>& snapshot) const override;
        void enable_recording(std::function<void(const occupancy_sensor&)> recording_function) override {};
        //virtual void set_occupancy_preset(int index, const rs2_occupancy_preset& sp) const = 0;
        //virtual rs2_occupancy_preset get_occupancy_preset(int index) const = 0;
    };
    MAP_EXTENSION(RS2_EXTENSION_OCCUPANCY_SENSOR, librealsense::occupancy_sensor);

    class occupancy_sensor_snapshot : public virtual occupancy_sensor, public extension_snapshot
    {
    public:
        occupancy_sensor_snapshot() {}

        void update(std::shared_ptr<extension_snapshot> ext) override
        {
            //empty
        }

        void create_snapshot(std::shared_ptr<occupancy_sensor>& snapshot) const  override
        {
            snapshot = std::make_shared<occupancy_sensor_snapshot>(*this);
        }
        void enable_recording(std::function<void(const occupancy_sensor&)> recording_function) override
        {
            //empty
        }

        //void set_occupancy_preset(int index, const rs2_occupancy_preset& sp)  const {};
        //rs2_occupancy_preset get_occupancy_preset(int index) const
        //{
        //    return rs2_occupancy_preset();
        //}
    };
}
