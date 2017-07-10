// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"
#include <atomic>
#include <array>
#include <math.h>

namespace librealsense
{
    class archive_interface;
    class md_attribute_parser_base;
    class frame;
}

struct frame_additional_data
{
    rs2_time_t timestamp = 0;
    unsigned long long frame_number = 0;
    unsigned int    fps = 0;
    rs2_format      format = RS2_FORMAT_ANY;
    rs2_stream      stream_type = RS2_STREAM_COUNT;
    rs2_timestamp_domain timestamp_domain = RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK;
    rs2_time_t      system_time = 0;
    rs2_time_t      frame_callback_started = 0;
    uint32_t        metadata_size = 0;
    bool            fisheye_ae_mode = false;
    std::array<uint8_t,MAX_META_DATA_SIZE> metadata_blob;

    frame_additional_data() {};

    frame_additional_data(double in_timestamp, unsigned long long in_frame_number, double in_system_time,
        const rs2_format in_format, rs2_stream in_stream_type, unsigned int fps,
        uint8_t md_size, const uint8_t* md_buf)
        : timestamp(in_timestamp),
          frame_number(in_frame_number),
          system_time(in_system_time),
          fps(fps),
          format(in_format),
          stream_type(in_stream_type),
          metadata_size(md_size)
    {
        // Copy up to 255 bytes to preserve metadata as raw data
        if (metadata_size)
            std::copy(md_buf,md_buf+ std::min(md_size,MAX_META_DATA_SIZE),metadata_blob.begin());
    }
};

struct rs2_frame // esentially an intrusive shared_ptr<frame>
{
    rs2_frame();
    explicit rs2_frame(librealsense::frame* frame);
    rs2_frame(const rs2_frame& other);
    rs2_frame(rs2_frame&& other);

    rs2_frame& operator=(rs2_frame other);
    ~rs2_frame();

    void swap(rs2_frame& other);

    void attach_continuation(librealsense::frame_continuation&& continuation) const;
    void disable_continuation() const;

    librealsense::frame* get() const;

    void log_callback_start(rs2_time_t timestamp) const;
    void log_callback_end(rs2_time_t timestamp) const;

private:
    librealsense::frame* frame_ptr = nullptr;
};

namespace librealsense
{
    typedef std::map<rs2_frame_metadata, std::shared_ptr<md_attribute_parser_base>> metadata_parser_map;

    // Define a movable but explicitly noncopyable buffer type to hold our frame data
    class frame
    {
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

        virtual ~frame() { on_release.reset(); }

        rs2_metadata_t get_frame_metadata(const rs2_frame_metadata& frame_metadata) const;
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
        rs2_format get_format() const;
        rs2_stream get_stream_type() const;
        int get_framerate() const;

        rs2_time_t get_frame_callback_start_time_point() const;
        void update_frame_callback_start_ts(rs2_time_t ts);

        void acquire() { ref_count.fetch_add(1); }
        void release();
        frame* publish(std::shared_ptr<librealsense::archive_interface> new_owner);
        void attach_continuation(librealsense::frame_continuation&& continuation) { on_release = std::move(continuation); }
        void disable_continuation() { on_release.reset(); }

        librealsense::archive_interface* get_owner() const { return owner.get(); }

    private:
        // TODO: check boost::intrusive_ptr or an alternative
        std::atomic<int> ref_count; // the reference count is on how many times this placeholder has been observed (not lifetime, not content)
        std::shared_ptr<librealsense::archive_interface> owner; // pointer to the owner to be returned to by last observe
        librealsense::frame_continuation on_release;
    };

    class video_frame : public frame
    {
    public:
        video_frame()
            : frame(), _width(0), _height(0), _bpp(0), _stride(0) 
        {}

        int get_width() const { return _width; }
        int get_height() const { return _height; }
        int get_stride() const { return _stride; }
        int get_bpp() const { return _bpp; }

        void assign(int width, int height, int stride, int bpp)
        {
            _width = width;
            _height = height;
            _stride = stride;
            _bpp = bpp;
        }

    private:
        int _width, _height, _bpp, _stride;
    };

    //TODO: Define Motion Frame

    class archive_interface
    {
    public:
        virtual callback_invocation_holder begin_callback() = 0;
         
        virtual rs2_frame* clone_frame(rs2_frame* frameset) = 0;
        virtual void release_frame_ref(rs2_frame* ref) = 0;
        
        virtual rs2_frame* alloc_and_track(const size_t size, const frame_additional_data& additional_data, bool requires_memory) = 0;

        virtual std::shared_ptr<metadata_parser_map> get_md_parsers() const = 0;
        
        virtual void flush() = 0;

        virtual frame* publish_frame(frame* frame) = 0;
        virtual void unpublish_frame(frame* frame) = 0;
        
        virtual ~archive_interface() = default;
    };

    std::shared_ptr<archive_interface> make_archive(rs2_extension_type type, 
                                                    std::atomic<uint32_t>* in_max_frame_queue_size,
                                                    std::shared_ptr<uvc::time_service> ts,
                                                    std::shared_ptr<metadata_parser_map> parsers);
}
