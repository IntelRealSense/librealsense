// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#pragma once

#include "post-processing-filter.h"


/*
    A post-processing filter that employs a separate thread to do its work.
*/
class post_processing_worker_filter : public post_processing_filter
{
    std::atomic_bool _alive { true };
    std::thread _worker;
    rs2::frame_queue _queue;   // size 1!

protected:
    post_processing_worker_filter( std::string const & name )
        : post_processing_filter( name )
    {
    }
    ~post_processing_worker_filter()
    {
        _alive = false;
        if( _worker.joinable() )
            _worker.join();
    }

public:
    void start( rs2::subdevice_model & model ) override
    {
        post_processing_filter::start( model );
        _worker = std::thread( [&]()
        {
            try
            {
                worker_start();
            }
            catch( std::exception const & e )
            {
                // Most likely file not found, if the user didn't set up his .xml/.bin files right
                LOG( ERROR ) << "Cannot start " << get_name() << ": " << e.what();
                return;
            }
            while( _alive )
            {
                rs2::frame f;
                if( !_queue.try_wait_for_frame( &f ) )
                    continue;
                if( !f )
                    continue;
                worker_body( f.as< rs2::frameset >() );
            }
            LOG(DEBUG) << "End of worker loop in " + get_name();
            worker_end();
        } );
    }

protected:
    rs2::frameset process_frameset( rs2::frameset fs ) override
    {
        _queue.enqueue( fs );
        return fs;
    }

    virtual void worker_start() {}
    virtual void worker_end() {}

    virtual void worker_body( rs2::frameset fs ) = 0;
};
