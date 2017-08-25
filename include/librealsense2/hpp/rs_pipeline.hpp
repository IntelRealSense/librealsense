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

        pipeline(device dev)
            : _ctx(context()), _dev(dev)
        {
            rs2_error* e = nullptr;
            _pipeline = std::shared_ptr<rs2_pipeline>(
                rs2_create_pipeline_with_device(_ctx._context.get(), _dev.get().get(), &e),
                rs2_delete_pipeline);
            error::handle(e);
        }

        /**
        * Retrieved the device used by the pipeline
        * \param[in] ctx   context
        * \param[in] pipe  pipeline
        * \param[out] error  if non-null, receives any error that occurs during this call, otherwise, errors are ignored
        * \return the device used by the pipeline
        */
        device get_device() const
        {
            rs2_error* e = nullptr;
            std::shared_ptr<rs2_device> dev(
                rs2_pipeline_get_device(_ctx._context.get(), _pipeline.get(), &e),
                rs2_delete_device);

            error::handle(e);

            return device(dev);
        }

        /**
        * Start streaming with default configuration or configuration commited by enable_stream
        */
        void start() const
        {
            rs2_error* e = nullptr;
            rs2_start_pipeline(_pipeline.get(), &e);
            error::handle(e);
        }

        /**
        * Start passing frames into user provided callback
        * \param[in] callback   Stream callback, can be any callable object accepting rs2::frame
        */
        template<class T>
        void start(T callback) const
        {
            rs2_error* e = nullptr;
            rs2_start_pipeline_with_callback_cpp(_pipeline.get(), new frame_callback<T>(std::move(callback)), &e);
            error::handle(e);
        }

        /**
        * Stop streaming
        */
        void stop() const
        {
            rs2_error* e = nullptr;
            rs2_stop_pipeline(_pipeline.get(), &e);
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
        void enable_stream(rs2_stream stream, int index, int width, int height, rs2_format format, int framerate) const
        {
            rs2_error* e = nullptr;
            rs2_enable_stream_pipeline(_pipeline.get(), stream, index, width, height, format, framerate, &e);
            error::handle(e);
        }

        /**
        *  remove a configuration from the pipeline
        * \param[in] stream    stream type
        */
        void disable_stream(rs2_stream stream) const
        {
            rs2_error* e = nullptr;
            rs2_disable_stream_pipeline(_pipeline.get(), stream, &e);
            error::handle(e);
        }

        /**
        *  remove all streams from the pipeline
        * \param[in] stream    stream type
        */
        void disable_all() const
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

        /**
        * poll if a new frame is available and dequeue if it is
        * \param[out] f - frame handle
        * \return true if new frame was stored to f
        */
        bool poll_for_frame(frame* f) const
        {
            rs2_error* e = nullptr;
            rs2_frame* frame_ref = nullptr;
            auto res = rs2_pipeline_poll_for_frames(_pipeline.get(), &frame_ref, &e);
            error::handle(e);
            if (res) *f = { frame_ref };
            return res > 0;
        }
    private:
        context _ctx;
        device _dev;
        std::shared_ptr<rs2_pipeline> _pipeline;
    };
}
#endif // LIBREALSENSE_RS2_PROCESSING_HPP
