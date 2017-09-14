// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS2_FRAME_HPP
#define LIBREALSENSE_RS2_FRAME_HPP

#include "rs_types.hpp"
#include "rs_sensor.hpp"

namespace rs2
{
    class frame
    {
    public:
        frame() : frame_ref(nullptr) {}
        frame(rs2_frame* frame_ref) : frame_ref(frame_ref) {}
        frame(frame&& other) noexcept : frame_ref(other.frame_ref) { other.frame_ref = nullptr; }
        frame& operator=(frame other)
        {
            swap(other);
            return *this;
        }
        frame(const frame& other)
            : frame_ref(other.frame_ref)
        {
            if (frame_ref) add_ref();
        }
        void swap(frame& other)
        {
            std::swap(frame_ref, other.frame_ref);
        }

        /**
        * relases the frame handle
        */
        ~frame()
        {
            if (frame_ref)
            {
                rs2_release_frame(frame_ref);
            }
        }

        operator bool() const { return frame_ref != nullptr; }

        /**
        * retrieve the time at which the frame was captured
        * \return            the timestamp of the frame, in milliseconds since the device was started
        */
        double get_timestamp() const
        {
            rs2_error* e = nullptr;
            auto r = rs2_get_frame_timestamp(frame_ref, &e);
            error::handle(e);
            return r;
        }

        /** retrieve the timestamp domain
        * \return            timestamp domain (clock name) for timestamp values
        */
        rs2_timestamp_domain get_frame_timestamp_domain() const
        {
            rs2_error* e = nullptr;
            auto r = rs2_get_frame_timestamp_domain(frame_ref, &e);
            error::handle(e);
            return r;
        }

        /** retrieve the current value of a single frame_metadata
        * \param[in] frame_metadata  the frame_metadata whose value should be retrieved
        * \return            the value of the frame_metadata
        */
        rs2_metadata_type get_frame_metadata(rs2_frame_metadata_value frame_metadata) const
        {
            rs2_error* e = nullptr;
            auto r = rs2_get_frame_metadata(frame_ref, frame_metadata, &e);
            error::handle(e);
            return r;
        }

        /** determine if the device allows a specific metadata to be queried
        * \param[in] frame_metadata  the frame_metadata to check for support
        * \return            true if the frame_metadata can be queried
        */
        bool supports_frame_metadata(rs2_frame_metadata_value frame_metadata) const
        {
            rs2_error* e = nullptr;
            auto r = rs2_supports_frame_metadata(frame_ref, frame_metadata, &e);
            error::handle(e);
            return r != 0;
        }

        /**
        * retrieve frame number (from frame handle)
        * \return               the frame nubmer of the frame, in milliseconds since the device was started
        */
        unsigned long long get_frame_number() const
        {
            rs2_error* e = nullptr;
            auto r = rs2_get_frame_number(frame_ref, &e);
            error::handle(e);
            return r;
        }

        /**
        * retrieve data from frame handle
        * \return               the pointer to the start of the frame data
        */
        const void* get_data() const
        {
            rs2_error* e = nullptr;
            auto r = rs2_get_frame_data(frame_ref, &e);
            error::handle(e);
            return r;
        }

        stream_profile get_profile() const
        {
            rs2_error* e = nullptr;
            auto s = rs2_get_frame_stream_profile(frame_ref, &e);
            error::handle(e);
            return stream_profile(s);
        }

        template<class T>
        bool is() const
        {
            T extension(*this);
            return extension;
        }

        template<class T>
        T as() const
        {
            T extension(*this);
            return extension;
        }

        rs2_frame* get() const { return frame_ref; }

    protected:
        /**
        * create additional reference to a frame without duplicating frame data
        * \param[out] result     new frame reference, release by destructor
        * \return                true if cloning was successful
        */
        void add_ref() const
        {
            rs2_error* e = nullptr;
            rs2_frame_add_ref(frame_ref, &e);
            error::handle(e);
        }

        void reset()
        {
            if (frame_ref)
            {
                rs2_release_frame(frame_ref);
            }
            frame_ref = nullptr;
        }

        friend class frame_queue;
        friend class syncer;
        friend class frame_source;
        friend class processing_block;
        friend class pointcloud_block;

    private:
        rs2_frame* frame_ref;
    };

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
    class frameset : public frame
    {
    public:
        frameset():_size(0) {};
        frameset(const frame& f)
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
       

        depth_frame get_depth_frame() const
        {
            auto f = first_or_default(RS2_STREAM_DEPTH);
            return f.as<depth_frame>();
        }

        video_frame get_color_frame()
        {
            auto f = first_or_default(RS2_STREAM_COLOR);

            if (!f)
            {
                auto ir = first_or_default(RS2_STREAM_INFRARED);
                if (ir && ir.get_profile().format() == RS2_FORMAT_RGB8)
                    f = ir;
            }
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
                auto fref = rs2_extract_frame(get(), (int)i, &e);
                error::handle(e);

                action(frame(fref));
            }
        }

        frame operator[](size_t index) const
        {
            rs2_error* e = nullptr;
            if(index < size())
            {
                 auto fref = rs2_extract_frame(get(), (int)index, &e);
                 error::handle(e);
                 return frame(fref);
            }

            throw error("Requested index is out of range!");
        }

        class iterator
        {
        public:
            iterator(frameset* owner, size_t index = 0) : _owner(owner), _index(index) {}
            iterator& operator++() { ++_index; return *this; }
            bool operator==(const iterator& other) const { return _index == other._index; }
            bool operator!=(const iterator& other) const { return !(*this == other); }

            frame operator*() { return (*_owner)[_index]; }
        private:
            size_t _index = 0;
            frameset* _owner;
        };

        iterator begin() { return iterator(this); }
        iterator end() { return iterator(this, size()); }
    private:
        size_t _size;
    };

    class frame_source
    {
    public:

        frame allocate_video_frame(const stream_profile& profile,
                                   const frame& original,
                                   int new_bpp = 0,
                                   int new_width = 0,
                                   int new_height = 0,
                                   int new_stride = 0,
                                   rs2_extension frame_type = RS2_EXTENSION_VIDEO_FRAME) const
        {
            rs2_error* e = nullptr;
            auto result = rs2_allocate_synthetic_video_frame(_source, profile.get(),
                original.get(), new_bpp, new_width, new_height, new_stride, frame_type, &e);
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
