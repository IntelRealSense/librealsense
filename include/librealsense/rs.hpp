#ifndef LIBREALSENSE_CPP_INCLUDE_GUARD
#define LIBREALSENSE_CPP_INCLUDE_GUARD

#include "rs.h"

#include <string>
#include <sstream>
#include <stdexcept>

#ifndef WIN32
#define RS_THROWING_DESTRUCTOR noexcept(false)
#else
#define RS_THROWING_DESTRUCTOR
#endif

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
                            ~auto_error() RS_THROWING_DESTRUCTOR                                    { if (error) handle_error(error); }
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

    inline const char *     to_string(stream s)                                                     { return rs_stream_to_string(s); }
    inline const char *     to_string(format f)                                                     { return rs_format_to_string(f); }
    inline const char *     to_string(preset p)                                                     { return rs_preset_to_string(p); }
    inline const char *     to_string(distortion d)                                                 { return rs_distortion_to_string(d); }
    inline const char *     to_string(option o)                                                     { return rs_option_to_string(o); }

    class device
    {
        rs_device *         dev;
    public:
                            device()                                                                : dev() {}
                            device(rs_device * dev)                                                 : dev(dev) {}
        explicit            operator bool() const                                                   { return !!dev; }
        rs_device *         get_handle() const                                                      { return dev; }

        const char *        get_name()                                                              { return rs_get_device_name(dev, auto_error()); }
        bool                supports_option(option o) const                                         { return !!rs_device_supports_option(dev, o, auto_error()); }
        extrinsics          get_extrinsics(stream from, stream to)                                  { extrinsics extrin; rs_get_device_extrinsics(dev, from, to, &extrin, auto_error()); return extrin; }

        void                enable_stream(stream s, int width, int height, format f, int fps)       { rs_enable_stream(dev, s, width, height, f, fps, auto_error()); }
        void                enable_stream_preset(stream s, preset preset)                           { rs_enable_stream_preset(dev, s, preset, auto_error()); }
        bool                is_stream_enabled(stream s)                                             { return !!rs_stream_is_enabled(dev, s, auto_error()); }

        void                start()                                                                 { rs_start_device(dev, auto_error()); }
        void                stop()                                                                  { rs_stop_device(dev, auto_error()); }
        bool                is_streaming()                                                          { return !!rs_device_is_streaming(dev, auto_error()); }
        intrinsics          get_stream_intrinsics(stream s)                                         { intrinsics intrin; rs_get_stream_intrinsics(dev, s, &intrin, auto_error()); return intrin; }
        format              get_stream_format(stream s)                                             { return rs_get_stream_format(dev, s, auto_error()); }

        void                wait_for_frames(int stream_bits)                                        { rs_wait_for_frames(dev, stream_bits, auto_error()); }
        int                 get_frame_number(stream s)                                              { return rs_get_frame_number(dev, s, auto_error()); }
        const void *        get_frame_data(stream s)                                                { return rs_get_frame_data(dev, s, auto_error()); }

        void                set_option(option o, int value)                                         { rs_set_device_option(dev, o, value, auto_error()); }
        int                 get_option(option o)                                                    { return rs_get_device_option(dev, o, auto_error()); }
        float               get_depth_scale()                                                       { return rs_get_device_depth_scale(dev, auto_error()); }
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

        int                 get_device_count()                                                      { return rs_get_device_count(ctx, auto_error()); }
        device              get_device(int index)                                                   { return rs_get_device(ctx, index, auto_error()); }
    };
}

inline std::ostream &       operator << (std::ostream & out, rs::stream stream)                     { return out << rs::to_string(stream); }
inline std::ostream &       operator << (std::ostream & out, rs::format format)                     { return out << rs::to_string(format); }
inline std::ostream &       operator << (std::ostream & out, rs::preset preset)                     { return out << rs::to_string(preset); }
inline std::ostream &       operator << (std::ostream & out, rs::distortion distortion)             { return out << rs::to_string(distortion); }
inline std::ostream &       operator << (std::ostream & out, rs::option option)                     { return out << rs::to_string(option); }

#endif
