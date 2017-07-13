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

namespace librealsense
{

    class snapshot_collection
    {
    public:
        snapshot_collection() {}
        snapshot_collection(const std::map<rs2_extension_type, std::shared_ptr<extension_snapshot>>& snapshots) :
            m_snapshots(snapshots)
        {
        }

        std::shared_ptr<extension_snapshot> find(rs2_extension_type t)
        {
            auto snapshot_it = m_snapshots.find(t);
            if(snapshot_it == std::end(m_snapshots))
            {
                return nullptr;
            }
            return snapshot_it->second;
        }
        std::map<rs2_extension_type, std::shared_ptr<extension_snapshot>> get_snapshots() const
        {
            return m_snapshots;
        }

        const std::shared_ptr<extension_snapshot>& operator[](rs2_extension_type extension) const
        {
            return m_snapshots.at(extension);
        }

        std::shared_ptr<extension_snapshot>& operator[](rs2_extension_type extension)
        {
            return m_snapshots[extension];
        }
    private:
        std::map<rs2_extension_type, std::shared_ptr<extension_snapshot>> m_snapshots;
    };
    class sensor_snapshot
    {
    public:
        sensor_snapshot(const snapshot_collection& sensor_extensions, const std::vector<stream_profile>& streaming_profiles) :
            m_snapshots(sensor_extensions), m_available_profiles(streaming_profiles)
        {
        }
        snapshot_collection get_sensor_extensions_snapshots() const
        {
            return m_snapshots;
        }
        std::vector<stream_profile> get_streamig_profiles() const
        {
            return m_available_profiles;
        }
    private:
        snapshot_collection m_snapshots;
        std::vector<stream_profile> m_available_profiles;
    };
    using device_extrinsics = std::map<std::tuple<size_t, rs2_stream, size_t, rs2_stream>, rs2_extrinsics>;
    
    class device_snapshot
    {
    public:
        device_snapshot() {}
        device_snapshot(const snapshot_collection& device_extensios, const std::vector<sensor_snapshot>& sensors_snapshot, const device_extrinsics& extrinsics) :
            m_device_snapshots(device_extensios),
            m_sensors_snapshot(sensors_snapshot),
            m_extrinsics(extrinsics)
        {

        }
        std::vector<sensor_snapshot> get_sensors_snapshots() const
        {
            return m_sensors_snapshot;
        }
        snapshot_collection get_device_extensions_snapshots() const
        {
            return m_device_snapshots;
        }
    private:
        snapshot_collection m_device_snapshots;
        std::vector<sensor_snapshot> m_sensors_snapshot;
        device_extrinsics m_extrinsics;
    };

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
            //virtual void read(uint32_t sensor_index, std::chrono::nanoseconds& timestamp, frame_holder& frame) = 0;
            virtual void read(std::chrono::nanoseconds& timestamp, uint32_t& sensor_index, frame_holder& frame) = 0;
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