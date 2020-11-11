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
                if( !f )
                    continue;
                worker_body( f.as< rs2::frameset >() );
            }
            LOG(DEBUG) << "End of worker loop in " + get_name();
            worker_end();
        } );
    }

protected:
    rs2::frameset process_frameset(rs2::frame fs) override
    {
        std::vector<rs2::frame> bundle;
        rs2::processing_block bundler([&](rs2::frame f, rs2::frame_source& src) {
            bundle.push_back(f);
            if (bundle.size())
            {
                auto fs = src.allocate_composite_frame(bundle);
                src.frame_ready(fs);
                bundle.clear();
            }
            });

        if (fs.as< rs2::frameset >().size() == 0)
        {
            rs2::frame_queue q;
            bundler.start(q); // Results from the block will be enqueued into q
            bundler.invoke(fs); // Invoke the lambda above
            rs2::frameset output_frame = q.wait_for_frame(); // Fetch the result
            _queue.enqueue(output_frame);
            return output_frame;
        }

        _queue.enqueue(fs);
        return fs;
    }

    virtual void worker_start() {}
    virtual void worker_end() {}

    virtual void worker_body( rs2::frameset fs ) = 0;
};
