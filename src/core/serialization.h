// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <chrono>
#include <map>
#include <memory>
#include <vector>
#include "types.h"
#include "extension.h"
#include "streaming.h"

namespace librealsense
{
    namespace device_serializer
    {
        struct sensor_identifier
        {
            uint32_t device_index;
            uint32_t sensor_index;
        };
        struct stream_identifier
        {
            uint32_t device_index;
            uint32_t sensor_index;
            rs2_stream stream_type;
            uint32_t stream_index;
        };
        inline bool operator==(const stream_identifier& lhs, const stream_identifier& rhs)
        {
            return lhs.device_index == rhs.device_index &&
                lhs.sensor_index == rhs.sensor_index &&
                lhs.stream_type == rhs.stream_type  &&
                lhs.stream_index == rhs.stream_index;
        }
        inline bool operator<(const stream_identifier& lhs, const stream_identifier& rhs)
        {
            return std::make_tuple(lhs.device_index, lhs.sensor_index, lhs.stream_type, lhs.stream_index) < std::make_tuple(rhs.device_index, rhs.sensor_index, rhs.stream_type, rhs.stream_index);
        }

        using nanoseconds = std::chrono::duration<uint64_t, std::nano>;


        class snapshot_collection
        {
        public:
            snapshot_collection() {}
            snapshot_collection(const std::map<rs2_extension, std::shared_ptr<extension_snapshot>>& snapshots) :
                m_snapshots(snapshots)
            {
            }

            std::shared_ptr<extension_snapshot> find(rs2_extension t)
            {
                auto snapshot_it = m_snapshots.find(t);
                if (snapshot_it == std::end(m_snapshots))
                {
                    return nullptr;
                }
                return snapshot_it->second;
            }
            std::map<rs2_extension, std::shared_ptr<extension_snapshot>> get_snapshots() const
            {
                return m_snapshots;
            }

            const std::shared_ptr<extension_snapshot>& operator[](rs2_extension extension) const
            {
                return m_snapshots.at(extension);
            }

            std::shared_ptr<extension_snapshot>& operator[](rs2_extension extension)
            {
                return m_snapshots[extension];
            }
        private:
            std::map<rs2_extension, std::shared_ptr<extension_snapshot>> m_snapshots;
        };

        class sensor_snapshot
        {
        public:
            sensor_snapshot(uint32_t index, const snapshot_collection& sensor_extensions, const std::map<rs2_option, float>& options) :
                m_snapshots(sensor_extensions),
                m_options(options),
                m_index(index)
            {
            }

            sensor_snapshot(uint32_t index, const snapshot_collection& sensor_extensions, const std::map<rs2_option, float>& options, stream_profiles streams) :
                m_snapshots(sensor_extensions),
                m_streams(streams),
                m_options(options),
                m_index(index)
            {
            }
            snapshot_collection get_sensor_extensions_snapshots() const
            {
                return m_snapshots;
            }

            snapshot_collection& get_sensor_extensions_snapshots()
            {
                return m_snapshots;
            }
            stream_profiles get_stream_profiles() const
            {
                return m_streams;
            }

            std::map<rs2_option, float> get_options() const
            {
                return m_options;
            }
            uint32_t get_sensor_index() const
            {
                return m_index;
            }
        private:
            snapshot_collection m_snapshots;
            stream_profiles m_streams;
            std::map<rs2_option, float> m_options;
            uint32_t m_index;
        };
        using device_extrinsics = std::map<std::tuple<size_t, rs2_stream, size_t, rs2_stream>, rs2_extrinsics>;

        class device_snapshot
        {
        public:
            device_snapshot() {}
            device_snapshot(const snapshot_collection& device_extensios, const std::vector<sensor_snapshot>& sensors_snapshot, const std::map<stream_identifier, std::pair<uint32_t, rs2_extrinsics>>& extrinsics_map) :
                m_device_snapshots(device_extensios),
                m_sensors_snapshot(sensors_snapshot),
                m_extrinsics_map(extrinsics_map)
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
            std::map<stream_identifier, std::pair<uint32_t, rs2_extrinsics>> get_extrinsics_map() const
            {
                return m_extrinsics_map;
            }
        private:
            snapshot_collection m_device_snapshots;
            std::vector<sensor_snapshot> m_sensors_snapshot;
            std::map<stream_identifier, std::pair<uint32_t, rs2_extrinsics>> m_extrinsics_map;
        };


        /** @brief
        *  Defines return codes that SDK interfaces
        *  use.  Negative values indicate errors, a zero value indicates success,
        *  and positive values indicate warnings.
        */
        enum status
        {
            /* success */
            status_no_error = 0,                /**< Operation succeeded without any warning */

            /* errors */
            status_feature_unsupported = -1,    /**< Unsupported feature */
            status_param_unsupported = -2,      /**< Unsupported parameter(s) */
            status_item_unavailable = -3,       /**< Item not found/not available */
            status_key_already_exists = -4,     /**< Key already exists in the data structure */
            status_invalid_argument = -5,       /**< Argument passed to the method is invalid */
            status_allocation_failled = -6,     /**< Failure in allocation */

            status_file_write_failed = -401,    /**< Failure in open file in WRITE mode */
            status_file_read_failed = -402,     /**< Failure in open file in READ mode */
            status_file_close_failed = -403,    /**< Failure in close a file handle */
            status_file_eof = -404,             /**< EOF */
        };

        class writer
        {
        public:
            virtual void write_device_description(const device_snapshot& device_description) = 0;
            virtual void write_frame(const stream_identifier& stream_id, const nanoseconds& timestamp, frame_holder&& frame) = 0;
            virtual void write_snapshot(uint32_t device_index, const nanoseconds& timestamp, rs2_extension type, const std::shared_ptr<extension_snapshot > snapshot) = 0;
            virtual void write_snapshot(const sensor_identifier& sensor_id, const nanoseconds& timestamp, rs2_extension type, const std::shared_ptr<extension_snapshot > snapshot) = 0;
            virtual ~writer() = default;
        };

        class reader
        {
        public:
            virtual ~reader() = default;
            virtual device_snapshot query_device_description() = 0;
            virtual status read_frame(nanoseconds& timestamp, stream_identifier& sensor_index, frame_holder& frame) = 0;
            virtual void seek_to_time(const nanoseconds& time) = 0;
            virtual nanoseconds query_duration() const = 0;
            virtual void reset() = 0;
            virtual void enable_stream(const std::vector<device_serializer::stream_identifier>& stream_ids) = 0;
            virtual void disable_stream(const std::vector<device_serializer::stream_identifier>& stream_ids) = 0;
            virtual const std::string& get_file_name() const = 0;
        };
    }
}
