// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <chrono>
#include <memory>
#include <vector>
#include "types.h"
#include "extension.h"
#include "streaming.h"
#include "../media/device_snapshot.h"
#include "../media/ros/status.h"

namespace librealsense
{
    namespace device_serializer
    {

        struct snapshot_box
        {
            std::chrono::nanoseconds timestamp;
            uint32_t sensor_index;
            std::shared_ptr<extension_snapshot> snapshot;
        };

        class writer
        {
        public:
            virtual void write_device_description(const device_snapshot& device_description) = 0;
            virtual void write(std::chrono::nanoseconds timestamp, uint32_t sensor_index, frame_holder&& frame) = 0;
            virtual void write(const snapshot_box& data) = 0;
            virtual void reset() = 0;
            virtual ~writer() = default;
        };
        class reader
        {
        public:
            virtual ~reader() = default;
            virtual device_snapshot query_device_description() = 0;
            //TODO: Ziv change return type of read
            virtual file_format::status read(std::chrono::nanoseconds& timestamp, uint32_t& sensor_index, frame_holder& frame) = 0;
            virtual void seek_to_time(std::chrono::nanoseconds time) = 0;
            virtual std::chrono::nanoseconds query_duration() const = 0;
            virtual void reset() = 0;
            virtual void set_filter(uint32_t m_sensor_index, const std::vector<stream_profile>& vector) = 0;
            virtual const std::string& get_file_name() const = 0;
        };
    }
}
