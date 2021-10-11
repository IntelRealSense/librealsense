// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"
#include "core/streaming.h"
#include "callback-invocation.h"


namespace librealsense
{
    struct frame_additional_data;

    class archive_interface : public sensor_part
    {
    public:
        virtual callback_invocation_holder begin_callback() = 0;

        virtual frame_interface* alloc_and_track(const size_t size, const frame_additional_data& additional_data, bool requires_memory) = 0;

        virtual std::shared_ptr<metadata_parser_map> get_md_parsers() const = 0;

        virtual void flush() = 0;

        virtual frame_interface* publish_frame(frame_interface* frame) = 0;
        virtual void unpublish_frame(frame_interface* frame) = 0;
        virtual void keep_frame(frame_interface* frame) = 0;
        virtual ~archive_interface() = default;
    };

    std::shared_ptr<archive_interface> make_archive(rs2_extension type,
        std::atomic<uint32_t>* in_max_frame_queue_size,
        std::shared_ptr<platform::time_service> ts,
        std::shared_ptr<metadata_parser_map> parsers);

}
