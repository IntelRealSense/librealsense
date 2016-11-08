// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"
#include <atomic>

namespace rsimpl
{
    class frame_archive;
}

struct frame_additional_data
{
    double timestamp = 0;
    unsigned long long frame_number = 0;
    long long system_time = 0;
    int width = 0;
    int height = 0;
    int fps = 0;
    int stride = 0;
    int bpp = 1;
    rs_format format = RS_FORMAT_ANY;
    rs_stream stream_type = RS_STREAM_COUNT;
    rs_timestamp_domain timestamp_domain = RS_TIMESTAMP_DOMAIN_CAMERA;
    std::chrono::high_resolution_clock::time_point frame_callback_started{};

    frame_additional_data() {};

    frame_additional_data(double in_timestamp, unsigned long long in_frame_number, long long in_system_time,
        int in_width, int in_height, int in_fps,
        int in_stride, int in_bpp,
        const rs_format in_format, rs_stream in_stream_type)
        : timestamp(in_timestamp),
          frame_number(in_frame_number),
          system_time(in_system_time),
          width(in_width),
          height(in_height),
          fps(in_fps),
          stride(in_stride),
          bpp(in_bpp),
          format(in_format),
          stream_type(in_stream_type) {}
};

// Define a movable but explicitly noncopyable buffer type to hold our frame data
struct frame
{
private:
    // TODO: check boost::intrusive_ptr or an alternative
    std::atomic<int> ref_count; // the reference count is on how many times this placeholder has been observed (not lifetime, not content)
    std::shared_ptr<rsimpl::frame_archive> owner; // pointer to the owner to be returned to by last observe
    rsimpl::frame_continuation on_release;

public:
    std::vector<byte> data;
    frame_additional_data additional_data;

    explicit frame() : ref_count(0), owner(nullptr), on_release() {}
    frame(const frame& r) = delete;
    frame(frame&& r)
        : ref_count(r.ref_count.exchange(0)),
          owner(r.owner), on_release()
    {
        *this = std::move(r);
    }

    frame& operator=(const frame& r) = delete;
    frame& operator=(frame&& r)
    {
        data = move(r.data);
        owner = r.owner;
        ref_count = r.ref_count.exchange(0);
        on_release = std::move(r.on_release);
        additional_data = std::move(r.additional_data);
        r.owner.reset();
        return *this;
    }

    ~frame() { on_release.reset(); }

    double get_frame_metadata(rs_frame_metadata frame_metadata) const;
    bool supports_frame_metadata(rs_frame_metadata frame_metadata) const;
    const byte* get_frame_data() const;
    double get_frame_timestamp() const;
    rs_timestamp_domain get_frame_timestamp_domain() const;
    void set_timestamp(double new_ts) { additional_data.timestamp = new_ts; }
    unsigned long long get_frame_number() const;

    void set_timestamp_domain(rs_timestamp_domain timestamp_domain)
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
    rs_stream get_stream_type() const;

    std::chrono::high_resolution_clock::time_point get_frame_callback_start_time_point() const;
    void update_frame_callback_start_ts(std::chrono::high_resolution_clock::time_point ts);

    void acquire() { ref_count.fetch_add(1); }
    void release();
    frame* publish(std::shared_ptr<rsimpl::frame_archive> new_owner);
    void attach_continuation(rsimpl::frame_continuation&& continuation) { on_release = std::move(continuation); }
    void disable_continuation() { on_release.reset(); }

    rsimpl::frame_archive* get_owner() const { return owner.get(); }
};

struct rs_frame // esentially an intrusive shared_ptr<frame>
{
    rs_frame() : frame_ptr(nullptr) {}

    explicit rs_frame(frame* frame)
        : frame_ptr(frame)
    {
        if (frame) frame->acquire();
    }

    rs_frame(const rs_frame& other)
        : frame_ptr(other.frame_ptr)
    {
        if (frame_ptr) frame_ptr->acquire();
    }

    rs_frame(rs_frame&& other)
        : frame_ptr(other.frame_ptr)
    {
        other.frame_ptr = nullptr;
    }

    rs_frame& operator=(rs_frame other)
    {
        swap(other);
        return *this;
    }

    ~rs_frame()
    {
        if (frame_ptr) frame_ptr->release();
    }

    void swap(rs_frame& other)
    {
        std::swap(frame_ptr, other.frame_ptr);
    }

    void disable_continuation() const
    {
        if (frame_ptr) frame_ptr->disable_continuation();
    }

    frame* get() const { return frame_ptr; }

    void log_callback_start(std::chrono::high_resolution_clock::time_point capture_start_time) const;

private:
    frame * frame_ptr = nullptr;
};

struct callback_invokation
{
    std::chrono::high_resolution_clock::time_point started;
    std::chrono::high_resolution_clock::time_point ended;
};

typedef rsimpl::small_heap<callback_invokation, 1> callbacks_heap;

struct callback_invokation_holder
{
    callback_invokation_holder(const callback_invokation_holder&) = delete;
    callback_invokation_holder& operator=(const callback_invokation_holder&) = delete;

    callback_invokation_holder(callback_invokation_holder&& other)
        : invokation(other.invokation), owner(other.owner)
    {
        other.invokation = nullptr;
    }

    callback_invokation_holder(callback_invokation* invokation, callbacks_heap& owner)
        : invokation(invokation), owner(owner)
    { }

    ~callback_invokation_holder()
    {
        if (invokation) owner.deallocate(invokation);
    }
private:
    callback_invokation* invokation;
    callbacks_heap& owner;
};

namespace rsimpl
{
    // Defines general frames storage model
    class frame_archive : public std::enable_shared_from_this<frame_archive>
    {
        std::atomic<uint32_t>* max_frame_queue_size;
        std::atomic<uint32_t> published_frames_count;
        small_heap<frame, RS_USER_QUEUE_SIZE> published_frames;
        small_heap<rs_frame, RS_USER_QUEUE_SIZE> detached_refs;
        callbacks_heap callback_inflight;

        std::vector<frame> freelist; // return frames here
        std::atomic<bool> recycle_frames;
        int pending_frames = 0;
        std::recursive_mutex mutex;
        std::chrono::high_resolution_clock::time_point capture_started;

    public:
        explicit frame_archive(std::atomic<uint32_t>* max_frame_queue_size,
                               std::chrono::high_resolution_clock::time_point capture_started
                                    = std::chrono::high_resolution_clock::now());

        callback_invokation_holder begin_callback()
        {
            return{ callback_inflight.allocate(), callback_inflight };
        }

        void unpublish_frame(frame* frame);
        frame* publish_frame(frame&& frame);

        rs_frame* clone_frame(rs_frame* frameset);
        void release_frame_ref(rs_frame* ref);

        // Frame callback thread API
        frame alloc_frame(const size_t size, const frame_additional_data& additional_data, bool requires_memory);
        rs_frame* track_frame(frame& f);
        void log_frame_callback_end(frame* frame) const;

        void flush();

        ~frame_archive();
    };
}