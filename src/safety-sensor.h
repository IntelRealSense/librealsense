// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "core/extension.h"

namespace librealsense {
    class safety_sensor : public recordable<safety_sensor>
    {
    public:
        virtual ~safety_sensor() = default;

        void create_snapshot(std::shared_ptr<safety_sensor>& snapshot) const override;
        void enable_recording(std::function<void(const safety_sensor&)> recording_function) override {};
        virtual void set_safety_preset(int index, const rs2_safety_preset& sp) const = 0;
        virtual rs2_safety_preset get_safety_preset(int index) const = 0;
    };
    MAP_EXTENSION(RS2_EXTENSION_SAFETY_SENSOR, librealsense::safety_sensor);

    class safety_sensor_snapshot : public virtual safety_sensor, public extension_snapshot
    {
    public:
        safety_sensor_snapshot() {}

        void update(std::shared_ptr<extension_snapshot> ext) override
        {
            //empty
        }

        void create_snapshot(std::shared_ptr<safety_sensor>& snapshot) const  override
        {
            snapshot = std::make_shared<safety_sensor_snapshot>(*this);
        }
        void enable_recording(std::function<void(const safety_sensor&)> recording_function) override
        {
            //empty
        }

        void set_safety_preset(int index, const rs2_safety_preset& sp)  const {};
        rs2_safety_preset get_safety_preset(int index) const
        {
            return rs2_safety_preset();
        }
    };
}
