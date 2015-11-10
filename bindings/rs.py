''' LibRealSense bindings for Python
'''

from ctypes import *

realsense = cdll.LoadLibrary("realsense")
realsense.rs_create_context.restype = c_void_p
realsense.rs_delete_context.restype = None
realsense.rs_get_device_count.restype = c_int
realsense.rs_get_device.restype = c_void_p
realsense.rs_get_device_name.restype = c_char_p
realsense.rs_get_device_serial.restype = c_char_p
realsense.rs_get_device_firmware_version.restype = c_char_p
realsense.rs_get_device_extrinsics.restype = None
realsense.rs_get_device_depth_scale.restype = c_float
realsense.rs_device_supports_option.restype = c_int
realsense.rs_get_stream_mode_count.restype = c_int
realsense.rs_get_stream_mode.restype = None
realsense.rs_enable_stream.restype = None
realsense.rs_enable_stream.restype = None
realsense.rs_disable_stream.restype = None
realsense.rs_is_stream_enabled.restype = c_int
realsense.rs_get_stream_intrinsics.restype = None
realsense.rs_get_stream_format.restype = c_int
realsense.rs_get_stream_framerate.restype = c_int
realsense.rs_start_device.restype = None
realsense.rs_stop_device.restype = None
realsense.rs_is_device_streaming.restype = c_int
realsense.rs_set_device_option.restype = None
realsense.rs_get_device_option.restype = c_int
realsense.rs_wait_for_frames.restype = None
realsense.rs_get_frame_timestamp.restype = c_int
realsense.rs_get_frame_data.restype = c_void_p
realsense.rs_get_failed_function.restype = c_char_p
realsense.rs_get_failed_args.restype = c_char_p
realsense.rs_get_error_message.restype = c_char_p
realsense.rs_free_error.restype = None
realsense.rs_stream_to_string.restype = c_char_p
realsense.rs_format_to_string.restype = c_char_p
realsense.rs_preset_to_string.restype = c_char_p
realsense.rs_distortion_to_string.restype = c_char_p
realsense.rs_option_to_string.restype = c_char_p

STREAM_DEPTH                            = 0  ##< Native stream of depth data produced by RealSense device
STREAM_COLOR                            = 1  ##< Native stream of color data captured by RealSense device
STREAM_INFRARED                         = 2  ##< Native stream of infrared data captured by RealSense device
STREAM_INFRARED2                        = 3  ##< Native stream of infrared data captured from a second viewpoint by RealSense device
STREAM_RECTIFIED_COLOR                  = 4  ##< Synthetic stream containing undistorted color data with no extrinsic rotation from the depth stream
STREAM_COLOR_ALIGNED_TO_DEPTH           = 5  ##< Synthetic stream containing color data but sharing intrinsics of depth stream
STREAM_DEPTH_ALIGNED_TO_COLOR           = 6  ##< Synthetic stream containing depth data but sharing intrinsics of color stream
STREAM_DEPTH_ALIGNED_TO_RECTIFIED_COLOR = 7  ##< Synthetic stream containing depth data but sharing intrinsics of rectified color stream
STREAM_COUNT                            = 8 

FORMAT_ANY   = 0  
FORMAT_Z16   = 1  
FORMAT_YUYV  = 2  
FORMAT_RGB8  = 3  
FORMAT_BGR8  = 4  
FORMAT_RGBA8 = 5  
FORMAT_BGRA8 = 6  
FORMAT_Y8    = 7  
FORMAT_Y16   = 8  
FORMAT_RAW10 = 9   ##< Four 10-bit luminance values encoded into a 5-byte macropixel
FORMAT_COUNT = 10 

PRESET_BEST_QUALITY      = 0 
PRESET_LARGEST_IMAGE     = 1 
PRESET_HIGHEST_FRAMERATE = 2 
PRESET_COUNT             = 3 

DISTORTION_NONE                   = 0  ##< Rectilinear images, no distortion compensation required
DISTORTION_MODIFIED_BROWN_CONRADY = 1  ##< Equivalent to Brown-Conrady distortion, except that tangential distortion is applied to radially distorted points
DISTORTION_INVERSE_BROWN_CONRADY  = 2  ##< Equivalent to Brown-Conrady distortion, except undistorts image instead of distorting it
DISTORTION_COUNT                  = 3 

OPTION_COLOR_BACKLIGHT_COMPENSATION  = 0  
OPTION_COLOR_BRIGHTNESS              = 1  
OPTION_COLOR_CONTRAST                = 2  
OPTION_COLOR_EXPOSURE                = 3  
OPTION_COLOR_GAIN                    = 4  
OPTION_COLOR_GAMMA                   = 5  
OPTION_COLOR_HUE                     = 6  
OPTION_COLOR_SATURATION              = 7  
OPTION_COLOR_SHARPNESS               = 8  
OPTION_COLOR_WHITE_BALANCE           = 9  
OPTION_F200_LASER_POWER              = 10  ##< 0 - 15
OPTION_F200_ACCURACY                 = 11  ##< 0 - 3
OPTION_F200_MOTION_RANGE             = 12  ##< 0 - 100
OPTION_F200_FILTER_OPTION            = 13  ##< 0 - 7
OPTION_F200_CONFIDENCE_THRESHOLD     = 14  ##< 0 - 15
OPTION_F200_DYNAMIC_FPS              = 15  ##< {2, 5, 15, 30, 60}
OPTION_R200_LR_AUTO_EXPOSURE_ENABLED = 16  ##< {0, 1}
OPTION_R200_LR_GAIN                  = 17  ##< 100 - 1600 (Units of 0.01)
OPTION_R200_LR_EXPOSURE              = 18  ##< > 0 (Units of 0.1 ms)
OPTION_R200_EMITTER_ENABLED          = 19  ##< {0, 1}
OPTION_R200_DEPTH_CONTROL_PRESET     = 20  ##< 0 - 5, 0 is default, 1-5 is low to high outlier rejection
OPTION_R200_DEPTH_UNITS              = 21  ##< micrometers per increment in integer depth values, 1000 is default (mm scale)
OPTION_R200_DEPTH_CLAMP_MIN          = 22  ##< 0 - USHORT_MAX
OPTION_R200_DEPTH_CLAMP_MAX          = 23  ##< 0 - USHORT_MAX
OPTION_R200_DISPARITY_MODE_ENABLED   = 24  ##< {0, 1}
OPTION_R200_DISPARITY_MULTIPLIER     = 25 
OPTION_R200_DISPARITY_SHIFT          = 26 
OPTION_COUNT                         = 27 

class Intrinsics(Structure):
    _fields_ = [("width", c_int),
                ("height", c_int),
                ("ppx", c_float),
                ("ppy", c_float),
                ("fx", c_float),
                ("fy", c_float),
                ("model", c_int),
                ("coeffs", c_float * 5)]

class Extrinsics(Structure):
    _fields_ = [("rotation", c_float * 9),
                ("translation", c_float * 3)]

def check_error(e):
    if(e):
        msg = "RealSense error calling {}({}):\n    {}".format(
            realsense.rs_get_failed_function(e).decode('utf-8', 'strict'),
            realsense.rs_get_failed_args(e).decode('utf-8', 'strict'),
            realsense.rs_get_error_message(e).decode('utf-8', 'strict'))
        realsense.rs_free_error(e)
        raise Exception(msg)

class Context:
    def __init__(self):
        e = c_void_p(0)
        self.handle = realsense.rs_create_context(3, byref(e))
        check_error(e)

    def free(self):
        realsense.rs_delete_context(self.handle, c_void_p(0))

    ## determine number of connected devices
    # @return  the count of devices
    def get_device_count(self):
        e = c_void_p(0)
        r = realsense.rs_get_device_count(self.handle, byref(e))
        check_error(e)
        return r

    ## retrieve connected device by index
    # @param index  the zero based index of device to retrieve
    # @return       the requested device
    def get_device(self, index):
        e = c_void_p(0)
        r = realsense.rs_get_device(self.handle, index, byref(e))
        check_error(e)
        return Device(r)

class Device:
    def __init__(self, handle):
        self.handle = handle

    ## retrieve a human readable device model string
    # @return  the model string, such as "Intel RealSense F200" or "Intel RealSense R200"
    def get_name(self):
        e = c_void_p(0)
        r = realsense.rs_get_device_name(self.handle, byref(e))
        check_error(e)
        return r.decode('utf-8', 'strict')

    ## retrieve the unique serial number of the device
    # @return  the serial number, in a format specific to the device model
    def get_serial(self):
        e = c_void_p(0)
        r = realsense.rs_get_device_serial(self.handle, byref(e))
        check_error(e)
        return r.decode('utf-8', 'strict')

    ## retrieve the version of the firmware currently installed on the device
    # @return  firmware version string, in a format is specific to device model
    def get_firmware_version(self):
        e = c_void_p(0)
        r = realsense.rs_get_device_firmware_version(self.handle, byref(e))
        check_error(e)
        return r.decode('utf-8', 'strict')

    ## retrieve extrinsic transformation between the viewpoints of two different streams
    # @param from_stream  stream whose coordinate space we will transform from
    # @param to_stream    stream whose coordinate space we will transform to
    # @return             the transformation between the two streams
    def get_extrinsics(self, from_stream, to_stream):
        e = c_void_p(0)
        extrin = Extrinsics()
        realsense.rs_get_device_extrinsics(self.handle, from_stream, to_stream, byref(extrin), byref(e))
        check_error(e)
        return (extrin)

    ## retrieve mapping between the units of the depth image and meters
    # @return  depth in meters corresponding to a depth value of 1
    def get_depth_scale(self):
        e = c_void_p(0)
        r = realsense.rs_get_device_depth_scale(self.handle, byref(e))
        check_error(e)
        return r

    ## determine if the device allows a specific option to be queried and set
    # @param option  the option to check for support
    # @return        true if the option can be queried and set
    def supports_option(self, option):
        e = c_void_p(0)
        r = realsense.rs_device_supports_option(self.handle, option, byref(e))
        check_error(e)
        return r

    ## determine the number of streaming modes available for a given stream
    # @param stream  the stream whose modes will be enumerated
    # @return        the count of available modes
    def get_stream_mode_count(self, stream):
        e = c_void_p(0)
        r = realsense.rs_get_stream_mode_count(self.handle, stream, byref(e))
        check_error(e)
        return r

    ## determine the properties of a specific streaming mode
    # @param stream  the stream whose mode will be queried
    # @param index   the zero based index of the streaming mode
    # @return        the width of a frame image in pixels
    # @return        the height of a frame image in pixels
    # @return        the pixel format of a frame image
    # @return        the number of frames which will be streamed per second
    def get_stream_mode(self, stream, index):
        e = c_void_p(0)
        width = c_int()
        height = c_int()
        format = c_int()
        framerate = c_int()
        realsense.rs_get_stream_mode(self.handle, stream, index, byref(width), byref(height), byref(format), byref(framerate), byref(e))
        check_error(e)
        return (width.value, height.value, format.value, framerate.value)

    ## enable a specific stream and request specific properties
    # @param stream     the stream to enable
    # @param width      the desired width of a frame image in pixels, or 0 if any width is acceptable
    # @param height     the desired height of a frame image in pixels, or 0 if any height is acceptable
    # @param format     the pixel format of a frame image, or ANY if any format is acceptable
    # @param framerate  the number of frames which will be streamed per second, or 0 if any framerate is acceptable
    def enable_stream(self, stream, width, height, format, framerate):
        e = c_void_p(0)
        realsense.rs_enable_stream(self.handle, stream, width, height, format, framerate, byref(e))
        check_error(e)

    ## enable a specific stream and request properties using a preset
    # @param stream  the stream to enable
    # @param preset  the preset to use to enable the stream
    def enable_stream(self, stream, preset):
        e = c_void_p(0)
        realsense.rs_enable_stream_preset(self.handle, stream, preset, byref(e))
        check_error(e)

    ## disable a specific stream
    # @param stream  the stream to disable
    def disable_stream(self, stream):
        e = c_void_p(0)
        realsense.rs_disable_stream(self.handle, stream, byref(e))
        check_error(e)

    ## determine if a specific stream is enabled
    # @param stream  the stream to check
    # @return        true if the stream is currently enabled
    def is_stream_enabled(self, stream):
        e = c_void_p(0)
        r = realsense.rs_is_stream_enabled(self.handle, stream, byref(e))
        check_error(e)
        return r

    ## retrieve intrinsic camera parameters for a specific stream
    # @param stream  the stream whose parameters to retrieve
    # @return        the intrinsic parameters of the stream
    def get_stream_intrinsics(self, stream):
        e = c_void_p(0)
        intrin = Intrinsics()
        realsense.rs_get_stream_intrinsics(self.handle, stream, byref(intrin), byref(e))
        check_error(e)
        return (intrin)

    ## retrieve the pixel format for a specific stream
    # @param stream  the stream whose format to retrieve
    # @return        the pixel format of the stream
    def get_stream_format(self, stream):
        e = c_void_p(0)
        r = realsense.rs_get_stream_format(self.handle, stream, byref(e))
        check_error(e)
        return r

    ## retrieve the framerate for a specific stream
    # @param stream  the stream whose framerate to retrieve
    # @return        the framerate of the stream, in frames per second
    def get_stream_framerate(self, stream):
        e = c_void_p(0)
        r = realsense.rs_get_stream_framerate(self.handle, stream, byref(e))
        check_error(e)
        return r

    ## begin streaming on all enabled streams for this device
    def start(self):
        e = c_void_p(0)
        realsense.rs_start_device(self.handle, byref(e))
        check_error(e)

    ## end streaming on all streams for this device
    def stop(self):
        e = c_void_p(0)
        realsense.rs_stop_device(self.handle, byref(e))
        check_error(e)

    ## determine if the device is currently streaming
    # @return  true if the device is currently streaming
    def is_streaming(self):
        e = c_void_p(0)
        r = realsense.rs_is_device_streaming(self.handle, byref(e))
        check_error(e)
        return r

    ## set the value of a specific device option
    # @param option  the option whose value to set
    # @param value   the desired value to set
    def set_option(self, option, value):
        e = c_void_p(0)
        realsense.rs_set_device_option(self.handle, option, value, byref(e))
        check_error(e)

    ## query the current value of a specific device option
    # @param option  the option whose value to retrieve
    # @return        the current value of the option
    def get_option(self, option):
        e = c_void_p(0)
        r = realsense.rs_get_device_option(self.handle, option, byref(e))
        check_error(e)
        return r

    ## block until new frames are available
    def wait_for_frames(self):
        e = c_void_p(0)
        realsense.rs_wait_for_frames(self.handle, byref(e))
        check_error(e)

    ## retrieve the time at which the latest frame on a stream was captured
    # @param stream  the stream whose latest frame we are interested in
    # @return        the timestamp of the frame, in milliseconds since the device was started
    def get_frame_timestamp(self, stream):
        e = c_void_p(0)
        r = realsense.rs_get_frame_timestamp(self.handle, stream, byref(e))
        check_error(e)
        return r

    ## retrieve the contents of the latest frame on a stream
    # @param stream  the stream whose latest frame we are interested in
    # @return        the pointer to the start of the frame data
    def get_frame_data(self, stream):
        e = c_void_p(0)
        r = realsense.rs_get_frame_data(self.handle, stream, byref(e))
        check_error(e)
        return r

def stream_to_string(stream):
    return realsense.rs_stream_to_string(stream).decode('utf-8', 'strict')

def format_to_string(format):
    return realsense.rs_format_to_string(format).decode('utf-8', 'strict')

def preset_to_string(preset):
    return realsense.rs_preset_to_string(preset).decode('utf-8', 'strict')

def distortion_to_string(distortion):
    return realsense.rs_distortion_to_string(distortion).decode('utf-8', 'strict')

def option_to_string(option):
    return realsense.rs_option_to_string(option).decode('utf-8', 'strict')
