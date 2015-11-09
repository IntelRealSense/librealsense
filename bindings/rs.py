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


STREAM_DEPTH = 0 # Native stream of depth data produced by RealSense device
STREAM_COLOR = 1 # Native stream of color data captured by RealSense device
STREAM_INFRARED = 2 # Native stream of infrared data captured by RealSense device
STREAM_INFRARED2 = 3 # Native stream of infrared data captured from a second viewpoint by RealSense device
STREAM_RECTIFIED_COLOR = 4 # Synthetic stream containing undistorted color data with no extrinsic rotation from the depth stream
STREAM_COLOR_ALIGNED_TO_DEPTH = 5 # Synthetic stream containing color data but sharing intrinsics of depth stream
STREAM_DEPTH_ALIGNED_TO_COLOR = 6 # Synthetic stream containing depth data but sharing intrinsics of color stream
STREAM_DEPTH_ALIGNED_TO_RECTIFIED_COLOR = 7 # Synthetic stream containing depth data but sharing intrinsics of rectified color stream
STREAM_COUNT = 8
FORMAT_ANY = 0
FORMAT_Z16 = 1
FORMAT_YUYV = 2
FORMAT_RGB8 = 3
FORMAT_BGR8 = 4
FORMAT_RGBA8 = 5
FORMAT_BGRA8 = 6
FORMAT_Y8 = 7
FORMAT_Y16 = 8
FORMAT_RAW10 = 9 # Four 10-bit luminance values encoded into a 5-byte macropixel
FORMAT_COUNT = 10
PRESET_BEST_QUALITY = 0
PRESET_LARGEST_IMAGE = 1
PRESET_HIGHEST_FRAMERATE = 2
PRESET_COUNT = 3
DISTORTION_NONE = 0 # Rectilinear images, no distortion compensation required
DISTORTION_MODIFIED_BROWN_CONRADY = 1 # Equivalent to Brown-Conrady distortion, except that tangential distortion is applied to radially distorted points
DISTORTION_INVERSE_BROWN_CONRADY = 2 # Equivalent to Brown-Conrady distortion, except undistorts image instead of distorting it
DISTORTION_COUNT = 3
OPTION_COLOR_BACKLIGHT_COMPENSATION = 0
OPTION_COLOR_BRIGHTNESS = 1
OPTION_COLOR_CONTRAST = 2
OPTION_COLOR_EXPOSURE = 3
OPTION_COLOR_GAIN = 4
OPTION_COLOR_GAMMA = 5
OPTION_COLOR_HUE = 6
OPTION_COLOR_SATURATION = 7
OPTION_COLOR_SHARPNESS = 8
OPTION_COLOR_WHITE_BALANCE = 9
OPTION_F200_LASER_POWER = 10 # 0 - 15
OPTION_F200_ACCURACY = 11 # 0 - 3
OPTION_F200_MOTION_RANGE = 12 # 0 - 100
OPTION_F200_FILTER_OPTION = 13 # 0 - 7
OPTION_F200_CONFIDENCE_THRESHOLD = 14 # 0 - 15
OPTION_F200_DYNAMIC_FPS = 15 # {2, 5, 15, 30, 60}
OPTION_R200_LR_AUTO_EXPOSURE_ENABLED = 16 # {0, 1}
OPTION_R200_LR_GAIN = 17 # 100 - 1600 (Units of 0.01)
OPTION_R200_LR_EXPOSURE = 18 # > 0 (Units of 0.1 ms)
OPTION_R200_EMITTER_ENABLED = 19 # {0, 1}
OPTION_R200_DEPTH_CONTROL_PRESET = 20 # 0 - 5, 0 is default, 1-5 is low to high outlier rejection
OPTION_R200_DEPTH_UNITS = 21 # micrometers per increment in integer depth values, 1000 is default (mm scale)
OPTION_R200_DEPTH_CLAMP_MIN = 22 # 0 - USHORT_MAX
OPTION_R200_DEPTH_CLAMP_MAX = 23 # 0 - USHORT_MAX
OPTION_R200_DISPARITY_MODE_ENABLED = 24 # {0, 1}
OPTION_R200_DISPARITY_MULTIPLIER = 25
OPTION_R200_DISPARITY_SHIFT = 26
OPTION_COUNT = 27

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

    def get_device_count(self):
        e = c_void_p(0)
        r = realsense.rs_get_device_count(self.handle, byref(e))
        check_error(e)
        return r

    def get_device(self, index):
        e = c_void_p(0)
        r = realsense.rs_get_device(self.handle, index, byref(e))
        check_error(e)
        return Device(r)

class Device:
    def __init__(self, handle):
        self.handle = handle

    def get_name(self):
        e = c_void_p(0)
        r = realsense.rs_get_device_name(self.handle, byref(e))
        check_error(e)
        return r.decode('utf-8', 'strict')

    def get_serial(self):
        e = c_void_p(0)
        r = realsense.rs_get_device_serial(self.handle, byref(e))
        check_error(e)
        return r.decode('utf-8', 'strict')

    def get_firmware_version(self):
        e = c_void_p(0)
        r = realsense.rs_get_device_firmware_version(self.handle, byref(e))
        check_error(e)
        return r.decode('utf-8', 'strict')

    def get_extrinsics(self, from_stream, to_stream):
        e = c_void_p(0)
        extrin = Extrinsics()
        realsense.rs_get_device_extrinsics(self.handle, from_stream, to_stream, byref(extrin), byref(e))
        check_error(e)
        return (extrin)

    def get_depth_scale(self):
        e = c_void_p(0)
        r = realsense.rs_get_device_depth_scale(self.handle, byref(e))
        check_error(e)
        return r

    def supports_option(self, option):
        e = c_void_p(0)
        r = realsense.rs_device_supports_option(self.handle, option, byref(e))
        check_error(e)
        return r

    def get_stream_mode_count(self, stream):
        e = c_void_p(0)
        r = realsense.rs_get_stream_mode_count(self.handle, stream, byref(e))
        check_error(e)
        return r

    def get_stream_mode(self, stream, index):
        e = c_void_p(0)
        width = c_int()
        height = c_int()
        format = c_int()
        framerate = c_int()
        realsense.rs_get_stream_mode(self.handle, stream, index, byref(width), byref(height), byref(format), byref(framerate), byref(e))
        check_error(e)
        return (width.value, height.value, format.value, framerate.value)

    def enable_stream(self, stream, width, height, format, framerate):
        e = c_void_p(0)
        realsense.rs_enable_stream(self.handle, stream, width, height, format, framerate, byref(e))
        check_error(e)

    def enable_stream(self, stream, preset):
        e = c_void_p(0)
        realsense.rs_enable_stream_preset(self.handle, stream, preset, byref(e))
        check_error(e)

    def disable_stream(self, stream):
        e = c_void_p(0)
        realsense.rs_disable_stream(self.handle, stream, byref(e))
        check_error(e)

    def is_stream_enabled(self, stream):
        e = c_void_p(0)
        r = realsense.rs_is_stream_enabled(self.handle, stream, byref(e))
        check_error(e)
        return r

    def get_stream_intrinsics(self, stream):
        e = c_void_p(0)
        intrin = Intrinsics()
        realsense.rs_get_stream_intrinsics(self.handle, stream, byref(intrin), byref(e))
        check_error(e)
        return (intrin)

    def get_stream_format(self, stream):
        e = c_void_p(0)
        r = realsense.rs_get_stream_format(self.handle, stream, byref(e))
        check_error(e)
        return r

    def get_stream_framerate(self, stream):
        e = c_void_p(0)
        r = realsense.rs_get_stream_framerate(self.handle, stream, byref(e))
        check_error(e)
        return r

    def start(self):
        e = c_void_p(0)
        realsense.rs_start_device(self.handle, byref(e))
        check_error(e)

    def stop(self):
        e = c_void_p(0)
        realsense.rs_stop_device(self.handle, byref(e))
        check_error(e)

    def is_streaming(self):
        e = c_void_p(0)
        r = realsense.rs_is_device_streaming(self.handle, byref(e))
        check_error(e)
        return r

    def set_option(self, option, value):
        e = c_void_p(0)
        realsense.rs_set_device_option(self.handle, option, value, byref(e))
        check_error(e)

    def get_option(self, option):
        e = c_void_p(0)
        r = realsense.rs_get_device_option(self.handle, option, byref(e))
        check_error(e)
        return r

    def wait_for_frames(self):
        e = c_void_p(0)
        realsense.rs_wait_for_frames(self.handle, byref(e))
        check_error(e)

    def get_frame_timestamp(self, stream):
        e = c_void_p(0)
        r = realsense.rs_get_frame_timestamp(self.handle, stream, byref(e))
        check_error(e)
        return r

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

