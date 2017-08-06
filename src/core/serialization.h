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

namespace librealsense
{
    namespace device_serializer
    {
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

        using nanoseconds = std::chrono::duration<uint64_t, std::nano>;

        class writer
        {
        public:
            virtual void write_device_description(const device_snapshot& device_description) = 0;
            virtual void write_frame(const stream_identifier& stream_id, const nanoseconds& timestamp, frame_holder&& frame) = 0;
            virtual void write_snapshot(uint32_t device_index, const nanoseconds& timestamp, rs2_extension type, const std::shared_ptr<extension_snapshot > snapshot) = 0;
            virtual void write_snapshot(const sensor_identifier& sensor_id, const nanoseconds& timestamp, rs2_extension type, const std::shared_ptr<extension_snapshot > snapshot) = 0;
            virtual void reset() = 0;
            virtual ~writer() = default;
        };
        class reader
        {
        public:
            virtual ~reader() = default;
            virtual device_snapshot query_device_description() = 0;
            virtual status read_frame(nanoseconds& timestamp, stream_identifier& sensor_index, frame_holder& frame) = 0;
            //TODO: Add read_snapshot(...)
            virtual void seek_to_time(const nanoseconds& time) = 0;
            virtual nanoseconds query_duration() const = 0;
            virtual void reset() = 0;
            virtual void set_filter(const device_serializer::stream_identifier& stream_id) = 0;
            virtual void clear_filter(const device_serializer::stream_identifier& stream_id) = 0;
            virtual const std::string& get_file_name() const = 0;
        };
    }
}
