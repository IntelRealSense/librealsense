// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_SYNC_H
#define LIBREALSENSE_SYNC_H

#include "types.h"
#include <memory>
#include <atomic>

namespace rsimpl
{
    class frame_archive
    {
    public:
        template<class T, int C>
        class small_heap
        {
            T buffer[C];
            bool is_free[C];
            std::mutex mutex;

        public:
            small_heap()
            {
                for (auto i = 0; i < C; i++)
                {
                    is_free[i] = true;
                    buffer[i] = std::move(T());
                }
            }

            T * allocate()
            {
                std::lock_guard<std::mutex> lock(mutex);
                for (auto i = 0; i < C; i++)
                {
                    if (is_free[i])
                    {
                        is_free[i] = false;
                        return &buffer[i];
                    }
                }
                return nullptr;
            }

            void deallocate(T * item)
            {
                std::lock_guard<std::mutex> lock(mutex);
                if (item < buffer || item >= buffer + C)
                {
                    throw std::runtime_error("Trying to return item to a heap that didn't allocate it!");
                }
                auto i = item - buffer;
                is_free[i] = true;
                buffer[i] = std::move(T());
            }
        };

        // Define a movable but explicitly noncopyable buffer type to hold our frame data
        struct frame 
        { 
        private:
            // TODO: consider boost::intrusive_ptr or an alternative
            std::atomic<int> ref_count; // the reference count is on how many times this placeholder has been observed (not lifetime, not content)
            frame_archive * owner; // pointer to the owner to be returned to by last observer

        public:
            std::vector<byte> data;
            int timestamp;

            explicit frame() : ref_count(0), owner(nullptr), timestamp() {}
            frame(const frame & r) = delete;
            frame(frame && r) : ref_count(r.ref_count.exchange(0)), owner(r.owner) { *this = std::move(r); }

            frame & operator = (const frame & r) = delete;
            frame & operator = (frame && r) 
            { 
                data = move(r.data); 
                timestamp = r.timestamp; 
                owner = r.owner;
                ref_count = r.ref_count.exchange(0);
                return *this; 
            }

            void acquire() { ref_count.fetch_add(1); }
            void release()
            {
                if (ref_count.fetch_sub(1) == 1)
                {
                    owner->unpublish_frame(this);
                }
            }
            frame * publish()
            {
                return owner->publish_frame(std::move(*this));
            }
            void update_owner(frame_archive * new_owner) { owner = new_owner; }
        };

        struct frame_ref // esential an intrusive shared_ptr<frame>
        {
            frame * frame_ptr;

            frame_ref() : frame_ptr(nullptr) {}
            explicit frame_ref(frame * frame) : frame_ptr(frame)
            {
                frame->acquire();
            }

            frame_ref(const frame_ref & other) : frame_ptr(other.frame_ptr)
            {
                if (frame_ptr) frame_ptr->acquire();
            }
            frame_ref(frame_ref && other) : frame_ptr(other.frame_ptr)
            {
                other.frame_ptr = nullptr;
            }

            frame_ref & operator = (frame_ref other)
            {
                swap(other);
                return *this;
            }

            ~frame_ref()
            {
                if (frame_ptr) frame_ptr->release();
            }

            void swap(frame_ref & other)
            {
                std::swap(frame_ptr, other.frame_ptr);
            }

            const byte * get_frame_data() const
            {
                return frame_ptr ? frame_ptr->data.data() : nullptr;
            }

            int get_frame_timestamp() const
            {
                return frame_ptr ? frame_ptr->timestamp : 0;
            }
        };

        struct frameset
        {
            frame_ref buffer[RS_STREAM_NATIVE_COUNT];

            frame_ref detach_ref(rs_stream stream)
            {
                return std::move(buffer[stream]);
            }

            void place_frame(rs_stream stream, frame&& new_frame)
            {
                auto published_frame = new_frame.publish();
                if (published_frame)
                {
                    frame_ref new_ref(published_frame); // allocate new frame_ref to ref-counter the now published frame
                    buffer[stream] = std::move(new_ref); // move the new handler in, release a ref count on previous frame
                }
            }

            const byte * get_frame_data(rs_stream stream) const { return buffer[stream].get_frame_data(); }
            int get_frame_timestamp(rs_stream stream) const { return buffer[stream].get_frame_timestamp(); }
        };

    private:
        // This data will be left constant after creation, and accessed from all threads
        subdevice_mode_selection modes[RS_STREAM_NATIVE_COUNT];
        rs_stream key_stream;
        std::vector<rs_stream> other_streams;

        // This data will be read and written exclusively from the application thread
        frameset frontbuffer;

        // This data will be read and written by all threads, and synchronized with a mutex
        std::vector<frame> frames[RS_STREAM_NATIVE_COUNT];
        std::vector<frame> freelist;
		small_heap<frame, RS_USER_QUEUE_SIZE> published_frames;
		small_heap<frameset, RS_USER_QUEUE_SIZE> published_sets;
		small_heap<frame_ref, RS_USER_QUEUE_SIZE> detached_refs;
        std::mutex mutex;
        std::condition_variable cv;

        // This data will be read and written exclusively from the frame callback thread
        frame backbuffer[RS_STREAM_NATIVE_COUNT];

        void get_next_frames();
        void dequeue_frame(rs_stream stream);
        void discard_frame(rs_stream stream);
        void cull_frames();
    public:
        frame_archive(const std::vector<subdevice_mode_selection> & selection, rs_stream key_stream);

        // Safe to call from any thread
        bool is_stream_enabled(rs_stream stream) const { return modes[stream].mode.pf.fourcc != 0; }
        const subdevice_mode_selection & get_mode(rs_stream stream) const { return modes[stream]; }

        // Application thread API
        void wait_for_frames();
        bool poll_for_frames();

        frameset * wait_for_frames_safe();
        bool poll_for_frames_safe(frameset ** frames);

        const byte * get_frame_data(rs_stream stream) const;
        int get_frame_timestamp(rs_stream stream) const;

        frameset * clone_frontbuffer()
        {
			return clone_frameset(&frontbuffer);
        }
        void release_frameset(frameset* frameset)
        {
            published_sets.deallocate(frameset);
        }
		frameset * clone_frameset(frameset * frameset)
		{
			auto new_set = published_sets.allocate();
			if (new_set)
			{
				*new_set = *frameset;
			}
			return new_set;
		}

        void unpublish_frame(frame * frame)
        {
            freelist.push_back(std::move(*frame));
            published_frames.deallocate(frame);
        }
        frame * publish_frame(frame&& frame)
        {
            auto new_frame = published_frames.allocate();
            if (new_frame)
            {
                *new_frame = std::move(frame);
            }
            return new_frame;
        }

        frame_ref * detach_frame_ref(frameset* frameset, rs_stream stream)
        {
            auto new_ref = detached_refs.allocate();
            if (new_ref)
            {
                *new_ref = std::move(frameset->detach_ref(stream));
            }
            return new_ref;
        }
		frame_ref * clone_frame(frame_ref * frameset)
		{
			auto new_ref = detached_refs.allocate();
			if (new_ref)
			{
				*new_ref = *frameset;
			}
			return new_ref;
		}


        void release_frame_ref(frame_ref * ref)
        {
            detached_refs.deallocate(ref);
        }

        // Frame callback thread API
        byte * alloc_frame(rs_stream stream, int timestamp);
        void commit_frame(rs_stream stream);  
    };
}

#endif