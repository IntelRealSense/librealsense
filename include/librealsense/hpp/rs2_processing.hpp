// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS2_PROCESSING_HPP
#define LIBREALSENSE_RS2_PROCESSING_HPP

#include "rs2_types.hpp"
#include "rs2_frame.hpp"
#include "rs2_context.hpp"

namespace rs2
{
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

    class processing_block
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
            : _block(block)
        {
        }

    private:
        friend class context;
        friend class syncer_processing_block;

        std::shared_ptr<rs2_processing_block> _block;
    };

    class syncer_processing_block
    {
    public:
        syncer_processing_block()
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
        frame wait_for_frame() const
        {
            rs2_error* e = nullptr;
            auto frame_ref = rs2_wait_for_frame(_queue.get(), &e);
            error::handle(e);
            return{ frame_ref };
        }

        frameset wait_for_frames() const
        {
            auto f = wait_for_frame();
            auto comp = f.as<composite_frame>();
            if (comp)
            {
                return std::move(comp.get_frames());
            }
            else
            {
                frameset res(1);
                res[0] = std::move(f);
                return std::move(res);
            }
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

        bool poll_for_frames(frameset* frames) const
        {
            frame f;
            if (poll_for_frame(&f))
            {
                if (auto comp = f.as<composite_frame>())
                {
                    *frames = std::move(comp.get_frames());
                }
                else
                {
                    frameset res(1);
                    res[0] = std::move(f);
                    *frames = std::move(res);
                }
                return true;
            }
            return false;
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
        points calculate(frame depth)
        {
            _block.invoke(std::move(depth));
            return _queue.wait_for_frame();
        }

        void map_to(frame mapped)
        {
            _block.invoke(std::move(mapped));
        }
    private:
        friend class context;

        pointcloud(processing_block block) : _block(block), _queue(1)
        {
            _block.start(_queue);
        }

        processing_block _block;
        frame_queue _queue;
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
            return _results.wait_for_frames();
        }

        /**
        * Check if a coherent set of frames is available
        * \param[out] result      New coherent frame-set
        * \return true if new frame-set was stored to result
        */
        bool poll_for_frames(frameset* result) const
        {
            return _results.poll_for_frames(result);
        }

        void operator()(frame f) const
        {
            _sync(std::move(f));
        }
    private:
        syncer_processing_block _sync;
        frame_queue _results;
    };

    class depth_frame : public video_frame
    {
    public:
        depth_frame(const frame& f)
            : video_frame(f)
        {
            rs2_error* e = nullptr;
            if (!f || (rs2_is_frame_extendable_to(f.get(), RS2_EXTENSION_DEPTH_FRAME, &e) == 0 && !e))
            {
                reset();
            }
            error::handle(e);
        }

        float get_distance(int x, int y) const
        {
            rs2_error * e = nullptr;
            auto r = rs2_depth_frame_get_distance(get(), x, y, &e);
            error::handle(e);
            return r;
        }
    };
    class frame_set
    {
    public:
        unsigned int size()
        {
            if(!_frame)
            {
                return 0;
            }

            auto comp = _frame.as<composite_frame>();
            if (comp)
            {
                return comp.size();
            }
        }

        frame get_depth()
        {
            frame res;
            if(!_frame)
            {
                return res;
            }

            auto comp = _frame.as<composite_frame>();
            if (comp)
            {
                for(auto&& f:comp.get_frames())
                {
                   auto depth = f.as<depth_frame>();
                   if(depth)
                       return depth;
                }
            }
            else
            {
                auto depth = _frame.as<depth_frame>();
                if(depth)
                    return depth;

            }
            return res;
        }

        frame get_color()
        {
            frame res;
            if(!_frame)
            {
                return res;
            }

            auto comp = _frame.as<composite_frame>();
            if (comp)
            {
                for(auto&& f:comp.get_frames())
                {
                   if(f.get_profile().stream_type() == RS2_STREAM_COLOR ||
                           (f.get_profile().stream_type() == RS2_STREAM_INFRARED && f.get_profile().format() == RS2_FORMAT_RGB8))
                       return f;

                }
            }
            else
            {
                if(_frame.get_profile().stream_type() == RS2_STREAM_COLOR ||
                        (_frame.get_profile().stream_type() == RS2_STREAM_INFRARED && _frame.get_profile().format() == RS2_FORMAT_RGB8))
                    return _frame;

            }
            return res;
        }
    private:
        friend class pipeline;
        frame_set(frame f):_frame(f){}
        frame _frame;

    };

    class colorizer
    {
    public:
        colorizer():
            _queue(1)
        {
            rs2_error* e = nullptr;
            _block = std::make_shared<processing_block>(
                        std::shared_ptr<rs2_processing_block>(
                            rs2_create_colorizer(&e),
                            rs2_delete_processing_block));
            error::handle(e);

            _block->start(_queue);
        }


        video_frame colorize(frame depth)
        {
            if(depth)
            {
                _block->invoke(std::move(depth));
                return _queue.wait_for_frame();
            }
            return depth;
        }
        //void set_bounds(float min, float max)
         //{
         //    rs2_colorizer_set_bounds
         //}

         //void set_equalize(bool equalize)
         //{

         //}
     private:
         std::shared_ptr<processing_block> _block;
         frame_queue _queue;
     };

}
#endif // LIBREALSENSE_RS2_PROCESSING_HPP
