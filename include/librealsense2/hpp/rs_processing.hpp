// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS2_PROCESSING_HPP
#define LIBREALSENSE_RS2_PROCESSING_HPP

#include "rs_types.hpp"
#include "rs_frame.hpp"
#include "rs_context.hpp"

namespace rs2
{
    class frame_source
    {
    public:
        frame allocate_video_frame(const stream_profile& profile,
            const frame& original,
            int new_bpp = 0,
            int new_width = 0,
            int new_height = 0,
            int new_stride = 0,
            rs2_extension frame_type = RS2_EXTENSION_VIDEO_FRAME) const
        {
            rs2_error* e = nullptr;
            auto result = rs2_allocate_synthetic_video_frame(_source, profile.get(),
                original.get(), new_bpp, new_width, new_height, new_stride, frame_type, &e);
            error::handle(e);
            return result;
        }

        frame allocate_composite_frame(std::vector<frame> frames) const
        {
            rs2_error* e = nullptr;

            std::vector<rs2_frame*> refs(frames.size(), nullptr);
            for (size_t i = 0; i < frames.size(); i++)
                std::swap(refs[i], frames[i].frame_ref);

            auto result = rs2_allocate_composite_frame(_source, refs.data(), (int)refs.size(), &e);
            error::handle(e);
            return result;
        }

        void frame_ready(frame result) const
        {
            rs2_error* e = nullptr;
            rs2_synthetic_frame_ready(_source, result.get(), &e);
            error::handle(e);
            result.frame_ref = nullptr;
        }

        rs2_source* _source;
    private:
        template<class T>
        friend class frame_processor_callback;

        frame_source(rs2_source* source) : _source(source) {}
        frame_source(const frame_source&) = delete;

    };

    template<class T>
    class frame_processor_callback : public rs2_frame_processor_callback
    {
        T on_frame_function;
    public:
        explicit frame_processor_callback(T on_frame) : on_frame_function(on_frame) {}

        void on_frame(rs2_frame* f, rs2_source * source) override
        {
            frame_source src(source);
            frame frm(f);
            on_frame_function(std::move(frm), src);
        }

        void release() override { delete this; }
    };

    class processing_block : public options
    {
    public:
        template<class S>
        void start(S on_frame)
        {
            rs2_error* e = nullptr;
            rs2_start_processing(_block.get(), new frame_callback<S>(on_frame), &e);
            error::handle(e);
        }

        void invoke(frame f) const
        {
            rs2_frame* ptr = nullptr;
            std::swap(f.frame_ref, ptr);

            rs2_error* e = nullptr;
            rs2_process_frame(_block.get(), ptr, &e);
            error::handle(e);
        }

        void operator()(frame f) const
        {
            invoke(std::move(f));
        }

        processing_block(std::shared_ptr<rs2_processing_block> block)
            : options((rs2_options*)block.get()),_block(block)
        {
        }

        template<class S>
        processing_block(S processing_function)
        {
           rs2_error* e = nullptr;
            _block = std::shared_ptr<rs2_processing_block>(
                        rs2_create_processing_block(new frame_processor_callback<S>(processing_function),&e),
                        rs2_delete_processing_block);
            options::operator=(_block);
            error::handle(e);
        }

        operator rs2_options*() const { return (rs2_options*)_block.get(); }

    private:
        std::shared_ptr<rs2_processing_block> _block;
    };


    class frame_queue
    {
    public:
        /**
        * create frame queue. frame queues are the simplest x-platform synchronization primitive provided by librealsense
        * to help developers who are not using async APIs
        * param[in] capacity size of the frame queue
        */
        explicit frame_queue(unsigned int capacity)
        {
            rs2_error* e = nullptr;
            _queue = std::shared_ptr<rs2_frame_queue>(
                    rs2_create_frame_queue(capacity, &e),
                    rs2_delete_frame_queue);
            error::handle(e);
        }

        frame_queue() : frame_queue(1) {}

        /**
        * enqueue new frame into a queue
        * \param[in] f - frame handle to enqueue (this operation passed ownership to the queue)
        */
        void enqueue(frame f) const
        {
            rs2_enqueue_frame(f.frame_ref, _queue.get()); // noexcept
            f.frame_ref = nullptr; // frame has been essentially moved from
        }

        /**
        * wait until new frame becomes available in the queue and dequeue it
        * \return frame handle to be released using rs2_release_frame
        */
        frame wait_for_frame(unsigned int timeout_ms = 5000) const
        {
            rs2_error* e = nullptr;
            auto frame_ref = rs2_wait_for_frame(_queue.get(), timeout_ms, &e);
            error::handle(e);
            return{ frame_ref };
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
            auto res = rs2_poll_for_frame(_queue.get(), &frame_ref, &e);
            error::handle(e);
            if (res) *f = { frame_ref };
            return res > 0;
        }

        void operator()(frame f) const
        {
            enqueue(std::move(f));
        }

    private:
        std::shared_ptr<rs2_frame_queue> _queue;
    };

    class pointcloud
    {
    public:
        pointcloud() :  _queue(1)
        {
            rs2_error* e = nullptr;

            _block = std::make_shared<processing_block>(
                                std::shared_ptr<rs2_processing_block>(
                                                    rs2_create_pointcloud(&e),
                                                    rs2_delete_processing_block));

            error::handle(e);

            _block->start(_queue);
        }

        points calculate(frame depth)
        {
            _block->invoke(std::move(depth));
            return _queue.wait_for_frame();
        }

        void map_to(frame mapped)
        {
            _block->invoke(std::move(mapped));
        }
    private:
        friend class context;

        std::shared_ptr<processing_block> _block;
        frame_queue _queue;
    };

    class asynchronous_syncer
    {
    public:
        asynchronous_syncer()
        {
            rs2_error* e = nullptr;
            _processing_block = std::make_shared<processing_block>(
                    std::shared_ptr<rs2_processing_block>(
                                        rs2_create_sync_processing_block(&e),
                                        rs2_delete_processing_block));

            error::handle(e);
        }

        template<class S>
        void start(S on_frame)
        {
            _processing_block->start(on_frame);
        }

        void operator()(frame f) const
        {
            _processing_block->operator()(std::move(f));
        }
    private:
        std::shared_ptr<processing_block> _processing_block;
    };

    class syncer
    {
    public:
        syncer()
        {
            _sync.start(_results);

        }

        /**
        * Wait until coherent set of frames becomes available
        * \param[in] timeout_ms   Max time in milliseconds to wait until an exception will be thrown
        * \return Set of coherent frames
        */
        frameset wait_for_frames(unsigned int timeout_ms = 5000) const
        {
            return frameset(_results.wait_for_frame(timeout_ms));
        }

        /**
        * Check if a coherent set of frames is available
        * \param[out] result      New coherent frame-set
        * \return true if new frame-set was stored to result
        */
        bool poll_for_frames(frameset* fs) const
        {
            frame result;
            if (_results.poll_for_frame(&result))
            {
                *fs = frameset(result);
                return true;
            }
            return false;
        }

        void operator()(frame f) const
        {
            _sync(std::move(f));
        }
    private:
        asynchronous_syncer _sync;
        frame_queue _results;
    };

    class align
    {
    public:
        align(rs2_stream align_to) :_queue(1)
        {
            rs2_error* e = nullptr;
            _block = std::make_shared<processing_block>(
            std::shared_ptr<rs2_processing_block>(
                rs2_create_align(align_to, &e),
                rs2_delete_processing_block));
            error::handle(e);

            _block->start(_queue);
        }

        frameset proccess(frameset frame)
        {
            (*_block)(frame);
            rs2::frame f;
            _queue.poll_for_frame(&f);
            return frameset(f);
        }

        void operator()(frame f) const
        {
            (*_block)(std::move(f));
        }
    private:
        friend class context;

        std::shared_ptr<processing_block> _block;
        frame_queue _queue;
    };

    class colorizer : public options
    {
    public:
        colorizer() : _queue(1)
        {
            rs2_error* e = nullptr;
            auto pb = std::shared_ptr<rs2_processing_block>(
                rs2_create_colorizer(&e),
                rs2_delete_processing_block);
            _block = std::make_shared<processing_block>(pb);
            error::handle(e);

            // Redirect options API to the processing block
            options::operator=(pb);

            _block->start(_queue);
        }

        video_frame colorize(frame depth) const
        {
            if(depth)
            {
                _block->invoke(std::move(depth));
                return _queue.wait_for_frame();
            }
            return depth;
        }

        video_frame operator()(frame depth) const { return colorize(depth); }

     private:
         std::shared_ptr<processing_block> _block;
         frame_queue _queue;
     };

}
#endif // LIBREALSENSE_RS2_PROCESSING_HPP
