// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "core/extension.h"

namespace librealsense {
    class depth_mapping_sensor : public recordable<depth_mapping_sensor>
    {
    public:
        virtual ~depth_mapping_sensor() = default;

        void create_snapshot(std::shared_ptr<depth_mapping_sensor>& snapshot) const override;
        void enable_recording(std::function<void(const depth_mapping_sensor&)> recording_function) override {};
    };
    MAP_EXTENSION(RS2_EXTENSION_DEPTH_MAPPING_SENSOR, librealsense::depth_mapping_sensor);

    class depth_mapping_sensor_snapshot : public virtual depth_mapping_sensor, public extension_snapshot
    {
    public:
        depth_mapping_sensor_snapshot() {}

        void update(std::shared_ptr<extension_snapshot> ext) override
        {
            //empty
        }

        void create_snapshot(std::shared_ptr<depth_mapping_sensor>& snapshot) const  override
        {
            snapshot = std::make_shared<depth_mapping_sensor_snapshot>(*this);
        }
        void enable_recording(std::function<void(const depth_mapping_sensor&)> recording_function) override
        {
            //empty
        }
    };
}
