// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <functional>
#include "source.h"
#include "sync.h"
#include "proc/synthetic-stream.h"
#include "proc/syncer-processing-block.h"


namespace librealsense
{
syncer_process_unit::syncer_process_unit( std::initializer_list< bool_option::ptr > enable_opts,
                                          bool log )
    : processing_block( "syncer" )
    , _enable_opts( enable_opts.begin(), enable_opts.end() )
{
    auto f = [this, log]( frame_holder frame, synthetic_source_interface * source ) {
        // if the syncer is disabled passthrough the frame
        bool enabled = false;
        size_t n_opts = 0;
        for( auto & wopt : _enable_opts )
        {
            auto opt = wopt.lock();
            if( opt )
            {
                ++n_opts;
                if( opt->is_true() )
                {
                    enabled = true;
                    break;
                }
            }
        }
        if( n_opts && ! enabled )
        {
            get_source().frame_ready( std::move( frame ) );
            return;
        }

        {
            std::lock_guard< std::mutex > lock( _mutex );

            if( !_matcher )
            {
                if(!create_matcher( frame, log ))
                    get_source().frame_ready(std::move(frame));
            }
            else
                _matcher->dispatch( std::move( frame ), { source, matches, log } );
        }

        frame_holder f;
        {
            std::unique_lock< std::mutex > lock(_callback_mutex, std::try_to_lock);
            if (!lock.owns_lock())
                return;

            while( matches.try_dequeue( &f ) )
            {
                get_source().frame_ready( std::move( f ) );
            }
        }
    };

    set_processing_callback( std::shared_ptr< rs2_frame_processor_callback >(
        new internal_frame_processor_callback< decltype( f ) >( f ) ) );
}

bool syncer_process_unit::create_matcher( const frame_holder & frame, bool log )
{
    try
    {
        auto sensor = frame.frame->get_sensor().get();
        if (!sensor)
        {
            LOG_DEBUG("Sensor is not exist any more cannot create matcher the frame will passthrough ");
            return false;
        }

        const device_interface * dev = nullptr;
        dev = sensor->get_device().shared_from_this().get();
        if (dev)
        {
            _matcher = dev->create_matcher(frame);

            _matcher->set_callback([this](frame_holder f, const syncronization_environment& env) {
                if (env.log)
                {
                    std::stringstream ss;
                    ss << "SYNCED: ";
                    auto composite = dynamic_cast<composite_frame *>(f.frame);
                    for (int i = 0; i < composite->get_embedded_frames_count(); i++)
                    {
                        auto matched = composite->get_frame(i);
                        ss << matched->get_stream()->get_stream_type() << " " << matched->get_frame_number()
                            << ", " << std::fixed << matched->get_frame_timestamp() << " ";
                    }

                    LOG_DEBUG(ss.str());
                }

                env.matches.enqueue(std::move(f));
            });
        }
        else
        {
            LOG_DEBUG("Device is not exist any more cannot create matcher, the frame will passthrough ");
            return false;
        }

    }
    catch( const std::bad_weak_ptr & )
    {
        LOG_WARNING( "Device destroyed" );
    }

    return true;
    
}
}  // namespace librealsense
