// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <chrono>
#include <memory>
#include <vector>
#include <archive.h>

#include "extension.h"
#include "streaming.h"

namespace librealsense
{
    class sensor_snapshot
    {
    public:
        sensor_snapshot(const std::vector<std::shared_ptr<extension_snapshot>>& sensor_extensios) :
            m_extensios(sensor_extensios)
        {
        }

        std::vector<std::shared_ptr<librealsense::extension_snapshot>> get_extensions_snapshots() const
        {
			return m_extensios;
        }
    private:
        std::vector<std::shared_ptr<extension_snapshot>> m_extensios;
    };

    class device_snapshot
    {
    public:
        device_snapshot() {}
        device_snapshot(const std::vector<std::shared_ptr<extension_snapshot>>& device_extensios,
                        const std::vector<sensor_snapshot>& sensors_snapshot) :
            m_extensios(device_extensios),
            m_sensors_snapshot(sensors_snapshot)
        {

        }
        std::vector<sensor_snapshot> get_sensors_snapshots() const
        {
            return m_sensors_snapshot;
        }
        std::vector<std::shared_ptr<extension_snapshot>> get_device_extensions_snapshots() const
        {
            return m_extensios;
        }
    private:
        std::vector<std::shared_ptr<extension_snapshot>> m_extensios;
        std::vector<sensor_snapshot> m_sensors_snapshot;
    };

    class device_serializer
    {
    public:
        struct frame_box
        {
            std::chrono::nanoseconds timestamp;
            uint32_t sensor_index;
            std::shared_ptr <librealsense::frame_interface> frame;
        };

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
            virtual void write(const frame_box& data) = 0;
            //virtual void write(const snapshot_box& data) = 0;
            virtual void reset() = 0;
            virtual ~writer() = default;
        };
        class reader
        {
        public:
            virtual device_snapshot query_device_description() = 0;
            virtual frame_box read() = 0;
            virtual void seek_to_time(std::chrono::nanoseconds time) = 0;
            virtual std::chrono::nanoseconds query_duration() const = 0;
            virtual void reset() = 0;
            virtual ~reader() = default;
        };

        virtual std::shared_ptr<writer> get_writer() = 0;
        virtual std::shared_ptr<reader> get_reader() = 0;
        virtual ~device_serializer() = default;
    };
}