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
        release_background_worker();
    }

    // Worker thread uses resources of classes that inherit from this class (e.g openvino_face_detection),
    // so it should be released from inherited classes.
    // This should be called from dtor of inherited classes!
    void release_background_worker()
    {
        _alive = false;
        if (_worker.joinable())
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
                if (!f)
                    continue;

                worker_body(f);
            }
            LOG(DEBUG) << "End of worker loop in " + get_name();
            worker_end();
        } );
    }

protected:
    rs2::frame process_frame(rs2::frame fs) override
    {
        _queue.enqueue(fs);
        return fs;
    }

    virtual void worker_start() {}
    virtual void worker_end() {}

    virtual void worker_body( rs2::frame fs ) = 0;
};
