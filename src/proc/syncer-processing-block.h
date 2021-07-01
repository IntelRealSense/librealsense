// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include <stdint.h>
#include <vector>
#include <mutex>
#include <memory>

#include "types.h"
#include "archive.h"
#include "option.h"

namespace librealsense
{
    class processing_block;
    class timestamp_composite_matcher;
    class syncer_process_unit : public processing_block
    {
    public:
        syncer_process_unit(std::initializer_list< bool_option::ptr > enable_opts, bool log = true);

        syncer_process_unit( bool_option::ptr is_enabled_opt = nullptr, bool log = true)
            : syncer_process_unit( { is_enabled_opt }, log) {}

        void add_enabling_option( bool_option::ptr is_enabled_opt )
        {
            _enable_opts.push_back( is_enabled_opt );
        }

        // Stopping the syncer means no more frames will be enqueued, and any existing frames
        // pending dispatch will be lost!
        void stop();

        ~syncer_process_unit()
        {
            _matcher.reset();
        }
    private:
        std::shared_ptr<matcher> _matcher;
        std::vector< std::weak_ptr<bool_option> > _enable_opts;

        single_consumer_frame_queue<frame_holder> _matches;
        std::mutex _callback_mutex;
    };
}
