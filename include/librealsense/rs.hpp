#ifndef LIBREALSENSE_RS_HPP
#define LIBREALSENSE_RS_HPP

#include "rsutil.h"
#include <cstdint>
#include <sstream>

namespace rs
{

    enum class stream : int32_t
    {
        depth = 0, // Native stream of depth data produced by RealSense device
        color = 1, // Native stream of color data captured by RealSense device
        infrared = 2, // Native stream of infrared data captured by RealSense device
        infrared2 = 3, // Native stream of infrared data captured from a second viewpoint by RealSense device
        rectified_color = 4, // Synthetic stream containing undistorted color data with no extrinsic rotation from the depth stream
        color_aligned_to_depth = 5, // Synthetic stream containing color data but sharing intrinsics of depth stream
        depth_aligned_to_color = 6, // Synthetic stream containing depth data but sharing intrinsics of color stream
        depth_aligned_to_rectified_color = 7 // Synthetic stream containing depth data but sharing intrinsics of rectified color stream
    };

    enum class format : int32_t
    {
        any = 0,
        z16 = 1,
        yuyv = 2,
        rgb8 = 3,
        bgr8 = 4,
        rgba8 = 5,
        bgra8 = 6,
        y8 = 7,
        y16 = 8
    };

    enum class preset : int32_t
    {
        best_quality = 0,
        largest_image = 1,
        highest_framerate = 2
    };

    enum class distortion : int32_t
    {
        none = 0, // Rectilinear images, no distortion compensation required
        modified_brown_conrady = 1, // Equivalent to Brown-Conrady distortion, except that tangential distortion is applied to radially distorted points
        inverse_brown_conrady = 2 // Equivalent to Brown-Conrady distortion, except undistorts image instead of distorting it
    };

    enum class option : int32_t
    {
        color_backlight_compensation = 0,
        color_brightness = 1,
        color_contrast = 2,
        color_exposure = 3,
        color_gain = 4,
        color_gamma = 5,
        color_hue = 6,
        color_saturation = 7,
        color_sharpness = 8,
        color_white_balance = 9,
        f200_laser_power = 10, // 0 - 15
        f200_accuracy = 11, // 0 - 3
        f200_motion_range = 12, // 0 - 100
        f200_filter_option = 13, // 0 - 7
        f200_confidence_threshold = 14, // 0 - 15
        f200_dynamic_fps = 15, // {2, 5, 15, 30, 60}
        r200_lr_auto_exposure_enabled = 16, // {0, 1}
        r200_lr_gain = 17, // 100 - 1600 (Units of 0.01)
        r200_lr_exposure = 18, // > 0 (Units of 0.1 ms)
        r200_emitter_enabled = 19, // {0, 1}
        r200_depth_control_preset = 20, // {0, 5}, 0 is default, 1-5 is low to high outlier rejection
        r200_depth_units = 21, // > 0
        r200_depth_clamp_min = 22, // 0 - USHORT_MAX
        r200_depth_clamp_max = 23, // 0 - USHORT_MAX
        r200_disparity_mode_enabled = 24, // {0, 1}
        r200_disparity_multiplier = 25,
        r200_disparity_shift = 26
    };

    struct float2 { float x,y; };
    struct float3 { float x,y,z; };

    struct intrinsics : rs_intrinsics
    {
        float       hfov() const                                                        { return (atan2f(ppx + 0.5f, fx) + atan2f(width - (ppx + 0.5f), fx)) * 57.2957795f; }
        float       vfov() const                                                        { return (atan2f(ppy + 0.5f, fy) + atan2f(height - (ppy + 0.5f), fy)) * 57.2957795f; }
        distortion  model() const                                                       { return (distortion)rs_intrinsics::model; }

                    // Helpers for mapping between pixel coordinates and texture coordinates
        float2      pixel_to_texcoord(const float2 & pixel) const                       { return {(pixel.x+0.5f)/width, (pixel.y+0.5f)/height}; }
        float2      texcoord_to_pixel(const float2 & coord) const                       { return {coord.x*width - 0.5f, coord.y*height - 0.5f}; }

                    // Helpers for mapping from image coordinates into 3D space
        float3      deproject(const float2 & pixel, float depth) const                  { float3 point; rs_deproject_pixel_to_point(&point.x, this, &pixel.x, depth); return point; }
        float3      deproject_from_texcoord(const float2 & coord, float depth) const    { return deproject(texcoord_to_pixel(coord), depth); }

                    // Helpers for mapping from 3D space into image coordinates
        float2      project(const float3 & point) const                                 { float2 pixel; rs_project_point_to_pixel(&pixel.x, this, &point.x); return pixel; }
        float2      project_to_texcoord(const float3 & point) const                     { return pixel_to_texcoord(project(point)); }

        bool        operator == (const intrinsics & r) const                            { return memcmp(this, &r, sizeof(r)) == 0; }

    };
    struct extrinsics : rs_extrinsics
    {
        bool        is_identity() const                                                 { return rotation[0] == 1 && rotation[4] == 1 && translation[0] == 0 && translation[1] == 0 && translation[2] == 0; }
        float3      transform(const float3 & point) const                               { float3 p; rs_transform_point_to_point(&p.x, this, &point.x); return p; }

    };
    class context;
    class device;
    
    class error : public std::runtime_error
    {
        std::string function, args;
    public:
        error(rs_error * err) : std::runtime_error(rs_get_error_message(err)), function(rs_get_failed_function(err)), args(rs_get_failed_args(err)) { rs_free_error(err); }
        const std::string & get_failed_function() const { return function; }
        const std::string & get_failed_args() const { return args; }
        static void handle(rs_error * e) { if(e) throw error(e); }
    };

    class context
    {
        rs_context * handle;
        context(const context &) = delete;
        context & operator = (const context &) = delete;
    public:
        context()
        {
            rs_error * e = nullptr;
            handle = rs_create_context(3, &e);
            error::handle(e);
        }

        ~context()
        {
            rs_delete_context(handle, nullptr);
        }

        int get_device_count() const
        {
            rs_error * e = nullptr;
            auto r = rs_get_device_count(handle, &e);
            error::handle(e);
            return r;
        }

        device & get_device(int index)
        {
            rs_error * e = nullptr;
            auto r = rs_get_device(handle, index, &e);
            error::handle(e);
            return *(device *)r;
        }
    };

    class device
    {
        device() = delete;
        device(const device &) = delete;
        device & operator = (const device &) = delete;
        ~device() = delete;
    public:
        const char * get_name() const
        {
            rs_error * e = nullptr;
            auto r = rs_get_device_name((const rs_device *)this, &e);
            error::handle(e);
            return r;
        }

        const char * get_serial() const
        {
            rs_error * e = nullptr;
            auto r = rs_get_device_serial((const rs_device *)this, &e);
            error::handle(e);
            return r;
        }

        const char * get_firmware_version() const
        {
            rs_error * e = nullptr;
            auto r = rs_get_device_firmware_version((const rs_device *)this, &e);
            error::handle(e);
            return r;
        }

        extrinsics get_extrinsics(stream from_stream, stream to_stream) const
        {
            rs_error * e = nullptr;
            extrinsics extrin;
            rs_get_device_extrinsics((const rs_device *)this, (rs_stream)from_stream, (rs_stream)to_stream, &extrin, &e);
            error::handle(e);
            return extrin;
        }

        float get_depth_scale() const
        {
            rs_error * e = nullptr;
            auto r = rs_get_device_depth_scale((const rs_device *)this, &e);
            error::handle(e);
            return r;
        }

        bool supports_option(option option) const
        {
            rs_error * e = nullptr;
            auto r = rs_device_supports_option((const rs_device *)this, (rs_option)option, &e);
            error::handle(e);
            return r != 0;
        }

        int get_stream_mode_count(stream stream) const
        {
            rs_error * e = nullptr;
            auto r = rs_get_stream_mode_count((const rs_device *)this, (rs_stream)stream, &e);
            error::handle(e);
            return r;
        }

        void get_stream_mode(stream stream, int index, int & width, int & height, format & format, int & framerate) const
        {
            rs_error * e = nullptr;
            rs_get_stream_mode((const rs_device *)this, (rs_stream)stream, index, &width, &height, (rs_format *)&format, &framerate, &e);
            error::handle(e);
        }

        void enable_stream(stream stream, int width, int height, format format, int framerate)
        {
            rs_error * e = nullptr;
            rs_enable_stream((rs_device *)this, (rs_stream)stream, width, height, (rs_format)format, framerate, &e);
            error::handle(e);
        }

        void enable_stream(stream stream, preset preset)
        {
            rs_error * e = nullptr;
            rs_enable_stream_preset((rs_device *)this, (rs_stream)stream, (rs_preset)preset, &e);
            error::handle(e);
        }

        void disable_stream(stream stream)
        {
            rs_error * e = nullptr;
            rs_disable_stream((rs_device *)this, (rs_stream)stream, &e);
            error::handle(e);
        }

        bool is_stream_enabled(stream stream) const
        {
            rs_error * e = nullptr;
            auto r = rs_is_stream_enabled((const rs_device *)this, (rs_stream)stream, &e);
            error::handle(e);
            return r != 0;
        }

        intrinsics get_stream_intrinsics(stream stream) const
        {
            rs_error * e = nullptr;
            intrinsics intrin;
            rs_get_stream_intrinsics((const rs_device *)this, (rs_stream)stream, &intrin, &e);
            error::handle(e);
            return intrin;
        }

        format get_stream_format(stream stream) const
        {
            rs_error * e = nullptr;
            auto r = rs_get_stream_format((const rs_device *)this, (rs_stream)stream, &e);
            error::handle(e);
            return (format)r;
        }

        int get_stream_framerate(stream stream) const
        {
            rs_error * e = nullptr;
            auto r = rs_get_stream_framerate((const rs_device *)this, (rs_stream)stream, &e);
            error::handle(e);
            return r;
        }

        void start()
        {
            rs_error * e = nullptr;
            rs_start_device((rs_device *)this, &e);
            error::handle(e);
        }

        void stop()
        {
            rs_error * e = nullptr;
            rs_stop_device((rs_device *)this, &e);
            error::handle(e);
        }

        bool is_streaming() const
        {
            rs_error * e = nullptr;
            auto r = rs_is_device_streaming((const rs_device *)this, &e);
            error::handle(e);
            return r != 0;
        }

        void set_option(option option, int value)
        {
            rs_error * e = nullptr;
            rs_set_device_option((rs_device *)this, (rs_option)option, value, &e);
            error::handle(e);
        }

        int get_option(option option) const
        {
            rs_error * e = nullptr;
            auto r = rs_get_device_option((const rs_device *)this, (rs_option)option, &e);
            error::handle(e);
            return r;
        }

        void wait_for_frames()
        {
            rs_error * e = nullptr;
            rs_wait_for_frames((rs_device *)this, &e);
            error::handle(e);
        }

        int get_frame_timestamp(stream stream) const
        {
            rs_error * e = nullptr;
            auto r = rs_get_frame_timestamp((const rs_device *)this, (rs_stream)stream, &e);
            error::handle(e);
            return r;
        }

        const void * get_frame_data(stream stream) const
        {
            rs_error * e = nullptr;
            auto r = rs_get_frame_data((const rs_device *)this, (rs_stream)stream, &e);
            error::handle(e);
            return r;
        }
    };

    std::ostream & operator << (std::ostream & o, stream stream) { return o << rs_stream_to_string((rs_stream)stream); }
    std::ostream & operator << (std::ostream & o, format format) { return o << rs_format_to_string((rs_format)format); }
    std::ostream & operator << (std::ostream & o, preset preset) { return o << rs_preset_to_string((rs_preset)preset); }
    std::ostream & operator << (std::ostream & o, distortion distortion) { return o << rs_distortion_to_string((rs_distortion)distortion); }
    std::ostream & operator << (std::ostream & o, option option) { return o << rs_option_to_string((rs_option)option); }
}
#endif
