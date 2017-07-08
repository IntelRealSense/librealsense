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

    class callback_source
    {
    public:
        callback_source(std::shared_ptr<uvc::time_service> ts)
            : _callback(nullptr, [](rs2_frame_callback*) {}),
              _max_publish_list_size(16),
              _ts(ts)
        {}

        void init(std::shared_ptr<metadata_parser_map> metadata_parsers)
        {
            std::lock_guard<std::mutex> lock(_callback_mutex);
            _archive = std::make_shared<frame_archive>(&_max_publish_list_size, _ts, metadata_parsers);
        }

        callback_invocation_holder begin_callback() { return _archive->begin_callback(); }

        void reset()
        {
            std::lock_guard<std::mutex> lock(_callback_mutex);
            _callback.reset();
            _archive.reset();
        }

        std::shared_ptr<option> get_published_size_option();

        rs2_frame* alloc_frame(size_t size, frame_additional_data additional_data, bool requires_memory) const
        {
            auto frame = _archive->alloc_frame(size, additional_data, requires_memory);
            return _archive->track_frame(frame);
        }

        void set_callback(frame_callback_ptr callback)
        {
            std::lock_guard<std::mutex> lock(_callback_mutex);
            _callback = callback;
        }

        void invoke_callback(frame_holder frame) const
        {
            if (frame)
            {
                auto callback = _archive->begin_callback();
                try
                {
                    frame->log_callback_start(_ts->get_time());
                    if (_callback)
                    {
                        rs2_frame* ref = nullptr;
                        std::swap(frame.frame, ref);
                        _callback->on_frame(ref);
                    }
                }
                catch(...)
                {
                    LOG_ERROR("Exception was thrown during user callback!");
                }
            }
        }

        void flush() const
        {
            if (_archive.get())
                _archive->flush();
        }

        virtual ~callback_source() { flush(); }

    private:
        std::mutex _callback_mutex;
        std::shared_ptr<frame_archive> _archive;
        std::atomic<uint32_t> _max_publish_list_size;
        frame_callback_ptr _callback;
        std::shared_ptr<uvc::time_service> _ts;
    };
}
