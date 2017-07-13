// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "core/processing.h"
#include "concurrency.h"
#include "archive.h"
#include "metadata-parser.h"

namespace librealsense
{
    class option;

    class frame_source
    {
    public:
        frame_source(std::shared_ptr<uvc::time_service> ts);

        void init(rs2_extension_type type, std::shared_ptr<metadata_parser_map> metadata_parsers, std::weak_ptr<device_interface> owner = std::shared_ptr<device_interface>(nullptr));

        callback_invocation_holder begin_callback();

        void reset();

        std::shared_ptr<option> get_published_size_option();

        rs2_frame* alloc_frame(size_t size, frame_additional_data additional_data, bool requires_memory) const;

        void set_callback(frame_callback_ptr callback);

        void invoke_callback(frame_holder frame) const;

        void flush() const;

        virtual ~frame_source() { flush(); }

        double get_time() const { return _ts?_ts->get_time():0; }

    private:
        friend class syncer_proccess_unit;

        std::mutex _callback_mutex;
        std::shared_ptr<archive_interface> _archive;
        std::atomic<uint32_t> _max_publish_list_size;
        frame_callback_ptr _callback;
        std::shared_ptr<uvc::time_service> _ts;
    };
}
