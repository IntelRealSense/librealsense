// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS2_PIPELINE_HPP
#define LIBREALSENSE_RS2_PIPELINE_HPP

#include "rs_types.hpp"
#include "rs_frame.hpp"
#include "rs_context.hpp"


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

        /**
        * Start streaming with default configuration or configuration commited by enable_stream
        */
        void start()
        {
            rs2_error* e = nullptr;
            rs2_start_pipeline(_pipeline.get(), &e);
            error::handle(e);
        }

        /**
        * committing a configuration to the pipeline
        * \param[in] stream    stream type
        * \param[in] index     stream index
        * \param[in] width     width
        * \param[in] height    height
        * \param[in] format    stream format
        * \param[in] framerate    stream framerate
        */
        void enable_stream(rs2_stream stream, int index, int width, int height, rs2_format format, int framerate)
        {
            rs2_error* e = nullptr;
            rs2_enable_stream_pipeline(_pipeline.get(), stream, index, width, height, format, framerate, &e);
            error::handle(e);
        }

        /**
        *  remove a configuration from the pipeline
        * \param[in] stream    stream type
        */
        void disable_stream(rs2_stream stream)
        {
            rs2_error* e = nullptr;
            rs2_disable_stream_pipeline(_pipeline.get(), stream, &e);
            error::handle(e);
        }

        /**
        *  remove all streams from the pipeline
        * \param[in] stream    stream type
        */
        void disable_all()
        {
            rs2_error* e = nullptr;
            rs2_disable_all_streams_pipeline(_pipeline.get(), &e);
            error::handle(e);
        }

        /**
        * wait until new frame becomes available
        * \param[in] timeout_ms   Max time in milliseconds to wait until an exception will be thrown
        * \return Set of coherent frames
        */
        frameset wait_for_frames(unsigned int timeout_ms = 5000) const
        {
            rs2_error* e = nullptr;
            frame f (rs2_pipeline_wait_for_frames(_pipeline.get(), timeout_ms, &e));
            error::handle(e);

            return frameset(f);
        }

    private:
        context _ctx;
        std::shared_ptr<rs2_pipeline> _pipeline;
    };
}
#endif // LIBREALSENSE_RS2_PROCESSING_HPP
