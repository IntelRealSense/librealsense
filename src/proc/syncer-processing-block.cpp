// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <functional>
#include "source.h"
#include "sync.h"
#include "proc/synthetic-stream.h"
#include "proc/syncer-processing-block.h"


namespace librealsense
{
    syncer_process_unit::syncer_process_unit(std::shared_ptr<bool_option> is_enabled_opt)
        : processing_block("syncer"), _matcher((new timestamp_composite_matcher({}))), _is_enabled_opt(is_enabled_opt)
    {
        _matcher->set_callback([this](frame_holder f, syncronization_environment env)
        {
            std::stringstream ss;
            ss << "SYNCED: ";
            auto composite = dynamic_cast<composite_frame*>(f.frame);
            for (int i = 0; i < composite->get_embedded_frames_count(); i++)
            {
                auto matched = composite->get_frame(i);
                ss << matched->get_stream()->get_stream_type() << " " << matched->get_frame_number() << ", "<<std::fixed<< matched->get_frame_timestamp()<<" ";
            }

            LOG_DEBUG(ss.str());
            env.matches.enqueue(std::move(f));
        });

        auto f = [&](frame_holder frame, synthetic_source_interface* source)
        {
            // if the syncer is disabled passthrough the frame
            if (auto is_enabled = _is_enabled_opt.lock())
            {
                if (!is_enabled->is_true())
                {
                    get_source().frame_ready(std::move(frame));
                    return;
                }
            }

            single_consumer_frame_queue<frame_holder> matches;

            {
                std::lock_guard<std::mutex> lock(_mutex);
                _matcher->dispatch(std::move(frame), { source, matches });
            }

            frame_holder f;
            while (matches.try_dequeue(&f))
                get_source().frame_ready(std::move(f));

        };

        set_processing_callback(std::shared_ptr<rs2_frame_processor_callback>(
            new internal_frame_processor_callback<decltype(f)>(f)));
    }
}
