// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "core/processing.h"
#include "concurrency.h"
#include "archive.h"
#include "metadata-parser.h"
#include "frame-archive.h"

namespace librealsense
{
    class option;

    class LRS_EXTENSION_API frame_source
    {
    public:
        frame_source(uint32_t max_publish_list_size = 16);

        void init(std::shared_ptr<metadata_parser_map> metadata_parsers);

        callback_invocation_holder begin_callback();

        void reset();

        std::shared_ptr<option> get_published_size_option();

        frame_interface* alloc_frame(rs2_extension type, size_t size, frame_additional_data additional_data, bool requires_memory) const;

        void set_callback(frame_callback_ptr callback);
        frame_callback_ptr get_callback() const;

        void invoke_callback(frame_holder frame) const;

        void flush() const;

        virtual ~frame_source() { flush(); }

        double get_time() const { return _ts ? _ts->get_time() : 0; }

        void set_sensor(const std::shared_ptr<sensor_interface>& s);

        template<class T>
        void add_extension(rs2_extension ex)
        {
            _archive[ex] = std::make_shared<frame_archive<T>>(&_max_publish_list_size, _ts, _metadata_parsers);
        }

        void set_max_publish_list_size(int qsize) {_max_publish_list_size = qsize; }

    private:
        friend class syncer_process_unit;

        mutable std::mutex _callback_mutex;

        std::map<rs2_extension, std::shared_ptr<archive_interface>> _archive;

        std::atomic<uint32_t> _max_publish_list_size;
        frame_callback_ptr _callback;
        std::shared_ptr<platform::time_service> _ts;
        std::shared_ptr<metadata_parser_map> _metadata_parsers;
    };
}
