#ifndef LIBREALSENSE_RS_HPP
#define LIBREALSENSE_RS_HPP

#include "rsutil.h"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <sstream>
#include <stdexcept>

namespace rs
{

    enum class stream : int32_t
    {
        depth                            = 0, ///< Native stream of depth data produced by RealSense device
        color                            = 1, ///< Native stream of color data captured by RealSense device
        infrared                         = 2, ///< Native stream of infrared data captured by RealSense device
        infrared2                        = 3, ///< Native stream of infrared data captured from a second viewpoint by RealSense device
        rectified_color                  = 4, ///< Synthetic stream containing undistorted color data with no extrinsic rotation from the depth stream
        color_aligned_to_depth           = 5, ///< Synthetic stream containing color data but sharing intrinsics of depth stream
        depth_aligned_to_color           = 6, ///< Synthetic stream containing depth data but sharing intrinsics of color stream
        depth_aligned_to_rectified_color = 7  ///< Synthetic stream containing depth data but sharing intrinsics of rectified color stream
    };

    enum class format : int32_t
    {
        any         = 0, 
        z16         = 1, ///< 16 bit linear depth values. The depth is meters is equal to depth scale * pixel value
        disparity16 = 2, ///< 16 bit linear disparity values. The depth in meters is equal to depth scale / pixel value
        yuyv        = 3, 
        rgb8        = 4, 
        bgr8        = 5, 
        rgba8       = 6, 
        bgra8       = 7, 
        y8          = 8, 
        y16         = 9, 
        raw10       = 10 ///< Four 10-bit luminance values encoded into a 5-byte macropixel
    };

    enum class preset : int32_t
    {
        best_quality      = 0, 
        largest_image     = 1, 
        highest_framerate = 2  
    };

    enum class distortion : int32_t
    {
        none                   = 0, ///< Rectilinear images, no distortion compensation required
        modified_brown_conrady = 1, ///< Equivalent to Brown-Conrady distortion, except that tangential distortion is applied to radially distorted points
        inverse_brown_conrady  = 2  ///< Equivalent to Brown-Conrady distortion, except undistorts image instead of distorting it
    };

    enum class option : int32_t
    {
        color_backlight_compensation    = 0,  
        color_brightness                = 1,  
        color_contrast                  = 2,  
        color_exposure                  = 3,  ///< Controls exposure time of color camera. Setting any value will disable auto exposure.
        color_gain                      = 4,  
        color_gamma                     = 5,  
        color_hue                       = 6,  
        color_saturation                = 7,  
        color_sharpness                 = 8,  
        color_white_balance             = 9,  ///< Controls white balance of color image. Setting any value will disable auto white balance.
        color_enable_auto_exposure      = 10, ///< Set to 1 to enable automatic exposure control, or 0 to return to manual control
        color_enable_auto_white_balance = 11, ///< Set to 1 to enable automatic white balance control, or 0 to return to manual control
        f200_laser_power                = 12, ///< 0 - 15
        f200_accuracy                   = 13, ///< 0 - 3
        f200_motion_range               = 14, ///< 0 - 100
        f200_filter_option              = 15, ///< 0 - 7
        f200_confidence_threshold       = 16, ///< 0 - 15
        f200_dynamic_fps                = 17, ///< {2, 5, 15, 30, 60}
        r200_lr_auto_exposure_enabled   = 18, ///< {0, 1}
        r200_lr_gain                    = 19, ///< 100 - 1600 (Units of 0.01)
        r200_lr_exposure                = 20, ///< > 0 (Units of 0.1 ms)
        r200_emitter_enabled            = 21, ///< {0, 1}
        r200_depth_units                = 22, ///< micrometers per increment in integer depth values, 1000 is default (mm scale)
        r200_depth_clamp_min            = 23, ///< 0 - USHORT_MAX
        r200_depth_clamp_max            = 24, ///< 0 - USHORT_MAX
        r200_disparity_multiplier       = 25, ///< 0 - 1000, the increments in integer disparity values corresponding to one pixel of disparity
        r200_disparity_shift            = 26  
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

    struct f200_auto_range_parameters : rs_f200_auto_range_parameters{};

    struct r200_lr_auto_exposure_parameters : rs_r200_lr_auto_exposure_parameters{};

    struct r200_depth_control_parameters : rs_r200_depth_control_parameters
    {
        static r200_depth_control_parameters get_preset(int preset) { r200_depth_control_parameters p; rs_get_depth_control_parameters_preset(preset, &p); return p; }
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
            handle = rs_create_context(4, &e);
            error::handle(e);
        }

        ~context()
        {
            rs_delete_context(handle, nullptr);
        }

        /// determine number of connected devices
        /// \return  the count of devices
        int get_device_count() const
        {
            rs_error * e = nullptr;
            auto r = rs_get_device_count(handle, &e);
            error::handle(e);
            return r;
        }

        /// retrieve connected device by index
        /// \param[in] index  the zero based index of device to retrieve
        /// \return           the requested device
        device * get_device(int index)
        {
            rs_error * e = nullptr;
            auto r = rs_get_device(handle, index, &e);
            error::handle(e);
            return (device *)r;
        }
    };

    class device
    {
        device() = delete;
        device(const device &) = delete;
        device & operator = (const device &) = delete;
        ~device() = delete;
    public:
        /// retrieve a human readable device model string
        /// \return  the model string, such as "Intel RealSense F200" or "Intel RealSense R200"
        const char * get_name() const
        {
            rs_error * e = nullptr;
            auto r = rs_get_device_name((const rs_device *)this, &e);
            error::handle(e);
            return r;
        }

        /// retrieve the unique serial number of the device
        /// \return  the serial number, in a format specific to the device model
        const char * get_serial() const
        {
            rs_error * e = nullptr;
            auto r = rs_get_device_serial((const rs_device *)this, &e);
            error::handle(e);
            return r;
        }

        /// retrieve the version of the firmware currently installed on the device
        /// \return  firmware version string, in a format is specific to device model
        const char * get_firmware_version() const
        {
            rs_error * e = nullptr;
            auto r = rs_get_device_firmware_version((const rs_device *)this, &e);
            error::handle(e);
            return r;
        }

        /// retrieve extrinsic transformation between the viewpoints of two different streams
        /// \param[in] from_stream  stream whose coordinate space we will transform from
        /// \param[in] to_stream    stream whose coordinate space we will transform to
        /// \return                 the transformation between the two streams
        extrinsics get_extrinsics(stream from_stream, stream to_stream) const
        {
            rs_error * e = nullptr;
            extrinsics extrin;
            rs_get_device_extrinsics((const rs_device *)this, (rs_stream)from_stream, (rs_stream)to_stream, &extrin, &e);
            error::handle(e);
            return extrin;
        }

        /// retrieve mapping between the units of the depth image and meters
        /// \return  depth in meters corresponding to a depth value of 1
        float get_depth_scale() const
        {
            rs_error * e = nullptr;
            auto r = rs_get_device_depth_scale((const rs_device *)this, &e);
            error::handle(e);
            return r;
        }

        /// determine if the device allows a specific option to be queried and set
        /// \param[in] option  the option to check for support
        /// \return            true if the option can be queried and set
        bool supports_option(option option) const
        {
            rs_error * e = nullptr;
            auto r = rs_device_supports_option((const rs_device *)this, (rs_option)option, &e);
            error::handle(e);
            return r != 0;
        }

        /// determine the range of acceptable values for an option on this device
        /// \param[in] option  the option whose range to query
        /// \param[out] min    the minimum acceptable value, attempting to set a value below this will take no effect and raise an error
        /// \param[out] max    the maximum acceptable value, attempting to set a value above this will take no effect and raise an error
        void get_option_range(option option, int & min, int & max) const
        {
            rs_error * e = nullptr;
            rs_get_device_option_range((const rs_device *)this, (rs_option)option, &min, &max, &e);
            error::handle(e);
        }

        /// determine the number of streaming modes available for a given stream
        /// \param[in] stream  the stream whose modes will be enumerated
        /// \return            the count of available modes
        int get_stream_mode_count(stream stream) const
        {
            rs_error * e = nullptr;
            auto r = rs_get_stream_mode_count((const rs_device *)this, (rs_stream)stream, &e);
            error::handle(e);
            return r;
        }

        /// determine the properties of a specific streaming mode
        /// \param[in] stream      the stream whose mode will be queried
        /// \param[in] index       the zero based index of the streaming mode
        /// \param[out] width      the width of a frame image in pixels
        /// \param[out] height     the height of a frame image in pixels
        /// \param[out] format     the pixel format of a frame image
        /// \param[out] framerate  the number of frames which will be streamed per second
        void get_stream_mode(stream stream, int index, int & width, int & height, format & format, int & framerate) const
        {
            rs_error * e = nullptr;
            rs_get_stream_mode((const rs_device *)this, (rs_stream)stream, index, &width, &height, (rs_format *)&format, &framerate, &e);
            error::handle(e);
        }

        /// enable a specific stream and request specific properties
        /// \param[in] stream     the stream to enable
        /// \param[in] width      the desired width of a frame image in pixels, or 0 if any width is acceptable
        /// \param[in] height     the desired height of a frame image in pixels, or 0 if any height is acceptable
        /// \param[in] format     the pixel format of a frame image, or ANY if any format is acceptable
        /// \param[in] framerate  the number of frames which will be streamed per second, or 0 if any framerate is acceptable
        void enable_stream(stream stream, int width, int height, format format, int framerate)
        {
            rs_error * e = nullptr;
            rs_enable_stream((rs_device *)this, (rs_stream)stream, width, height, (rs_format)format, framerate, &e);
            error::handle(e);
        }

        /// enable a specific stream and request properties using a preset
        /// \param[in] stream  the stream to enable
        /// \param[in] preset  the preset to use to enable the stream
        void enable_stream(stream stream, preset preset)
        {
            rs_error * e = nullptr;
            rs_enable_stream_preset((rs_device *)this, (rs_stream)stream, (rs_preset)preset, &e);
            error::handle(e);
        }

        /// disable a specific stream
        /// \param[in] stream  the stream to disable
        void disable_stream(stream stream)
        {
            rs_error * e = nullptr;
            rs_disable_stream((rs_device *)this, (rs_stream)stream, &e);
            error::handle(e);
        }

        /// determine if a specific stream is enabled
        /// \param[in] stream  the stream to check
        /// \return            true if the stream is currently enabled
        bool is_stream_enabled(stream stream) const
        {
            rs_error * e = nullptr;
            auto r = rs_is_stream_enabled((const rs_device *)this, (rs_stream)stream, &e);
            error::handle(e);
            return r != 0;
        }

        /// retrieve intrinsic camera parameters for a specific stream
        /// \param[in] stream  the stream whose parameters to retrieve
        /// \return            the intrinsic parameters of the stream
        intrinsics get_stream_intrinsics(stream stream) const
        {
            rs_error * e = nullptr;
            intrinsics intrin;
            rs_get_stream_intrinsics((const rs_device *)this, (rs_stream)stream, &intrin, &e);
            error::handle(e);
            return intrin;
        }

        /// retrieve the pixel format for a specific stream
        /// \param[in] stream  the stream whose format to retrieve
        /// \return            the pixel format of the stream
        format get_stream_format(stream stream) const
        {
            rs_error * e = nullptr;
            auto r = rs_get_stream_format((const rs_device *)this, (rs_stream)stream, &e);
            error::handle(e);
            return (format)r;
        }

        /// retrieve the framerate for a specific stream
        /// \param[in] stream  the stream whose framerate to retrieve
        /// \return            the framerate of the stream, in frames per second
        int get_stream_framerate(stream stream) const
        {
            rs_error * e = nullptr;
            auto r = rs_get_stream_framerate((const rs_device *)this, (rs_stream)stream, &e);
            error::handle(e);
            return r;
        }

        /// begin streaming on all enabled streams for this device
        ///
        void start()
        {
            rs_error * e = nullptr;
            rs_start_device((rs_device *)this, &e);
            error::handle(e);
        }

        /// end streaming on all streams for this device
        ///
        void stop()
        {
            rs_error * e = nullptr;
            rs_stop_device((rs_device *)this, &e);
            error::handle(e);
        }

        /// determine if the device is currently streaming
        /// \return  true if the device is currently streaming
        bool is_streaming() const
        {
            rs_error * e = nullptr;
            auto r = rs_is_device_streaming((const rs_device *)this, &e);
            error::handle(e);
            return r != 0;
        }

        /// set the value of a specific device option
        /// \param[in] option  the option whose value to set
        /// \param[in] value   the desired value to set
        void set_option(option option, int value)
        {
            rs_error * e = nullptr;
            rs_set_device_option((rs_device *)this, (rs_option)option, value, &e);
            error::handle(e);
        }

        /// query the current value of a specific device option
        /// \param[in] option  the option whose value to retrieve
        /// \return            the current value of the option
        int get_option(option option) const
        {
            rs_error * e = nullptr;
            auto r = rs_get_device_option((const rs_device *)this, (rs_option)option, &e);
            error::handle(e);
            return r;
        }

        void set_auto_range_parameters(const f200_auto_range_parameters & parameters)
        {
            rs_error * e = nullptr;
            rs_set_auto_range_parameters((rs_device *)this, (const rs_f200_auto_range_parameters *)&parameters, &e);
            error::handle(e);
        }

        f200_auto_range_parameters get_auto_range_parameters() const
        {
            rs_error * e = nullptr;
            f200_auto_range_parameters parameters;
            rs_get_auto_range_parameters((const rs_device *)this, &parameters, &e);
            error::handle(e);
            return parameters;
        }

        void set_lr_auto_exposure_parameters(const r200_lr_auto_exposure_parameters & parameters)
        {
            rs_error * e = nullptr;
            rs_set_lr_auto_exposure_parameters((rs_device *)this, (const rs_r200_lr_auto_exposure_parameters *)&parameters, &e);
            error::handle(e);
        }

        r200_lr_auto_exposure_parameters get_lr_auto_exposure_parameters() const
        {
            rs_error * e = nullptr;
            r200_lr_auto_exposure_parameters parameters;
            rs_get_lr_auto_exposure_parameters((const rs_device *)this, &parameters, &e);
            error::handle(e);
            return parameters;
        }

        void set_depth_control_parameters(const r200_depth_control_parameters & parameters)
        {
            rs_error * e = nullptr;
            rs_set_depth_control_parameters((rs_device *)this, (const rs_r200_depth_control_parameters *)&parameters, &e);
            error::handle(e);
        }

        r200_depth_control_parameters get_depth_control_parameters() const
        {
            rs_error * e = nullptr;
            r200_depth_control_parameters parameters;
            rs_get_depth_control_parameters((const rs_device *)this, &parameters, &e);
            error::handle(e);
            return parameters;
        }

        /// block until new frames are available
        ///
        void wait_for_frames()
        {
            rs_error * e = nullptr;
            rs_wait_for_frames((rs_device *)this, &e);
            error::handle(e);
        }

        /// retrieve the time at which the latest frame on a stream was captured
        /// \param[in] stream  the stream whose latest frame we are interested in
        /// \return            the timestamp of the frame, in milliseconds since the device was started
        int get_frame_timestamp(stream stream) const
        {
            rs_error * e = nullptr;
            auto r = rs_get_frame_timestamp((const rs_device *)this, (rs_stream)stream, &e);
            error::handle(e);
            return r;
        }

        /// retrieve the contents of the latest frame on a stream
        /// \param[in] stream  the stream whose latest frame we are interested in
        /// \return            the pointer to the start of the frame data
        const void * get_frame_data(stream stream) const
        {
            rs_error * e = nullptr;
            auto r = rs_get_frame_data((const rs_device *)this, (rs_stream)stream, &e);
            error::handle(e);
            return r;
        }
    };

    inline std::ostream & operator << (std::ostream & o, stream stream) { return o << rs_stream_to_string((rs_stream)stream); }
    inline std::ostream & operator << (std::ostream & o, format format) { return o << rs_format_to_string((rs_format)format); }
    inline std::ostream & operator << (std::ostream & o, preset preset) { return o << rs_preset_to_string((rs_preset)preset); }
    inline std::ostream & operator << (std::ostream & o, distortion distortion) { return o << rs_distortion_to_string((rs_distortion)distortion); }
    inline std::ostream & operator << (std::ostream & o, option option) { return o << rs_option_to_string((rs_option)option); }
}
#endif
