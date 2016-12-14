// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_ARCHIVE_H
#define LIBREALSENSE_ARCHIVE_H

#include "types.h"
#include <atomic>
#include "timestamps.h"

namespace rsimpl
{
    // Defines general frames storage model
    class frame_archive
    {
    public:
        struct frame_additional_data
        {
            double actual_fps = 0;
            double timestamp = 0;
            double exposure_value = 0;
            unsigned long long frame_number = 0;
            long long system_time = 0;
            int width = 0;
            int height = 0;
            int fps = 0;
            int stride_x = 0;
            int stride_y = 0;
            int bpp = 1;
            rs_format format = RS_FORMAT_ANY;
            rs_stream stream_type = RS_STREAM_COUNT;
            rs_timestamp_domain timestamp_domain = RS_TIMESTAMP_DOMAIN_CAMERA;
            int pad = 0;
            std::shared_ptr<std::vector<rs_frame_metadata>> supported_metadata_vector;
            std::chrono::high_resolution_clock::time_point frame_callback_started {};

            frame_additional_data(){};

            frame_additional_data(double in_timestamp, unsigned long long in_frame_number, long long in_system_time, 
                int in_width, int in_height, int in_fps, 
                int in_stride_x, int in_stride_y, int in_bpp, 
                const rs_format in_format, rs_stream in_stream_type, int in_pad, std::shared_ptr<std::vector<rs_frame_metadata>> in_supported_metadata_vector, double in_exposure_value, double in_actual_fps)
                : timestamp(in_timestamp),
                  frame_number(in_frame_number),
                  system_time(in_system_time),
                  width(in_width),
                  height(in_height),
                  fps(in_fps),
                  stride_x(in_stride_x),
                  stride_y(in_stride_y),
                  bpp(in_bpp),
                  format(in_format),
                  stream_type(in_stream_type),
                  pad(in_pad),
                  supported_metadata_vector(in_supported_metadata_vector),
                  exposure_value(in_exposure_value),
                  actual_fps(in_actual_fps){}
        };

        // Define a movable but explicitly noncopyable buffer type to hold our frame data
        struct frame : frame_interface
        {
        private:
            // TODO: check boost::intrusive_ptr or an alternative
            std::atomic<int> ref_count; // the reference count is on how many times this placeholder has been observed (not lifetime, not content)
            frame_archive * owner; // pointer to the owner to be returned to by last observe
            frame_continuation on_release;

        public:
            std::vector<byte> data;
            frame_additional_data additional_data;

            explicit frame() : ref_count(0), owner(nullptr), on_release(){}
            frame(const frame & r) = delete;
            frame(frame && r) 
                : ref_count(r.ref_count.exchange(0)), 
                  owner(r.owner), on_release() 
            {
                *this = std::move(r); // TODO: This is not very safe, refactor later
            }

            frame & operator=(const frame & r) = delete;
            frame& operator=(frame&& r)
            {
                data = move(r.data);
                owner = r.owner;
                ref_count = r.ref_count.exchange(0);
                on_release = std::move(r.on_release);
                additional_data = std::move(r.additional_data);
                return *this;
            }

            ~frame() { on_release.reset(); }

            double get_frame_metadata(rs_frame_metadata frame_metadata) const override;
            bool supports_frame_metadata(rs_frame_metadata frame_metadata) const override;
            const byte* get_frame_data() const;
            double get_frame_timestamp() const;
            rs_timestamp_domain get_frame_timestamp_domain() const;
            void set_timestamp(double new_ts) override { additional_data.timestamp = new_ts; }
            unsigned long long get_frame_number() const override;
            void set_timestamp_domain(rs_timestamp_domain timestamp_domain) override { additional_data.timestamp_domain = timestamp_domain; }
            long long get_frame_system_time() const;
            int get_width() const;
            int get_height() const;
            int get_framerate() const;
            int get_stride() const;
            int get_bpp() const;
            rs_format get_format() const;
            rs_stream get_stream_type() const override;

            std::chrono::high_resolution_clock::time_point get_frame_callback_start_time_point() const;
            void update_frame_callback_start_ts(std::chrono::high_resolution_clock::time_point ts);

            void acquire() { ref_count.fetch_add(1); }
            void release();
            frame* publish();
            void update_owner(frame_archive * new_owner) { owner = new_owner; }
            void attach_continuation(frame_continuation&& continuation) { on_release = std::move(continuation); }
            void disable_continuation() { on_release.reset(); }
        };

        class frame_ref : public rs_frame_ref // esentially an intrusive shared_ptr<frame>
        {
            frame * frame_ptr;
        public:
            frame_ref() : frame_ptr(nullptr) {}

            explicit frame_ref(frame* frame) : frame_ptr(frame)
            {
                if (frame) frame->acquire();
            }

            frame_ref(const frame_ref& other) : frame_ptr(other.frame_ptr)
            {
                if (frame_ptr) frame_ptr->acquire();
            }

            frame_ref(frame_ref&& other) : frame_ptr(other.frame_ptr)
            {
                other.frame_ptr = nullptr;
            }

            frame_ref& operator =(frame_ref other)
            {
                swap(other);
                return *this;
            }

            ~frame_ref()
            {
                if (frame_ptr) frame_ptr->release();
            }

            void swap(frame_ref& other)
            {
                std::swap(frame_ptr, other.frame_ptr);
            }

            void disable_continuation()
            {
                if (frame_ptr) frame_ptr->disable_continuation();
            }

            double get_frame_metadata(rs_frame_metadata frame_metadata) const override;
            bool supports_frame_metadata(rs_frame_metadata frame_metadata) const override;
            const byte* get_frame_data() const override;
            double get_frame_timestamp() const override;
            unsigned long long get_frame_number() const override;
            long long get_frame_system_time() const override;
            rs_timestamp_domain get_frame_timestamp_domain() const override;
            int get_frame_width() const override;
            int get_frame_height() const override;
            int get_frame_framerate() const override;
            int get_frame_stride() const override;
            int get_frame_bpp() const override;
            rs_format get_frame_format() const override;
            rs_stream get_stream_type() const override;
            std::chrono::high_resolution_clock::time_point get_frame_callback_start_time_point() const;
            void update_frame_callback_start_ts(std::chrono::high_resolution_clock::time_point ts);
            void log_callback_start(std::chrono::high_resolution_clock::time_point capture_start_time);
        };

        class frameset
        {
            frame_ref buffer[RS_STREAM_NATIVE_COUNT];
        public:

            frame_ref detach_ref(rs_stream stream);
            void place_frame(rs_stream stream, frame&& new_frame);

            const rs_frame_ref * get_frame(rs_stream stream) const
            {
                return &buffer[stream];
            }

            double get_frame_metadata(rs_stream stream, rs_frame_metadata frame_metadata) const { return buffer[stream].get_frame_metadata(frame_metadata); }
            bool supports_frame_metadata(rs_stream stream, rs_frame_metadata frame_metadata) const { return buffer[stream].supports_frame_metadata(frame_metadata); }
            const byte * get_frame_data(rs_stream stream) const { return buffer[stream].get_frame_data(); }
            double get_frame_timestamp(rs_stream stream) const { return buffer[stream].get_frame_timestamp(); }
            unsigned long long get_frame_number(rs_stream stream) const { return buffer[stream].get_frame_number(); }
            long long get_frame_system_time(rs_stream stream) const { return buffer[stream].get_frame_system_time(); }
            int get_frame_stride(rs_stream stream) const { return buffer[stream].get_frame_stride(); }
            int get_frame_bpp(rs_stream stream) const { return buffer[stream].get_frame_bpp(); }

            void cleanup();
            
        };

    private:
        // This data will be left constant after creation, and accessed from all threads
        subdevice_mode_selection modes[RS_STREAM_NATIVE_COUNT];
        
        std::atomic<uint32_t>* max_frame_queue_size;
        std::atomic<uint32_t> published_frames_per_stream[RS_STREAM_COUNT];
        small_heap<frame, RS_USER_QUEUE_SIZE*RS_STREAM_COUNT> published_frames;
        small_heap<frameset, RS_USER_QUEUE_SIZE*RS_STREAM_COUNT> published_sets;
        small_heap<frame_ref, RS_USER_QUEUE_SIZE*RS_STREAM_COUNT> detached_refs;
        

    protected:
        frame backbuffer[RS_STREAM_NATIVE_COUNT]; // receive frame here
        std::vector<frame> freelist; // return frames here
        std::recursive_mutex mutex;
        std::chrono::high_resolution_clock::time_point capture_started;

    public:
        frame_archive(const std::vector<subdevice_mode_selection> & selection, std::atomic<uint32_t>* max_frame_queue_size, std::chrono::high_resolution_clock::time_point capture_started = std::chrono::high_resolution_clock::now());

        // Safe to call from any thread
        bool is_stream_enabled(rs_stream stream) const { return modes[stream].mode.pf.fourcc != 0; }
        const subdevice_mode_selection & get_mode(rs_stream stream) const { return modes[stream]; }
        
        void release_frameset(frameset * frameset)
        {
            published_sets.deallocate(frameset);
        }
        frameset * clone_frameset(frameset * frameset);

        void unpublish_frame(frame * frame);
        frame * publish_frame(frame && frame);

        frame_ref * detach_frame_ref(frameset * frameset, rs_stream stream);
        frame_ref * clone_frame(frame_ref * frameset);
        void release_frame_ref(frame_ref * ref)
        {
            detached_refs.deallocate(ref);
        }

        // Frame callback thread API
        byte * alloc_frame(rs_stream stream, const frame_additional_data& additional_data, bool requires_memory);
        frame_ref * track_frame(rs_stream stream);
        void attach_continuation(rs_stream stream, frame_continuation&& continuation);
        void log_frame_callback_end(frame* frame);
        void log_callback_start(frame_ref* frame_ref, std::chrono::high_resolution_clock::time_point capture_start_time);

        virtual void flush();

        virtual ~frame_archive() {};
    };
}

#endif
