// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS2_FRAME_HPP
#define LIBREALSENSE_RS2_FRAME_HPP

#include "rs_types.hpp"

namespace rs2
{
    class frame_source;
    class frame_queue;
    class syncer;
    class processing_block;
    class pointcloud;
    class sensor;
    class frame;
    class pipeline_profile;
    class points;

    class stream_profile
    {
    public:
        stream_profile() : _profile(nullptr) {}

        int stream_index() const { return _index; }
        rs2_stream stream_type() const { return _type; }
        rs2_format format() const { return _format; }

        int fps() const { return _framerate; }

        int unique_id() const { return _uid; }

        stream_profile clone(rs2_stream type, int index, rs2_format format) const
        {
            rs2_error* e = nullptr;
            auto ref = rs2_clone_stream_profile(_profile, type, index, format, &e);
            error::handle(e);
            stream_profile res(ref);
            res._clone = std::shared_ptr<rs2_stream_profile>(ref, [](rs2_stream_profile* r) { rs2_delete_stream_profile(r); });

            return res;
        }

        bool operator==(const stream_profile& rhs)
        {
            return  stream_index() == rhs.stream_index()&&
                    stream_type() == rhs.stream_type()&&
                    format() == rhs.format()&&
                    fps() == rhs.fps();
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

        std::string stream_name() const
        {
            std::stringstream ss;
            ss << rs2_stream_to_string(stream_type());
            if (stream_index() != 0) ss << " " << stream_index();
            return ss.str();
        }

        bool is_default() const { return _default; }

        operator bool() const { return _profile != nullptr; }

        const rs2_stream_profile* get() const { return _profile; }

        operator const rs2_stream_profile*()
        {
            return _profile;
        }
        rs2_extrinsics get_extrinsics_to(const stream_profile& to) const
        {
            rs2_error* e = nullptr;
            rs2_extrinsics res;
            rs2_get_extrinsics(get(), to.get(), &res, &e);
            error::handle(e);
            return res;
        }
        void register_extrinsics_to(const stream_profile& to, rs2_extrinsics extrinsics)
        {
            rs2_error* e = nullptr;
            rs2_register_extrinsics(get(), to.get(), extrinsics, &e);
            error::handle(e);
        }

    protected:
        friend class rs2::sensor;
        friend class rs2::frame;
        friend class rs2::pipeline_profile;
        friend class software_sensor;

        explicit stream_profile(const rs2_stream_profile* profile) : _profile(profile)
        {
            rs2_error* e = nullptr;
            rs2_get_stream_profile_data(_profile, &_type, &_format, &_index, &_uid, &_framerate, &e);
            error::handle(e);

            _default = !!(rs2_is_stream_profile_default(_profile, &e));
            error::handle(e);

        }

        const rs2_stream_profile* _profile;
        std::shared_ptr<rs2_stream_profile> _clone;

        int _index = 0;
        int _uid = 0;
        int _framerate = 0;
        rs2_format _format = RS2_FORMAT_ANY;
        rs2_stream _type = RS2_STREAM_ANY;

        bool _default = false;
    };

    class video_stream_profile : public stream_profile
    {
    public:
        explicit video_stream_profile(const stream_profile& sp)
            : stream_profile(sp)
        {
            rs2_error* e = nullptr;
            if ((rs2_stream_profile_is(sp.get(), RS2_EXTENSION_VIDEO_PROFILE, &e) == 0 && !e))
            {
                _profile = nullptr;
            }
            error::handle(e);

            if (_profile)
            {
                rs2_get_video_stream_resolution(_profile, &_width, &_height, &e);
                error::handle(e);
            }
        }

        int width() const
        {
            return _width;
        }

        int height() const
        {
            return _height;
        }

        rs2_intrinsics get_intrinsics() const
        {
            rs2_error* e = nullptr;
            rs2_intrinsics intr;
            rs2_get_video_stream_intrinsics(_profile, &intr, &e);
            error::handle(e);
            return intr;
        }

    private:
        int _width = 0;
        int _height = 0;
    };


    class motion_stream_profile : public stream_profile
    {
    public:
        explicit motion_stream_profile(const stream_profile& sp)
            : stream_profile(sp)
        {
            rs2_error* e = nullptr;
            if ((rs2_stream_profile_is(sp.get(), RS2_EXTENSION_MOTION_PROFILE, &e) == 0 && !e))
            {
                _profile = nullptr;
            }
            error::handle(e);
        }

        /**
        * returns scale and bias of a the motion stream profile
        */
        rs2_motion_device_intrinsic get_motion_intrinsics() const

        {
            rs2_error* e = nullptr;
            rs2_motion_device_intrinsic intrin;
            rs2_get_motion_intrinsics(_profile, &intrin, &e);
            error::handle(e);
            return intrin;
        }
    };

    class frame
    {
    public:
        frame() : frame_ref(nullptr) {}
        frame(rs2_frame* frame_ref) : frame_ref(frame_ref)
        {
#ifdef _DEBUG
            if (frame_ref)
            {
                rs2_error* e = nullptr;
                auto r = rs2_get_frame_number(frame_ref, &e);
                if (!e)
                    frame_number = r;
                auto s = rs2_get_frame_stream_profile(frame_ref, &e);
                if (!e)
                    profile = stream_profile(s);
            }
            else
            {
                frame_number = 0;
                profile = stream_profile();
            }
#endif
        }

        frame(frame&& other) noexcept : frame_ref(other.frame_ref)
        {
            other.frame_ref = nullptr;
#ifdef _DEBUG
            frame_number = other.frame_number;
            profile = other.profile;
#endif
        }
        frame& operator=(frame other)
        {
            swap(other);
            return *this;
        }
        frame(const frame& other)
            : frame_ref(other.frame_ref)
        {
            if (frame_ref) add_ref();
#ifdef _DEBUG
            frame_number = other.frame_number;
            profile =  other.profile;
#endif
        }
        void swap(frame& other)
        {
            std::swap(frame_ref, other.frame_ref);

#ifdef _DEBUG
            std::swap(frame_number, other.frame_number);
            std::swap(profile, other.profile);
#endif
        }

        /**
        * releases the frame handle
        */
        ~frame()
        {
            if (frame_ref)
            {
                rs2_release_frame(frame_ref);
            }
        }

        void keep() { rs2_keep_frame(frame_ref); }

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
        * \return               the frame number of the frame, in milliseconds since the device was started
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

    private:
        friend class rs2::frame_source;
        friend class rs2::frame_queue;
        friend class rs2::syncer;
        friend class rs2::processing_block;
        friend class rs2::pointcloud;
        friend class rs2::points;

        rs2_frame* frame_ref;

#ifdef _DEBUG
        stream_profile profile;
        unsigned long long frame_number = 0;
#endif
    };

    class video_frame : public frame
    {
    public:
        video_frame(const frame& f)
            : frame(f)
        {
            rs2_error* e = nullptr;
            if (!f || (rs2_is_frame_extendable_to(f.get(), RS2_EXTENSION_VIDEO_FRAME, &e) == 0 && !e))
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
            if (!f || (rs2_is_frame_extendable_to(f.get(), RS2_EXTENSION_POINTS, &e) == 0 && !e))
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

        void export_to_ply(const std::string& fname, video_frame texture)
        {
            rs2_frame* ptr = nullptr;
            std::swap(texture.frame_ref, ptr);
            rs2_error* e = nullptr;
            rs2_export_to_ply(get(), fname.c_str(), ptr, &e);
            error::handle(e);
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

    class disparity_frame : public depth_frame
    {
    public:
        disparity_frame(const frame& f)
            : depth_frame(f)
        {
            rs2_error* e = nullptr;
            if (!f || (rs2_is_frame_extendable_to(f.get(), RS2_EXTENSION_DISPARITY_FRAME, &e) == 0 && !e))
            {
                reset();
            }
            error::handle(e);
        }

        float get_baseline(void) const
        {
            rs2_error * e = nullptr;
            auto r = rs2_depth_stereo_frame_get_baseline(get(), &e);
            error::handle(e);
            return r;
        }
    };

    class motion_frame : public frame
    {
    public:
        motion_frame(const frame& f)
            : frame(f)
        {
            rs2_error* e = nullptr;
            if (!f || (rs2_is_frame_extendable_to(f.get(), RS2_EXTENSION_MOTION_FRAME, &e) == 0 && !e))
            {
                reset();
            }
            error::handle(e);
        }

        rs2_vector get_motion_data()
        {
            auto data = reinterpret_cast<const float*>(get_data());
            return rs2_vector{data[0], data[1], data[2]};
        }
    };

    class pose_frame : public frame
    {
    public:
        pose_frame(const frame& f)
            : frame(f)
        {
            rs2_error* e = nullptr;
            if (!f || (rs2_is_frame_extendable_to(f.get(), RS2_EXTENSION_POSE_FRAME, &e) == 0 && !e))
            {
                reset();
            }
            error::handle(e);
        }

        rs2_pose get_pose_data()
        {
            rs2_pose pose_data;
            rs2_error* e = nullptr;
            rs2_pose_frame_get_pose_data(get(), &pose_data, &e);
            error::handle(e);
            return pose_data;
        }
    };

    class frameset : public frame
    {
    public:
        frameset() :_size(0) {};
        frameset(const frame& f)
            : frame(f), _size(0)
        {
            rs2_error* e = nullptr;
            if (!f || (rs2_is_frame_extendable_to(f.get(), RS2_EXTENSION_COMPOSITE_FRAME, &e) == 0 && !e))
            {
                reset();
                // TODO - consider explicit constructor to move resultion to compile time
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
            foreach([&result, s](frame f) {
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

        video_frame get_color_frame() const
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

        video_frame get_infrared_frame(const size_t index = 0) const
        {
            frame f;
            if (!index)
            {
                f = first_or_default(RS2_STREAM_INFRARED);
            }
            else
            {
                foreach([&f, index](const frame& frame) {
                    if (frame.get_profile().stream_type() == RS2_STREAM_INFRARED && frame.get_profile().stream_index() == index)
                        f = frame;
                });
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
            if (index < size())
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
            iterator(const frameset* owner, size_t index = 0) : _index(index), _owner(owner) {}
            iterator& operator++() { ++_index; return *this; }
            bool operator==(const iterator& other) const { return _index == other._index; }
            bool operator!=(const iterator& other) const { return !(*this == other); }

            frame operator*() { return (*_owner)[_index]; }
        private:
            size_t _index = 0;
            const frameset* _owner;
        };

        iterator begin() const { return iterator(this); }
        iterator end() const { return iterator(this, size()); }
    private:
        size_t _size;
    };


}
#endif // LIBREALSENSE_RS2_FRAME_HPP
