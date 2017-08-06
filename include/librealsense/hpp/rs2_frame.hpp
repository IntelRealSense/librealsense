// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS2_FRAME_HPP
#define LIBREALSENSE_RS2_FRAME_HPP

#include "rs2_types.hpp"
#include "rs2_streaming.hpp"

namespace rs2
{
    class video_frame : public frame
    {
    public:
        video_frame(const frame& f)
            : frame(f)
        {
            rs2_error* e = nullptr;
            if(!f || (rs2_is_frame_extendable_to(f.get(), RS2_EXTENSION_VIDEO_FRAME, &e) == 0 && !e))
            {
                reset();
            }
            error::handle(e);
        }

        /**
        * returns image width in pixels
        * \return        frame width in pixels
        */
        int get_width() const
        {
            rs2_error* e = nullptr;
            auto r = rs2_get_frame_width(get(), &e);
            error::handle(e);
            return r;
        }

        /**
        * returns image height in pixels
        * \return        frame height in pixels
        */
        int get_height() const
        {
            rs2_error* e = nullptr;
            auto r = rs2_get_frame_height(get(), &e);
            error::handle(e);
            return r;
        }

        /**
        * retrieve frame stride, meaning the actual line width in memory in bytes (not the logical image width)
        * \return            stride in bytes
        */
        int get_stride_in_bytes() const
        {
            rs2_error* e = nullptr;
            auto r = rs2_get_frame_stride_in_bytes(get(), &e);
            error::handle(e);
            return r;
        }

        /**
        * retrieve bits per pixel
        * \return            number of bits per one pixel
        */
        int get_bits_per_pixel() const
        {
            rs2_error* e = nullptr;
            auto r = rs2_get_frame_bits_per_pixel(get(), &e);
            error::handle(e);
            return r;
        }

        int get_bytes_per_pixel() const { return get_bits_per_pixel() / 8; }
    };

    struct vertex {
        float x, y, z;
        operator const float*() const { return &x; }
    };
    struct texture_coordinate {
        float u, v;
        operator const float*() const { return &u; }
    };

    class points : public frame
    {
    public:
        points() : frame(), _size(0) {}

        points(const frame& f)
                : frame(f), _size(0)
        {
            rs2_error* e = nullptr;
            if(!f || (rs2_is_frame_extendable_to(f.get(), RS2_EXTENSION_POINTS, &e) == 0 && !e))
            {
                reset();
            }
            error::handle(e);

            if (get())
            {
                rs2_error* e = nullptr;
                _size = rs2_get_frame_points_count(get(), &e);
                error::handle(e);
            }
        }

        const vertex* get_vertices() const
        {
            rs2_error* e = nullptr;
            auto res = rs2_get_frame_vertices(get(), &e);
            error::handle(e);
            return (const vertex*)res;
        }

        const texture_coordinate* get_texture_coordinates() const
        {
            rs2_error* e = nullptr;
            auto res = rs2_get_frame_texture_coordinates(get(), &e);
            error::handle(e);
            return (const texture_coordinate*)res;
        }

        size_t size() const
        {
            return _size;
        }

    private:
        size_t _size;
    };

    class composite_frame : public frame
    {
    public:
        composite_frame(const frame& f)
            : frame(f), _size(0)
        {
            rs2_error* e = nullptr;
            if(!f || (rs2_is_frame_extendable_to(f.get(), RS2_EXTENSION_COMPOSITE_FRAME, &e) == 0 && !e))
            {
                reset();
            }
            error::handle(e);

            if (get())
            {
                rs2_error* e = nullptr;
                _size = rs2_embedded_frames_count(get(), &e);
                error::handle(e);
            }
        }

        frame first_or_default(rs2_stream s) const
        {
            frame result;
            foreach([&result, s](frame f){
                if (!result && f.get_profile().stream_type() == s)
                {
                    result = std::move(f);
                }
            });
            return result;
        }

        frame first(rs2_stream s) const
        {
            auto f = first_or_default(s);
            if (!f) throw error("Frame of requested stream type was not found!");
            return f;
        }

        size_t size() const
        {
            return _size;
        }

        template<class T>
        void foreach(T action) const
        {
            rs2_error* e = nullptr;
            auto count = size();
            for (size_t i = 0; i < count; i++)
            {
                auto fref = rs2_extract_frame(get(), i, &e);
                error::handle(e);

                action(frame(fref));
            }
        }

        frameset get_frames() const
        {
            frameset res;
            res.reserve(size());

            foreach([&res](frame f){
                res.emplace_back(std::move(f));
            });

            return std::move(res);
        }

    private:
        size_t _size;
    };

    template<class T>
    class frame_callback : public rs2_frame_callback
    {
        T on_frame_function;
    public:
        explicit frame_callback(T on_frame) : on_frame_function(on_frame) {}

        void on_frame(rs2_frame* fref) override
        {
            on_frame_function(frame{ fref });
        }

        void release() override { delete this; }
    };

    class frame_source
    {
    public:

        frame allocate_video_frame(const stream_profile& profile,
                                   const frame& original,
                                   rs2_format new_format = RS2_FORMAT_ANY,
                                   int new_bpp = 0,
                                   int new_width = 0,
                                   int new_height = 0,
                                   int new_stride = 0) const
        {
            rs2_error* e = nullptr;
            auto result = rs2_allocate_synthetic_video_frame(_source, profile.get(),
                original.get(), new_bpp, new_width, new_height, new_stride, &e);
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
}
#endif // LIBREALSENSE_RS2_FRAME_HPP
