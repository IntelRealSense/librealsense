// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"
#include <atomic>

namespace rsimpl2
{
    class frame_archive;
}

struct frame_additional_data
{
    rs2_time_t timestamp = 0;
    unsigned long long frame_number = 0;
    int width = 0;
    int height = 0;
    int fps = 0;
    int stride = 0;
    int bpp = 1;
    rs2_format format = RS2_FORMAT_ANY;
    rs2_stream stream_type = RS2_STREAM_COUNT;
    rs2_timestamp_domain timestamp_domain = RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK;
    rs2_time_t system_time = 0;
    rs2_time_t frame_callback_started = 0;

    frame_additional_data() {};

    frame_additional_data(double in_timestamp, unsigned long long in_frame_number, double in_system_time,
        int in_width, int in_height, int in_fps,
        int in_stride, int in_bpp,
        const rs2_format in_format, rs2_stream in_stream_type)
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
    std::shared_ptr<rsimpl2::frame_archive> owner; // pointer to the owner to be returned to by last observe
    rsimpl2::frame_continuation on_release;

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

    double get_frame_metadata(const rs2_frame_metadata& frame_metadata) const;
    bool supports_frame_metadata(const rs2_frame_metadata& frame_metadata) const;
    const byte* get_frame_data() const;
    rs2_time_t get_frame_timestamp() const;
    rs2_timestamp_domain get_frame_timestamp_domain() const;
    void set_timestamp(double new_ts) { additional_data.timestamp = new_ts; }
    unsigned long long get_frame_number() const;

    void set_timestamp_domain(rs2_timestamp_domain timestamp_domain)
    {
        additional_data.timestamp_domain = timestamp_domain;
    }

    rs2_time_t get_frame_system_time() const;
    int get_width() const;
    int get_height() const;
    int get_framerate() const;
    int get_stride() const;
    int get_bpp() const;
    rs2_format get_format() const;
    rs2_stream get_stream_type() const;

    rs2_time_t get_frame_callback_start_time_point() const;
    void update_frame_callback_start_ts(rs2_time_t ts);

    void acquire() { ref_count.fetch_add(1); }
    void release();
    frame* publish(std::shared_ptr<rsimpl2::frame_archive> new_owner);
    void attach_continuation(rsimpl2::frame_continuation&& continuation) { on_release = std::move(continuation); }
    void disable_continuation() { on_release.reset(); }

    rsimpl2::frame_archive* get_owner() const { return owner.get(); }
};

struct rs2_frame // esentially an intrusive shared_ptr<frame>
{
    rs2_frame() : frame_ptr(nullptr) {}

    explicit rs2_frame(frame* frame)
        : frame_ptr(frame)
    {
        if (frame) frame->acquire();
    }

    rs2_frame(const rs2_frame& other)
        : frame_ptr(other.frame_ptr)
    {
        if (frame_ptr) frame_ptr->acquire();
    }

    rs2_frame(rs2_frame&& other)
        : frame_ptr(other.frame_ptr)
    {
        other.frame_ptr = nullptr;
    }

    rs2_frame& operator=(rs2_frame other)
    {
        swap(other);
        return *this;
    }

    ~rs2_frame()
    {
        if (frame_ptr) frame_ptr->release();
    }

    void swap(rs2_frame& other)
    {
        std::swap(frame_ptr, other.frame_ptr);
    }

    void attach_continuation(rsimpl2::frame_continuation&& continuation) const
    {
        if (frame_ptr) frame_ptr->attach_continuation(std::move(continuation));
    }

    void disable_continuation() const
    {
        if (frame_ptr) frame_ptr->disable_continuation();
    }

    frame* get() const { return frame_ptr; }

    void log_callback_start(rs2_time_t timestamp) const;
    void log_callback_end(rs2_time_t timestamp) const;

private:
    frame * frame_ptr = nullptr;
};

struct callback_invocation
{
    std::chrono::high_resolution_clock::time_point started;
    std::chrono::high_resolution_clock::time_point ended;
};

typedef rsimpl2::small_heap<callback_invocation, 1> callbacks_heap;

struct callback_invocation_holder
{
    callback_invocation_holder() : invocation(nullptr), owner(nullptr) {}
    callback_invocation_holder(const callback_invocation_holder&) = delete;
    callback_invocation_holder& operator=(const callback_invocation_holder&) = delete;

    callback_invocation_holder(callback_invocation_holder&& other)
        : invocation(other.invocation), owner(other.owner)
    {
        other.invocation = nullptr;
    }

    callback_invocation_holder(callback_invocation* invocation, callbacks_heap* owner)
        : invocation(invocation), owner(owner)
    { }

    ~callback_invocation_holder()
    {
        if (invocation) owner->deallocate(invocation);
    }

    callback_invocation_holder& operator=(callback_invocation_holder&& other)
    {
        invocation = other.invocation;
        owner = other.owner;
        other.invocation = nullptr;
        return *this;
    }

private:
    callback_invocation* invocation;
    callbacks_heap* owner;
};

namespace rsimpl2
{
    // Defines general frames storage model
    class frame_archive : public std::enable_shared_from_this<frame_archive>
    {
        std::atomic<uint32_t>* max_frame_queue_size;
        std::atomic<uint32_t> published_frames_count;
        small_heap<frame, RS2_USER_QUEUE_SIZE> published_frames;
        small_heap<rs2_frame, RS2_USER_QUEUE_SIZE> detached_refs;
        callbacks_heap callback_inflight;

        std::vector<frame> freelist; // return frames here
        std::atomic<bool> recycle_frames;
        int pending_frames = 0;
        std::recursive_mutex mutex;
        std::shared_ptr<uvc::time_service> _time_service;
        std::shared_ptr<uvc::uvc_device> device;

    public:
        explicit frame_archive(std::atomic<uint32_t>* max_frame_queue_size,
                               std::shared_ptr<uvc::time_service> ts,
                               std::shared_ptr<uvc::uvc_device> dev);

        callback_invocation_holder begin_callback()
        {
            return{ callback_inflight.allocate(), &callback_inflight };
        }

        void unpublish_frame(frame* frame);
        frame* publish_frame(frame&& frame);

        rs2_frame* clone_frame(rs2_frame* frameset);
        void release_frame_ref(rs2_frame* ref);

        // Frame callback thread API
        frame alloc_frame(const size_t size, const frame_additional_data& additional_data, bool requires_memory);
        rs2_frame* track_frame(frame& f);
        void log_frame_callback_end(frame* frame) const;

        void flush();

        ~frame_archive();
    };
}
