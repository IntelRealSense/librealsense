#ifndef LIBREALSENSE_RS2_LEGACY_ADAPTOR_CONVERSIONS_HPP
#define LIBREALSENSE_RS2_LEGACY_ADAPTOR_CONVERSIONS_HPP

#include <librealsense2/rs.hpp> // Librealsense Cross-Platform API
#include <librealsense/rs.hpp> // Legacy Librealsense for wrapping

/** Conversion functions to help bridge the two APIs **/

// Convert legacy format enum to modern enum
rs2_format fmt_conv(rs::format fmt) {
    switch (fmt) {
    case rs::format::z16: return RS2_FORMAT_Z16;
    case rs::format::disparity16: return RS2_FORMAT_DISPARITY16;
    case rs::format::xyz32f: return RS2_FORMAT_XYZ32F;
    case rs::format::yuyv: return RS2_FORMAT_YUYV;
    case rs::format::rgb8: return RS2_FORMAT_RGB8;
    case rs::format::bgr8: return RS2_FORMAT_BGR8;
    case rs::format::rgba8: return RS2_FORMAT_RGBA8;
    case rs::format::bgra8: return RS2_FORMAT_BGRA8;
    case rs::format::y8: return RS2_FORMAT_Y8;
    case rs::format::y16: return RS2_FORMAT_Y16;
    case rs::format::raw10: return RS2_FORMAT_RAW10;
    case rs::format::raw16: return RS2_FORMAT_RAW16;
    case rs::format::raw8: return RS2_FORMAT_RAW8;
    default: return RS2_FORMAT_ANY;
    }
}

// Convert modern format enum to legacy enum
rs::format fmt_iconv(rs2_format fmt) {
    switch (fmt) {
    case RS2_FORMAT_Z16: return rs::format::z16;
    case RS2_FORMAT_DISPARITY16: return rs::format::disparity16;
    case RS2_FORMAT_XYZ32F: return rs::format::xyz32f;
    case RS2_FORMAT_YUYV: return rs::format::yuyv;
    case RS2_FORMAT_RGB8: return rs::format::rgb8;
    case RS2_FORMAT_BGR8: return rs::format::bgr8;
    case RS2_FORMAT_RGBA8: return rs::format::rgba8;
    case RS2_FORMAT_BGRA8: return rs::format::bgra8;
    case RS2_FORMAT_Y8: return rs::format::y8;
    case RS2_FORMAT_Y16: return rs::format::y16;
    case RS2_FORMAT_RAW10: return rs::format::raw10;
    case RS2_FORMAT_RAW16: return rs::format::raw16;
    case RS2_FORMAT_RAW8: return rs::format::raw8;
    default: return rs::format::any;
    }
}

// Legacy API only exposes this value from live frames,
// but luckily it is identical for all streams with the same format.
int fmt_to_bpp(rs::format fmt) {
    switch (fmt) {
    case rs::format::y8: case rs::format::raw8:
        return 1;
    case rs::format::z16: case rs::format::disparity16:
    case rs::format::yuyv: case rs::format::y16:
    case rs::format::raw16: case rs::format::raw10:
        return 2;
    case rs::format::rgb8: case rs::format::bgr8:
        return 3;
    case rs::format::rgba8: case rs::format::bgra8:
        return 4;
    case rs::format::xyz32f:
        return 12;
    default: assert(false); return 0;
    }
}

// Convert legacy distortion enum to modern enum
rs2_distortion distortion_conv(rs::distortion distortion) {
    switch (distortion) {
    case rs::distortion::modified_brown_conrady: return RS2_DISTORTION_MODIFIED_BROWN_CONRADY;
    case rs::distortion::inverse_brown_conrady: return RS2_DISTORTION_INVERSE_BROWN_CONRADY;
    case rs::distortion::distortion_ftheta: return RS2_DISTORTION_FTHETA;
    default: /* case rs::distortion::none: */ return RS2_DISTORTION_NONE;
    }
}

// Convert legacy camera info enum to modern enum
rs2_camera_info info_conv(rs::camera_info info) {
    switch (info) {
    case rs::camera_info::device_name: return RS2_CAMERA_INFO_NAME;
    case rs::camera_info::serial_number: return RS2_CAMERA_INFO_SERIAL_NUMBER;
    case rs::camera_info::camera_firmware_version: return RS2_CAMERA_INFO_FIRMWARE_VERSION;
    case rs::camera_info::camera_type: return RS2_CAMERA_INFO_PRODUCT_LINE;
    case rs::camera_info::oem_id: return RS2_CAMERA_INFO_PRODUCT_ID;
    default: return RS2_CAMERA_INFO_COUNT; // Legacy API has a lot of specific infos not in modern API.
    }
}

// Convert legacy stream type enum to modern enum
rs2_stream stream_conv(rs::stream stream) {
    switch (stream) {
    case rs::stream::depth: return RS2_STREAM_DEPTH;
    case rs::stream::color: return RS2_STREAM_COLOR;
        //case rs::stream::infrared: return RS2_STREAM_INFRARED;
        //case rs::stream::infrared2: return RS2_STREAM_INFRARED;
    case rs::stream::fisheye: return RS2_STREAM_FISHEYE;
    default: return RS2_STREAM_ANY;
    }
}

// Convert modern stream type enum to legacy enum
rs::stream stream_iconv(rs2_stream stream) {
    switch (stream) {
    case RS2_STREAM_DEPTH: return rs::stream::depth;
    case RS2_STREAM_COLOR: return rs::stream::color;
        //case RS2_STREAM_INFRARED: return rs::stream::infrared;
    case RS2_STREAM_FISHEYE: return rs::stream::fisheye;
    default: return rs::stream::depth;
    }
}

// Convert legacy options enum to {stream, modern enum, readonly}
auto option_conv(rs::option option) -> std::tuple<rs2_stream, rs2_option, bool> {
    switch (option) {
    case rs::option::color_backlight_compensation: return std::make_tuple(RS2_STREAM_COLOR, RS2_OPTION_BACKLIGHT_COMPENSATION, true);
    case rs::option::color_brightness: return std::make_tuple(RS2_STREAM_COLOR, RS2_OPTION_BRIGHTNESS, true);
    case rs::option::color_contrast: return std::make_tuple(RS2_STREAM_COLOR, RS2_OPTION_CONTRAST, true);
    case rs::option::color_exposure: return std::make_tuple(RS2_STREAM_COLOR, RS2_OPTION_EXPOSURE, true);
    case rs::option::color_gain: return std::make_tuple(RS2_STREAM_COLOR, RS2_OPTION_GAIN, true);
    case rs::option::color_gamma: return std::make_tuple(RS2_STREAM_COLOR, RS2_OPTION_GAMMA, true);
    case rs::option::color_hue: return std::make_tuple(RS2_STREAM_COLOR, RS2_OPTION_HUE, true);
    case rs::option::color_saturation: return std::make_tuple(RS2_STREAM_COLOR, RS2_OPTION_SATURATION, true);
    case rs::option::color_sharpness: return std::make_tuple(RS2_STREAM_COLOR, RS2_OPTION_SHARPNESS, true);
    case rs::option::color_white_balance: return std::make_tuple(RS2_STREAM_COLOR, RS2_OPTION_WHITE_BALANCE, true);
    case rs::option::color_enable_auto_exposure: return std::make_tuple(RS2_STREAM_COLOR, RS2_OPTION_ENABLE_AUTO_EXPOSURE, true);
    case rs::option::color_enable_auto_white_balance: return std::make_tuple(RS2_STREAM_COLOR, RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE, true);
    case rs::option::f200_laser_power: return std::make_tuple(RS2_STREAM_DEPTH, RS2_OPTION_LASER_POWER, true);
    case rs::option::f200_accuracy: return std::make_tuple(RS2_STREAM_DEPTH, RS2_OPTION_ACCURACY, true);
    case rs::option::f200_motion_range: return std::make_tuple(RS2_STREAM_DEPTH, RS2_OPTION_MOTION_RANGE, true);
    case rs::option::f200_filter_option: return std::make_tuple(RS2_STREAM_DEPTH, RS2_OPTION_FILTER_OPTION, true);
    case rs::option::f200_confidence_threshold: return std::make_tuple(RS2_STREAM_DEPTH, RS2_OPTION_CONFIDENCE_THRESHOLD, true);
    case rs::option::r200_lr_auto_exposure_enabled: return std::make_tuple(RS2_STREAM_DEPTH, RS2_OPTION_ENABLE_AUTO_EXPOSURE, true);
    case rs::option::r200_lr_gain: return std::make_tuple(RS2_STREAM_DEPTH, RS2_OPTION_GAIN, true);
    case rs::option::r200_lr_exposure: return std::make_tuple(RS2_STREAM_DEPTH, RS2_OPTION_EXPOSURE, true);
    case rs::option::r200_emitter_enabled: return std::make_tuple(RS2_STREAM_DEPTH, RS2_OPTION_EMITTER_ENABLED, true);
    case rs::option::r200_depth_units: return std::make_tuple(RS2_STREAM_DEPTH, RS2_OPTION_DEPTH_UNITS, true);
    case rs::option::r200_depth_clamp_min: return std::make_tuple(RS2_STREAM_DEPTH, RS2_OPTION_MIN_DISTANCE, true);
    case rs::option::r200_depth_clamp_max: return std::make_tuple(RS2_STREAM_DEPTH, RS2_OPTION_MAX_DISTANCE, true);
    case rs::option::fisheye_exposure: return std::make_tuple(RS2_STREAM_FISHEYE, RS2_OPTION_EXPOSURE, true);
    case rs::option::fisheye_gain: return std::make_tuple(RS2_STREAM_FISHEYE, RS2_OPTION_GAIN, true);
        //case rs::option::fisheye_external_trigger: return std::make_tuple(RS2_STREAM_FISHEYE, RS2_OPTION_INTER_CAM_SYNC_MODE, true);
    case rs::option::fisheye_color_auto_exposure: return std::make_tuple(RS2_STREAM_FISHEYE, RS2_OPTION_ENABLE_AUTO_EXPOSURE, true);
    case rs::option::fisheye_color_auto_exposure_mode: return std::make_tuple(RS2_STREAM_FISHEYE, RS2_OPTION_AUTO_EXPOSURE_MODE, true);
    case rs::option::fisheye_color_auto_exposure_rate: return std::make_tuple(RS2_STREAM_FISHEYE, RS2_OPTION_POWER_LINE_FREQUENCY, true);
    case rs::option::frames_queue_size: return std::make_tuple(RS2_STREAM_DEPTH, RS2_OPTION_FRAMES_QUEUE_SIZE, true);
    case rs::option::total_frame_drops: return std::make_tuple(RS2_STREAM_DEPTH, RS2_OPTION_TOTAL_FRAME_DROPS, false);
    default: return std::make_tuple(RS2_STREAM_ANY, RS2_OPTION_COUNT, false);
    }
}

#endif