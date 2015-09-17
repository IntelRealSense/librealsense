#ifndef LIBREALSENSE_CPP_INCLUDE_GUARD
#define LIBREALSENSE_CPP_INCLUDE_GUARD

#include "rs.h"
#include "rsutil.h"

#include <cmath>        // For atan2f
#include <cstdint>      // For int32_t
#include <cstring>      // For memcmp
#include <ostream>      // For std::ostream
#include <stdexcept>    // For std::runtime_error

namespace rs
{
    enum class stream : int32_t
    {
        depth                           = RS_STREAM_DEPTH,
        color                           = RS_STREAM_COLOR,
        infrared                        = RS_STREAM_INFRARED,
        infrared2                       = RS_STREAM_INFRARED2
    };

    enum class format : int32_t
    {
        any                             = RS_FORMAT_ANY,
        z16                             = RS_FORMAT_Z16,
        yuyv                            = RS_FORMAT_YUYV,
        rgb8                            = RS_FORMAT_RGB8,
        bgr8                            = RS_FORMAT_BGR8,
        rgba8                           = RS_FORMAT_RGBA8,
        bgra8                           = RS_FORMAT_BGRA8,
        y8                              = RS_FORMAT_Y8,
        y16                             = RS_FORMAT_Y16
    };

    enum class preset : int32_t
    {
        best_quality                    = RS_PRESET_BEST_QUALITY,
        largest_image                   = RS_PRESET_LARGEST_IMAGE,
        highest_framerate               = RS_PRESET_HIGHEST_FRAMERATE
    };

    enum class distortion : int32_t
    {
        none                            = RS_DISTORTION_NONE,
        modified_brown_conrady          = RS_DISTORTION_MODIFIED_BROWN_CONRADY,
        inverse_brown_conrady           = RS_DISTORTION_INVERSE_BROWN_CONRADY
    };

    enum class option : int32_t
    {
        color_backlight_compensation    = RS_OPTION_COLOR_BACKLIGHT_COMPENSATION,
        color_brightness                = RS_OPTION_COLOR_BRIGHTNESS,
        color_contrast                  = RS_OPTION_COLOR_CONTRAST,
        color_exposure                  = RS_OPTION_COLOR_EXPOSURE,
        color_gain                      = RS_OPTION_COLOR_GAIN,
        color_gamma                     = RS_OPTION_COLOR_GAMMA,
        color_hue                       = RS_OPTION_COLOR_HUE,
        color_saturation                = RS_OPTION_COLOR_SATURATION,
        color_sharpness                 = RS_OPTION_COLOR_SHARPNESS,
        color_white_balance             = RS_OPTION_COLOR_WHITE_BALANCE,

        f200_laser_power                = RS_OPTION_F200_LASER_POWER,
        f200_accuracy                   = RS_OPTION_F200_ACCURACY,
        f200_motion_range               = RS_OPTION_F200_MOTION_RANGE,
        f200_filter_option              = RS_OPTION_F200_FILTER_OPTION,
        f200_confidence_threshold       = RS_OPTION_F200_CONFIDENCE_THRESHOLD,
        f200_dynamic_fps                = RS_OPTION_F200_DYNAMIC_FPS,

        r200_lr_auto_exposure_enabled   = RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED,
        r200_lr_gain                    = RS_OPTION_R200_LR_GAIN,
        r200_lr_exposure                = RS_OPTION_R200_LR_EXPOSURE,
        r200_emitter_enabled            = RS_OPTION_R200_EMITTER_ENABLED,
        r200_depth_control_preset       = RS_OPTION_R200_DEPTH_CONTROL_PRESET,
        r200_depth_units                = RS_OPTION_R200_DEPTH_UNITS,
        r200_depth_clamp_min            = RS_OPTION_R200_DEPTH_CLAMP_MIN,
        r200_depth_clamp_max            = RS_OPTION_R200_DEPTH_CLAMP_MAX,
        r200_disparity_mode_enabled     = RS_OPTION_R200_DISPARITY_MODE_ENABLED,
        r200_disparity_multiplier       = RS_OPTION_R200_DISPARITY_MULTIPLIER,
        r200_disparity_shift            = RS_OPTION_R200_DISPARITY_SHIFT         
    };

    class error : public std::runtime_error
    {
        std::string         function, args;
    public:
                            error(rs_error * err)                                                   : std::runtime_error(rs_get_error_message(err)), function(rs_get_failed_function(err)), args(rs_get_failed_args(err)) { rs_free_error(err); }

        const std::string & get_failed_function() const                                             { return function; }
        const std::string & get_failed_args() const                                                 { return args; }
    };

    class throw_on_error
    {
        rs_error *          err;
                            throw_on_error(const throw_on_error &)                                  = delete;
        throw_on_error &    operator = (const throw_on_error &)                                     = delete;
    public:
                            throw_on_error()                                                        : err() {}
                            #ifdef WIN32
                            ~throw_on_error()                                                       { if (err) throw error(err); }
                            #else
                            ~throw_on_error() noexcept(false)                                       { if (err) throw error(err); }
                            #endif
                            operator rs_error ** ()                                                 { return &err; }
    };

    struct float2 { float x,y; };
    struct float3 { float x,y,z; };

    struct intrinsics
    {
        rs_intrinsics       intrin;

        // Basic properties of a stream
        int                 width() const                                                           { return intrin.width; }
        int                 height() const                                                          { return intrin.height; }
        float               hfov() const                                                            { return (atan2f(intrin.ppx + 0.5f, intrin.fx) + atan2f(intrin.width - (intrin.ppx + 0.5f), intrin.fx)) * 57.2957795f; }
        float               vfov() const                                                            { return (atan2f(intrin.ppy + 0.5f, intrin.fy) + atan2f(intrin.height - (intrin.ppy + 0.5f), intrin.fy)) * 57.2957795f; }
        bool                distorted() const                                                       { return distortion_model() != distortion::none; }
        distortion          distortion_model() const                                                { return (distortion)intrin.model; }

        // Helpers for mapping between pixel coordinates and texture coordinates
        float2              pixel_to_texcoord(const float2 & pixel) const                           { return {(pixel.x+0.5f)/width(), (pixel.y+0.5f)/height()}; }
        float2              texcoord_to_pixel(const float2 & coord) const                           { return {coord.x*width() - 0.5f, coord.y*height() - 0.5f}; }

        // Helpers for mapping from image coordinates into 3D space
        float3              deproject(const float2 & pixel, float depth) const                      { float3 point; rs_deproject_pixel_to_point(&point.x, &intrin, &pixel.x, depth); return point; }
        float3              deproject_from_texcoord(const float2 & coord, float depth) const        { return deproject(texcoord_to_pixel(coord), depth); }

        // Helpers for mapping from 3D space into image coordinates
        float2              project(const float3 & point) const                                     { float2 pixel; rs_project_point_to_pixel(&pixel.x, &intrin, &point.x); return pixel; }
        float2              project_to_texcoord(const float3 & point) const                         { return pixel_to_texcoord(project(point)); }

        bool                operator == (const intrinsics & r) const                                { return memcmp(&intrin, &r.intrin, sizeof(intrin)) == 0; }
    };

    struct extrinsics
    {
        rs_extrinsics       extrin;

        bool                is_identity() const                                                     { return extrin.rotation[0] == 1 && extrin.rotation[4] == 1 && extrin.translation[0] == 0 && extrin.translation[1] == 0 && extrin.translation[2] == 0; }
        float3              transform(const float3 & point) const                                   { float3 p; rs_transform_point_to_point(&p.x, &extrin, &point.x); return p; }
    };

    class device
    {
        rs_device *         dev;
    public:
                            device()                                                                : dev() {}
                            device(rs_device * dev)                                                 : dev(dev) {}
        explicit            operator bool() const                                                   { return !!dev; }
        rs_device *         get_handle() const                                                      { return dev; }

        const char *        get_name()                                                              { return rs_get_device_name(dev, throw_on_error()); }
        extrinsics          get_extrinsics(stream from, stream to)                                  { extrinsics extrin; rs_get_device_extrinsics(dev, (rs_stream)from, (rs_stream)to, &extrin.extrin, throw_on_error()); return extrin; }
        float               get_depth_scale()                                                       { return rs_get_device_depth_scale(dev, throw_on_error()); }
        bool                supports_option(option o) const                                         { return !!rs_device_supports_option(dev, (rs_option)o, throw_on_error()); }

        void                enable_stream(stream s, int width, int height, format f, int fps)       { rs_enable_stream(dev, (rs_stream)s, width, height, (rs_format)f, fps, throw_on_error()); }
        void                enable_stream(stream s, preset preset)                                  { rs_enable_stream_preset(dev, (rs_stream)s, (rs_preset)preset, throw_on_error()); }
        void                disable_stream(stream s)                                                { rs_disable_stream(dev, (rs_stream)s, throw_on_error()); }
        bool                is_stream_enabled(stream s)                                             { return !!rs_stream_is_enabled(dev, (rs_stream)s, throw_on_error()); }
        intrinsics          get_stream_intrinsics(stream s)                                         { intrinsics intrin; rs_get_stream_intrinsics(dev, (rs_stream)s, &intrin.intrin, throw_on_error()); return intrin; }
        format              get_stream_format(stream s)                                             { return (format)rs_get_stream_format(dev, (rs_stream)s, throw_on_error()); }
        int                 get_stream_framerate(stream s)                                          { return rs_get_stream_framerate(dev, (rs_stream)s, throw_on_error()); }

        void                start()                                                                 { rs_start_device(dev, throw_on_error()); }
        void                stop()                                                                  { rs_stop_device(dev, throw_on_error()); }
        bool                is_streaming()                                                          { return !!rs_device_is_streaming(dev, throw_on_error()); }

        void                set_option(option o, int value)                                         { rs_set_device_option(dev, (rs_option)o, value, throw_on_error()); }
        int                 get_option(option o)                                                    { return rs_get_device_option(dev, (rs_option)o, throw_on_error()); }

        void                wait_for_frames()                                                       { rs_wait_for_frames(dev, throw_on_error()); }
        int                 get_frame_timestamp(stream s)                                           { return rs_get_frame_timestamp(dev, (rs_stream)s, throw_on_error()); }
        const void *        get_frame_data(stream s)                                                { return rs_get_frame_data(dev, (rs_stream)s, throw_on_error()); }
    };

    class context
    {
        rs_context *        ctx;
                            context(const context &)                                                = delete;
        context &           operator = (const context &)                                            = delete;
    public:
                            context()                                                               : ctx(rs_create_context(RS_API_VERSION, throw_on_error())) {}

                            ~context()                                                              { rs_delete_context(ctx, nullptr); } // Deliberately ignore error on destruction
        rs_context *        get_handle() const                                                      { return ctx; }

        int                 get_device_count()                                                      { return rs_get_device_count(ctx, throw_on_error()); }
        device              get_device(int index)                                                   { return rs_get_device(ctx, index, throw_on_error()); }
    };

    inline std::ostream &   operator << (std::ostream & out, stream stream)                         { for(auto s = rs_stream_to_string((rs_stream)stream);             *s; ++s) out << (char)tolower(*s); return out; }
    inline std::ostream &   operator << (std::ostream & out, format format)                         { for(auto s = rs_format_to_string((rs_format)format);             *s; ++s) out << (char)tolower(*s); return out; }
    inline std::ostream &   operator << (std::ostream & out, preset preset)                         { for(auto s = rs_preset_to_string((rs_preset)preset);             *s; ++s) out << (char)tolower(*s); return out; }
    inline std::ostream &   operator << (std::ostream & out, distortion distortion)                 { for(auto s = rs_distortion_to_string((rs_distortion)distortion); *s; ++s) out << (char)tolower(*s); return out; }
    inline std::ostream &   operator << (std::ostream & out, option option)                         { for(auto s = rs_option_to_string((rs_option)option);             *s; ++s) out << (char)tolower(*s); return out; }
}

static_assert(sizeof(rs::intrinsics) == sizeof(rs_intrinsics), "struct layout error");
static_assert(sizeof(rs::extrinsics) == sizeof(rs_extrinsics), "struct layout error");

#endif
