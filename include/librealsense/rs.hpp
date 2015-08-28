#ifndef LIBREALSENSE_CPP_INCLUDE_GUARD
#define LIBREALSENSE_CPP_INCLUDE_GUARD

#include "rs.h"

#include <string>
#include <sstream>
#include <stdexcept>

namespace rs
{
    // Modify this function if you wish to change error handling behavior from throwing exceptions to logging / callbacks / global status, etc.
    inline void handle_error(rs_error * error)
    {
        std::ostringstream ss;
        ss << rs_get_error_message(error) << " (" << rs_get_failed_function(error) << '(' << rs_get_failed_args(error) << "))";
        rs_free_error(error);
        throw std::runtime_error(ss.str());
        // std::cerr << ss.str() << std::endl;
    }

    class auto_error
    {
        rs_error *          error;
    public:
                            auto_error()                                                            : error() {}
                            auto_error(const auto_error &)                                          = delete;
                            ~auto_error() noexcept(false)                                           { if (error) handle_error(error); }
        auto_error &        operator = (const auto_error &)                                         = delete;
                            operator rs_error ** ()                                                 { return &error; }
    };

    typedef rs_stream       stream;
    typedef rs_format       format;
    typedef rs_preset       preset;
    typedef rs_distortion   distortion;
    typedef rs_option       option;
    typedef rs_intrinsics   intrinsics;
    typedef rs_extrinsics   extrinsics;

    inline const char *     get_name(stream s)                                                      { return rs_get_stream_name(s, auto_error()); }
    inline const char *     get_name(format f)                                                      { return rs_get_format_name(f, auto_error()); }
    inline const char *     get_name(preset p)                                                      { return rs_get_preset_name(p, auto_error()); }
    inline const char *     get_name(distortion d)                                                  { return rs_get_distortion_name(d, auto_error()); }
    inline const char *     get_name(option d)                                                      { return rs_get_option_name(d, auto_error()); }

    class camera
    {
        rs_camera *         cam;
    public:
                            camera()                                                                : cam() {}
                            camera(rs_camera * cam)                                                 : cam(cam) {}
        explicit            operator bool() const                                                   { return !!cam; }
        rs_camera *         get_handle() const                                                      { return cam; }

        const char *        get_name()                                                              { return rs_get_camera_name(cam, auto_error()); }

        void                enable_stream(stream s, int width, int height, format f, int fps)       { rs_enable_stream(cam, s, width, height, f, fps, auto_error()); }
        void                enable_stream_preset(stream s, preset preset)                           { rs_enable_stream_preset(cam, s, preset, auto_error()); }
        bool                is_stream_enabled(stream s)                                             { return !!rs_is_stream_enabled(cam, s, auto_error()); }

        void                start_capture()                                                         { rs_start_capture(cam, auto_error()); }
        void                stop_capture()                                                          { rs_stop_capture(cam, auto_error()); }
        bool                is_capturing()                                                          { return !!rs_is_capturing(cam, auto_error()); }

        float               get_depth_scale()                                                       { return rs_get_depth_scale(cam, auto_error()); }
        format              get_stream_format(stream s)                                             { return rs_get_stream_format(cam, s, auto_error()); }
        intrinsics          get_stream_intrinsics(stream s)                                         { intrinsics intrin; rs_get_stream_intrinsics(cam, s, &intrin, auto_error()); return intrin; }
        extrinsics          get_stream_extrinsics(stream from, stream to)                           { extrinsics extrin; rs_get_stream_extrinsics(cam, from, to, &extrin, auto_error()); return extrin; }

        void                wait_all_streams()                                                      { rs_wait_all_streams(cam, auto_error()); }
        const void *        get_image_pixels(stream s)                                              { return rs_get_image_pixels(cam, s, auto_error()); }
        int                 get_image_frame_number(stream s)                                        { return rs_get_image_frame_number(cam, s, auto_error()); }

        bool                supports_option(option o) const                                         { return rs_camera_supports_option(cam, o, auto_error()); }
        void                set_option(option o, int value)                                         { rs_set_camera_option(cam, o, value, auto_error()); }
        int                 get_option(option o)                                                    { return rs_get_camera_option(cam, o, auto_error()); }
    };

    class context
    {
        rs_context *        ctx;
    public:
                            context()                                                               : ctx(rs_create_context(RS_API_VERSION, auto_error())) {}
                            context(const auto_error &)                                             = delete;
                            ~context()                                                              { rs_delete_context(ctx, nullptr); } // Deliberately ignore error on destruction
                            context & operator = (const context &)                                  = delete;
        rs_context *        get_handle() const                                                      { return ctx; }

        int                 get_camera_count()                                                      { return rs_get_camera_count(ctx, auto_error()); }
        camera              get_camera(int index)                                                   { return rs_get_camera(ctx, index, auto_error()); }
    };
}

inline std::ostream &       operator << (std::ostream & out, rs::stream stream)                     { return out << rs::get_name(stream); }
inline std::ostream &       operator << (std::ostream & out, rs::format format)                     { return out << rs::get_name(format); }
inline std::ostream &       operator << (std::ostream & out, rs::preset preset)                     { return out << rs::get_name(preset); }
inline std::ostream &       operator << (std::ostream & out, rs::distortion distortion)             { return out << rs::get_name(distortion); }
inline std::ostream &       operator << (std::ostream & out, rs::option option)                     { return out << rs::get_name(option); }

#endif
