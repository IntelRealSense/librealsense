// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS2_PIPELINE_HPP
#define LIBREALSENSE_RS2_PIPELINE_HPP

#include "rs2_types.hpp"
#include "rs2_frame.hpp"
#include "rs2_context.hpp"


namespace rs2
{
    class pipeline
    {
    public:  
        pipeline(context ctx = context())
            : _ctx(ctx)
        {
            rs2_error* e = nullptr;
            _pipeline = std::shared_ptr<rs2_pipeline>(
                rs2_create_pipeline(ctx._context.get(), &e),
                rs2_delete_pipeline);
            error::handle(e);
        }

        void start()
        {
            rs2_error* e = nullptr;
            rs2_start_pipeline(_pipeline.get(), &e);
            error::handle(e);
        }

        frame_set wait_for_frames(unsigned int timeout_ms = 5000) const
        {
            rs2_error* e = nullptr;
            frame f (rs2_pipeline_wait_for_frames(_pipeline.get(), timeout_ms, &e));
            error::handle(e);

            return frame_set(f);
        }

    private:
        context _ctx;
        std::shared_ptr<rs2_pipeline> _pipeline;
    };
}
#endif // LIBREALSENSE_RS2_PROCESSING_HPP
