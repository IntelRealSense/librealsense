// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <chrono>
#include <memory>
#include <vector>
#include <archive.h>
#include <map>
#include "extension.h"
#include "streaming.h"
#include "../recording/device_snapshot.h"
#include "../recording/ros/status.h"

namespace librealsense
{
    class device_serializer
    {
    public:
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
            virtual device_snapshot query_device_description() = 0;
            //TODO: change return type of read
            virtual rs::file_format::status read(std::chrono::nanoseconds& timestamp, uint32_t& sensor_index, frame_holder& frame) = 0;
            virtual void seek_to_time(std::chrono::nanoseconds time) = 0;
            virtual std::chrono::nanoseconds query_duration() const = 0;
            virtual void reset() = 0;
            virtual ~reader() = default;
            virtual void set_filter(uint32_t m_sensor_index, const std::vector<stream_profile>& vector) = 0;
        };

        virtual std::shared_ptr<writer> get_writer() = 0;
        virtual std::shared_ptr<reader> get_reader() = 0;
        virtual ~device_serializer() = default;
    };
}
