// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <functional>
#include "source.h"
#include "sync.h"
#include "proc/synthetic-stream.h"
#include "proc/syncer-processing-block.h"


namespace librealsense
{
    syncer_process_unit::syncer_process_unit(std::initializer_list< bool_option::ptr > enable_opts, bool log)
        : processing_block("syncer"), _matcher((new composite_identity_matcher({})))
        , _enable_opts(enable_opts.begin(), enable_opts.end())
    {
        _matcher->set_callback( [this]( frame_holder f, syncronization_environment env ) {
            if( env.log )
            {
                LOG_DEBUG( "<-- queueing " << frame_holder_to_string( f ) );
            }

            // We get here from within a dispatch() call, already protected by a mutex -- so only
            // one thread can enqueue!
            env.matches.enqueue( std::move( f ) );
        } );

        // This callback gets called by the previous processing block when it is done with a frame. We
        // call the matchers with the frame and eventually call the next callback in the list using frame_ready().
        // This callback can get called from multiple threads, one thread per stream -- but always in the correct
        // frame order per stream.
        auto f = [&, log](frame_holder frame, synthetic_source_interface* source)
        {
            // if the syncer is disabled passthrough the frame
            bool enabled = false;
            size_t n_opts = 0;
            for (auto& wopt : _enable_opts)
            {
                auto opt = wopt.lock();
                if (opt)
                {
                    ++n_opts;
                    if (opt->is_true())
                    {
                        enabled = true;
                        break;
                    }
                }
            }
            if (n_opts && !enabled)
            {
                get_source().frame_ready(std::move(frame));
                return;
            }
            LOG_DEBUG( "--> syncing " << frame_holder_to_string( frame ));
            {
                std::lock_guard<std::mutex> lock(_mutex);
                if( ! _matcher->get_active() )
                {
                    LOG_DEBUG( "matcher was stopped: NOT DISPATCHING FRAME!" );
                    return;
                }
                _matcher->dispatch(std::move(frame), { source, _matches, log });
            }

            frame_holder f;
            {
                // Another thread has the lock, meaning will get into the following loop and dequeue all
                // the frames. So there's nothing for us to do...
                std::unique_lock< std::mutex > lock(_callback_mutex, std::try_to_lock);
                if (!lock.owns_lock())
                    return;

                while (_matches.try_dequeue(&f))
                {
                    LOG_DEBUG( "--> frame ready: " << *f.frame );
                    get_source().frame_ready(std::move(f));
                }
            }

        };

        set_processing_callback(std::shared_ptr<rs2_frame_processor_callback>(
            new internal_frame_processor_callback<decltype(f)>(f)));
    }

    // Stopping the syncer means no more frames will be enqueued, and any existing frames
    // pending dispatch will be lost!
    void syncer_process_unit::stop()
    {
        _matcher->stop();
    }
}

