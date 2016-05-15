// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_SYNC_H
#define LIBREALSENSE_SYNC_H

#include "types.h"
#include <atomic>

namespace rsimpl
{
    class frame_archive
    {
    public:
        // Define a movable but explicitly noncopyable buffer type to hold our frame data
        struct frame 
        { 
        private:
            // TODO: consider boost::intrusive_ptr or an alternative
            std::atomic<int> ref_count; // the reference count is on how many times this placeholder has been observed (not lifetime, not content)
            frame_archive * owner; // pointer to the owner to be returned to by last observe
			frame_continuation on_release;

        public:
            std::vector<byte> data;
            int timestamp;

            explicit frame() : ref_count(0), owner(nullptr), timestamp() {}
            frame(const frame & r) = delete;
			frame(frame && r) : ref_count(r.ref_count.exchange(0)), owner(r.owner), 
				on_release() { *this = std::move(r); }

            frame & operator = (const frame & r) = delete;
	        frame& operator =(frame&& r);

            const byte* get_frame_data() const;

	        void acquire() { ref_count.fetch_add(1); }
	        void release();
	        frame* publish();
	        void update_owner(frame_archive * new_owner) { owner = new_owner; }
			void attach_continuation(frame_continuation&& continuation) { on_release = std::move(continuation); }
        };

        class frame_ref // esentially an intrusive shared_ptr<frame>
        {
            frame * frame_ptr;
		public:
            frame_ref() : frame_ptr(nullptr) {}
	        explicit frame_ref(frame* frame);
	        frame_ref(const frame_ref& other);
	        frame_ref(frame_ref&& other);
	        frame_ref& operator =(frame_ref other);
	        ~frame_ref();
	        void swap(frame_ref& other);

	        const byte* get_frame_data() const;
	        int get_frame_timestamp() const;
        };

        class frameset
        {
            frame_ref buffer[RS_STREAM_NATIVE_COUNT];
		public:

	        frame_ref detach_ref(rs_stream stream);
	        void place_frame(rs_stream stream, frame&& new_frame);

	        const byte * get_frame_data(rs_stream stream) const { return buffer[stream].get_frame_data(); }
            int get_frame_timestamp(rs_stream stream) const { return buffer[stream].get_frame_timestamp(); }

			void cleanup();
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

	    frameset * clone_frontbuffer();
	    void release_frameset(frameset * frameset);
	    frameset * clone_frameset(frameset * frameset);

	    void unpublish_frame(frame * frame);
	    frame * publish_frame(frame && frame);

	    frame_ref * detach_frame_ref(frameset * frameset, rs_stream stream);
	    frame_ref * clone_frame(frame_ref * frameset);
	    void release_frame_ref(frame_ref * ref);

	    // Frame callback thread API
        byte * alloc_frame(rs_stream stream, int timestamp, bool requires_memory);
		void commit_frame(rs_stream stream);
		frame_ref * track_frame(rs_stream stream);
		void attach_continuation(rs_stream stream, frame_continuation&& continuation);

		void flush();
    };
}

#endif
