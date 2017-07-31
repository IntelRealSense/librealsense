// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"
#include "core/streaming.h"

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

namespace librealsense
{
    typedef std::map<rs2_frame_metadata, std::shared_ptr<md_attribute_parser_base>> metadata_parser_map;

    // Define a movable but explicitly noncopyable buffer type to hold our frame data
    class frame : public frame_interface
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
        rs2_metadata_t get_frame_metadata(const rs2_frame_metadata& frame_metadata) const override;
        bool supports_frame_metadata(const rs2_frame_metadata& frame_metadata) const override;
        const byte* get_frame_data() const override;
        rs2_time_t get_frame_timestamp() const override;
        rs2_timestamp_domain get_frame_timestamp_domain() const override;
        void set_timestamp(double new_ts) override { additional_data.timestamp = new_ts; }
        unsigned long long get_frame_number() const override;

        void set_timestamp_domain(rs2_timestamp_domain timestamp_domain) override
        {
            additional_data.timestamp_domain = timestamp_domain;
        }

        rs2_time_t get_frame_system_time() const override;
        rs2_format get_format() const override;
        rs2_stream get_stream_type() const override;
        int get_framerate() const override;

        rs2_time_t get_frame_callback_start_time_point() const override;
        void update_frame_callback_start_ts(rs2_time_t ts) override;

        void acquire() override { ref_count.fetch_add(1); }
        void release() override;
        frame_interface* publish(std::shared_ptr<archive_interface> new_owner) override;
        void attach_continuation(frame_continuation&& continuation) override { on_release = std::move(continuation); }
        void disable_continuation() override { on_release.reset(); }

        archive_interface* get_owner() const override { return owner.get(); }

        std::shared_ptr<sensor_interface> get_sensor() const override;
        void set_sensor(std::shared_ptr<sensor_interface> s) override;


        void log_callback_start(rs2_time_t timestamp) override;
        void log_callback_end(rs2_time_t timestamp) const override;

    private:
        // TODO: check boost::intrusive_ptr or an alternative
        std::atomic<int> ref_count; // the reference count is on how many times this placeholder has been observed (not lifetime, not content)
        std::shared_ptr<archive_interface> owner; // pointer to the owner to be returned to by last observe
        std::weak_ptr<sensor_interface> sensor;
        frame_continuation on_release;
    };

    class points : public frame
    {
    public:
        float3* get_vertices();
        size_t get_vertex_count() const;
        int2* get_pixel_coordinates();
    };

    MAP_EXTENSION(RS2_EXTENSION_POINTS, librealsense::points);

    class composite_frame : public frame
    {
    public:
        composite_frame() : frame() {}

        frame_interface* get_frame(int i) const
        {
            auto frames = get_frames();
            return frames[i];
        }

        frame_interface** get_frames() const { return (frame_interface**)data.data(); }

        const frame_interface* first() const
        {
            return get_frame(0);
        }
        frame_interface* first()
        {
            return get_frame(0);
        }

        size_t get_embedded_frames_count() const { return data.size() / sizeof(rs2_frame*); }

        // In the next section we make the composite frame "look and feel" like the first of its children
        rs2_metadata_t get_frame_metadata(const rs2_frame_metadata& frame_metadata) const override
        {
            return first()->get_frame_metadata(frame_metadata);
        }
        bool supports_frame_metadata(const rs2_frame_metadata& frame_metadata) const override
        {
            return first()->supports_frame_metadata(frame_metadata);
        }
        const byte* get_frame_data() const override
        {
            return first()->get_frame_data();
        }
        rs2_time_t get_frame_timestamp() const override
        {
            return first()->get_frame_timestamp();
        }
        rs2_timestamp_domain get_frame_timestamp_domain() const override
        {
            return first()->get_frame_timestamp_domain();
        }
        unsigned long long get_frame_number() const override
        {
            if (first())
                return first()->get_frame_number();
            else
                return frame::get_frame_number();
        }
        rs2_time_t get_frame_system_time() const override
        {
            return first()->get_frame_system_time();
        }
        rs2_format get_format() const override
        {
            return first()->get_format();
        }
        rs2_stream get_stream_type() const override
        {
            if (first())
                return first()->get_stream_type();
            else
                return frame::get_stream_type();
        }
        int get_framerate() const override
        {
            auto frames = get_frames();

            auto max_fps = first()->get_framerate();
            for(auto i = 1; i < get_embedded_frames_count(); i++)
            {
                max_fps = std::max(max_fps, frames[i]->get_framerate());
            }
            return max_fps;
        }
        std::shared_ptr<sensor_interface> get_sensor() const override
        {
            return first()->get_sensor();
        }
    };
    
    MAP_EXTENSION(RS2_EXTENSION_COMPOSITE_FRAME, librealsense::composite_frame);

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
    
    MAP_EXTENSION(RS2_EXTENSION_VIDEO_FRAME, librealsense::video_frame);

    //TODO: Define Motion Frame

    class archive_interface : public sensor_part
    {
    public:
        virtual callback_invocation_holder begin_callback() = 0;

        virtual frame_interface* alloc_and_track(const size_t size, const frame_additional_data& additional_data, bool requires_memory) = 0;

        virtual std::shared_ptr<metadata_parser_map> get_md_parsers() const = 0;

        virtual void flush() = 0;

        virtual frame_interface* publish_frame(frame_interface* frame) = 0;
        virtual void unpublish_frame(frame_interface* frame) = 0;

        virtual ~archive_interface() = default;

    };

    std::shared_ptr<archive_interface> make_archive(rs2_extension type,
                                                    std::atomic<uint32_t>* in_max_frame_queue_size,
                                                    std::shared_ptr<platform::time_service> ts,
                                                    std::shared_ptr<metadata_parser_map> parsers);
}
