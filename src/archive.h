// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"
#include <atomic>
#include "timestamps.h"

namespace rsimpl
{
    class frame_archive;
}

struct frame_additional_data
{
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
    std::vector<rs_frame_metadata> supported_metadata_vector;
    std::chrono::high_resolution_clock::time_point frame_callback_started{};

    frame_additional_data() {};

    frame_additional_data(double in_timestamp, unsigned long long in_frame_number, long long in_system_time,
        int in_width, int in_height, int in_fps,
        int in_stride_x, int in_stride_y, int in_bpp,
        const rs_format in_format, rs_stream in_stream_type, int in_pad, std::vector<rs_frame_metadata> in_supported_metadata_vector, double in_exposure_value)
        : timestamp(in_timestamp),
          exposure_value(in_exposure_value),
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
          supported_metadata_vector(in_supported_metadata_vector) {}
};

// Define a movable but explicitly noncopyable buffer type to hold our frame data
struct frame : rsimpl::frame_interface
{
private:
    // TODO: check boost::intrusive_ptr or an alternative
    std::atomic<int> ref_count; // the reference count is on how many times this placeholder has been observed (not lifetime, not content)
    rsimpl::frame_archive * owner; // pointer to the owner to be returned to by last observe
    rsimpl::frame_continuation on_release;

public:
    std::vector<rsimpl::byte> data;
    frame_additional_data additional_data;

    explicit frame() : ref_count(0), owner(nullptr), on_release() {}
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
    const rsimpl::byte* get_frame_data() const;
    double get_frame_timestamp() const;
    rs_timestamp_domain get_frame_timestamp_domain() const;
    void set_timestamp(double new_ts) override { additional_data.timestamp = new_ts; }
    unsigned long long get_frame_number() const override;

    void set_timestamp_domain(rs_timestamp_domain timestamp_domain) override 
    { 
        additional_data.timestamp_domain = timestamp_domain; 
    }

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
    void update_owner(rsimpl::frame_archive * new_owner) { owner = new_owner; }
    void attach_continuation(rsimpl::frame_continuation&& continuation) { on_release = std::move(continuation); }
    void disable_continuation() { on_release.reset(); }
};

struct rs_frame_ref // esentially an intrusive shared_ptr<frame>
{
    rs_frame_ref() : frame_ptr(nullptr) {}

    explicit rs_frame_ref(frame* frame) : frame_ptr(frame)
    {
        if (frame) frame->acquire();
    }

    rs_frame_ref(const rs_frame_ref& other) : frame_ptr(other.frame_ptr)
    {
        if (frame_ptr) frame_ptr->acquire();
    }

    rs_frame_ref(rs_frame_ref&& other) : frame_ptr(other.frame_ptr)
    {
        other.frame_ptr = nullptr;
    }

    rs_frame_ref& operator =(rs_frame_ref other)
    {
        swap(other);
        return *this;
    }

    ~rs_frame_ref()
    {
        if (frame_ptr) frame_ptr->release();
    }

    void swap(rs_frame_ref& other)
    {
        std::swap(frame_ptr, other.frame_ptr);
    }

    void disable_continuation()
    {
        if (frame_ptr) frame_ptr->disable_continuation();
    }

    double get_frame_metadata(rs_frame_metadata frame_metadata) const;
    bool supports_frame_metadata(rs_frame_metadata frame_metadata) const;
    const rsimpl::byte* get_frame_data() const;
    double get_frame_timestamp() const;
    unsigned long long get_frame_number() const;
    long long get_frame_system_time() const;
    rs_timestamp_domain get_frame_timestamp_domain() const;
    int get_frame_width() const;
    int get_frame_height() const;
    int get_frame_framerate() const;
    int get_frame_stride() const;
    int get_frame_bpp() const;
    rs_format get_frame_format() const;
    rs_stream get_stream_type() const;
    std::chrono::high_resolution_clock::time_point get_frame_callback_start_time_point() const;
    void update_frame_callback_start_ts(std::chrono::high_resolution_clock::time_point ts) const;
    void log_callback_start(std::chrono::high_resolution_clock::time_point capture_start_time) const;

private:
    frame * frame_ptr;
};

namespace rsimpl
{
    // Defines general frames storage model
    class frame_archive
    {        
        std::atomic<uint32_t>* max_frame_queue_size;
        std::atomic<uint32_t> published_frames_count;
        small_heap<frame, RS_USER_QUEUE_SIZE> published_frames;
        small_heap<rs_frame_ref, RS_USER_QUEUE_SIZE> detached_refs;

        std::vector<frame> freelist; // return frames here
        std::recursive_mutex mutex;
        std::chrono::high_resolution_clock::time_point capture_started;

    public:
        explicit frame_archive(std::atomic<uint32_t>* max_frame_queue_size, 
                               std::chrono::high_resolution_clock::time_point capture_started 
                                    = std::chrono::high_resolution_clock::now());

        void unpublish_frame(frame * frame);
        frame * publish_frame(frame && frame);

        rs_frame_ref * clone_frame(rs_frame_ref * frameset);
        void release_frame_ref(rs_frame_ref * ref)
        {
            detached_refs.deallocate(ref);
        }

        // Frame callback thread API
        frame alloc_frame(uint8_t* data, const size_t size, const frame_additional_data& additional_data, bool requires_memory);
        rs_frame_ref * track_frame(frame& f);
        void log_frame_callback_end(frame* frame) const;

        void flush();
    };
}