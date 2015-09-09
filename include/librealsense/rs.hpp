#ifndef LIBREALSENSE_CPP_INCLUDE_GUARD
#define LIBREALSENSE_CPP_INCLUDE_GUARD

#include "rs.h"
#include "rsutil.h"

#include <cstdint>
#include <string>
#include <sstream>
#include <stdexcept>

namespace rs
{
    enum class stream : int32_t
    {
        DEPTH                           = RS_STREAM_DEPTH,
        COLOR                           = RS_STREAM_COLOR,
        INFRARED                        = RS_STREAM_INFRARED,
        INFRARED2                       = RS_STREAM_INFRARED2
    };

    enum class format : int32_t
    {
        ANY                             = RS_FORMAT_ANY,
        Z16                             = RS_FORMAT_Z16,
        YUYV                            = RS_FORMAT_YUYV,
        RGB8                            = RS_FORMAT_RGB8,
        BGR8                            = RS_FORMAT_BGR8,
        RGBA8                           = RS_FORMAT_RGBA8,
        BGRA8                           = RS_FORMAT_BGRA8,
        Y8                              = RS_FORMAT_Y8,
        Y16                             = RS_FORMAT_Y16
    };

    enum class preset : int32_t
    {
        BEST_QUALITY                    = RS_PRESET_BEST_QUALITY,
        LARGEST_IMAGE                   = RS_PRESET_LARGEST_IMAGE,
        HIGHEST_FRAMERATE               = RS_PRESET_HIGHEST_FRAMERATE
    };

    enum class distortion : int32_t
    {
        NONE                            = RS_DISTORTION_NONE,
        MODIFIED_BROWN_CONRADY          = RS_DISTORTION_MODIFIED_BROWN_CONRADY,
        INVERSE_BROWN_CONRADY           = RS_DISTORTION_INVERSE_BROWN_CONRADY
    };

    enum class option : int32_t
    {
        F200_LASER_POWER                = RS_OPTION_F200_LASER_POWER,
        F200_ACCURACY                   = RS_OPTION_F200_ACCURACY,
        F200_MOTION_RANGE               = RS_OPTION_F200_MOTION_RANGE,
        F200_FILTER_OPTION              = RS_OPTION_F200_FILTER_OPTION,
        F200_CONFIDENCE_THRESHOLD       = RS_OPTION_F200_CONFIDENCE_THRESHOLD,
        F200_DYNAMIC_FPS                = RS_OPTION_F200_DYNAMIC_FPS,
        R200_LR_AUTO_EXPOSURE_ENABLED   = RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED,
        R200_LR_GAIN                    = RS_OPTION_R200_LR_GAIN,
        R200_LR_EXPOSURE                = RS_OPTION_R200_LR_EXPOSURE,
        R200_EMITTER_ENABLED            = RS_OPTION_R200_EMITTER_ENABLED,
        R200_DEPTH_CONTROL_PRESET       = RS_OPTION_R200_DEPTH_CONTROL_PRESET,
        R200_DEPTH_UNITS                = RS_OPTION_R200_DEPTH_UNITS,
        R200_DEPTH_CLAMP_MIN            = RS_OPTION_R200_DEPTH_CLAMP_MIN,
        R200_DEPTH_CLAMP_MAX            = RS_OPTION_R200_DEPTH_CLAMP_MAX,
        R200_DISPARITY_MODE_ENABLED     = RS_OPTION_R200_DISPARITY_MODE_ENABLED,
        R200_DISPARITY_MULTIPLIER       = RS_OPTION_R200_DISPARITY_MULTIPLIER,
        R200_DISPARITY_SHIFT            = RS_OPTION_R200_DISPARITY_SHIFT         
    };

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
                            #ifdef WIN32
                            ~auto_error()                                                           { if (error) handle_error(error); }
                            #else
                            ~auto_error() noexcept(false)                                           { if (error) handle_error(error); }
                            #endif
        auto_error &        operator = (const auto_error &)                                         = delete;
                            operator rs_error ** ()                                                 { return &error; }
    };

    struct int2     { int    x,y;   bool operator == (const int2     & r) const { return x==r.x && y==r.y;           } };
    struct float2   { float  x,y;   bool operator == (const float2   & r) const { return x==r.x && y==r.y;           } };
    struct float3   { float  x,y,z; bool operator == (const float3   & r) const { return x==r.x && y==r.y && z==r.z; } };
    struct float3x3 { float3 x,y,z; bool operator == (const float3x3 & r) const { return x==r.x && y==r.y && z==r.z; } };

    struct intrinsics
    {
        int2                image_size;
        float2              focal_length;
        float2              principal_point;
        float               distortion_coeff[5];
        distortion          distortion_model;

        float3              deproject(const float2 & pixel, float depth) const                      { float3 point; rs_deproject_pixel_to_point(&point.x, (const rs_intrinsics *)this, &pixel.x, depth); return point; }
        float2              project(const float3 & point) const                                     { float2 pixel; rs_project_point_to_pixel(&pixel.x, (const rs_intrinsics *)this, &point.x); return pixel; }
    };

    struct extrinsics
    {
        float3x3            rotation;
        float3              translation;

        bool                is_identity() const                                                     { return rotation == float3x3{{1,0,0},{0,1,0},{0,0,1}} && translation == float3{0,0,0}; }
        float3              transform(const float3 & point) const                                   { float3 p; rs_transform_point_to_point(&p.x, (const rs_extrinsics *)this, &point.x); return p; }
    };

    class device
    {
        rs_device *         dev;
    public:
                            device()                                                                : dev() {}
                            device(rs_device * dev)                                                 : dev(dev) {}
        explicit            operator bool() const                                                   { return !!dev; }
        rs_device *         get_handle() const                                                      { return dev; }

        const char *        get_name()                                                              { return rs_get_device_name(dev, auto_error()); }
        extrinsics          get_extrinsics(stream from, stream to)                                  { extrinsics extrin; rs_get_device_extrinsics(dev, (rs_stream)from, (rs_stream)to, (rs_extrinsics *)&extrin, auto_error()); return extrin; }
        float               get_depth_scale()                                                       { return rs_get_device_depth_scale(dev, auto_error()); }
        bool                supports_option(option o) const                                         { return !!rs_device_supports_option(dev, (rs_option)o, auto_error()); }

        void                enable_stream(stream s, int width, int height, format f, int fps)       { rs_enable_stream(dev, (rs_stream)s, width, height, (rs_format)f, fps, auto_error()); }
        void                enable_stream_preset(stream s, preset preset)                           { rs_enable_stream_preset(dev, (rs_stream)s, (rs_preset)preset, auto_error()); }
        bool                is_stream_enabled(stream s)                                             { return !!rs_stream_is_enabled(dev, (rs_stream)s, auto_error()); }
        intrinsics          get_stream_intrinsics(stream s)                                         { intrinsics intrin; rs_get_stream_intrinsics(dev, (rs_stream)s, (rs_intrinsics *)&intrin, auto_error()); return intrin; }
        format              get_stream_format(stream s)                                             { return (format)rs_get_stream_format(dev, (rs_stream)s, auto_error()); }
        int                 get_stream_framerate(stream s)                                          { return rs_get_stream_framerate(dev, (rs_stream)s, auto_error()); }

        void                start()                                                                 { rs_start_device(dev, auto_error()); }
        void                stop()                                                                  { rs_stop_device(dev, auto_error()); }
        bool                is_streaming()                                                          { return !!rs_device_is_streaming(dev, auto_error()); }

        void                set_option(option o, int value)                                         { rs_set_device_option(dev, (rs_option)o, value, auto_error()); }
        int                 get_option(option o)                                                    { return rs_get_device_option(dev, (rs_option)o, auto_error()); }

        void                wait_for_frames(int stream_bits)                                        { rs_wait_for_frames(dev, stream_bits, auto_error()); }
        int                 get_frame_number(stream s)                                              { return rs_get_frame_number(dev, (rs_stream)s, auto_error()); }
        const void *        get_frame_data(stream s)                                                { return rs_get_frame_data(dev, (rs_stream)s, auto_error()); }
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

    inline std::ostream &   operator << (std::ostream & out, stream stream)                         { return out << rs_stream_to_string((rs_stream)stream); }
    inline std::ostream &   operator << (std::ostream & out, format format)                         { return out << rs_format_to_string((rs_format)format); }
    inline std::ostream &   operator << (std::ostream & out, preset preset)                         { return out << rs_preset_to_string((rs_preset)preset); }
    inline std::ostream &   operator << (std::ostream & out, distortion distortion)                 { return out << rs_distortion_to_string((rs_distortion)distortion); }
    inline std::ostream &   operator << (std::ostream & out, option option)                         { return out << rs_option_to_string((rs_option)option); }
}

#endif