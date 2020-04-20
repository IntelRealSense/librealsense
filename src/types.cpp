// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "types.h"

#include <algorithm>
#include <iomanip>
#include <numeric>
#include <fstream>
#include <cmath>

#include "core/streaming.h"
#include "../include/librealsense2/hpp/rs_processing.hpp"

#define STRCASE(T, X) case RS2_##T##_##X: {\
        static const std::string s##T##_##X##_str = make_less_screamy(#X);\
        return s##T##_##X##_str.c_str(); }

const double SQRT_DBL_EPSILON = sqrt(std::numeric_limits<double>::epsilon());

namespace librealsense
{
    std::string make_less_screamy(const char* str)
    {
        std::string res(str);

        bool first = true;
        for (auto i = 0; i < res.size(); i++)
        {
            if (res[i] != '_')
            {
                if (!first) res[i] = tolower(res[i]);
                first = false;
            }
            else
            {
                res[i] = ' ';
                first = true;
            }
        }

        return res;
    }

    recoverable_exception::recoverable_exception(const std::string& msg,
        rs2_exception_type exception_type) noexcept
        : librealsense_exception(msg, exception_type)
    {
        LOG_WARNING(msg);
    }

    bool file_exists(const char* filename)
    {
        std::ifstream f(filename);
        return f.good();
    }

    frame_holder::~frame_holder()
    {
        if (frame)
            frame->release();
    }

    frame_holder& frame_holder::operator=(frame_holder&& other)
    {
        if (frame)
            frame->release();
        frame = other.frame;
        other.frame = nullptr;
        return *this;
    }

    frame_holder frame_holder::clone() const
    {
        return frame_holder(*this);
    }

    frame_holder::frame_holder(const frame_holder& other)
        : frame(other.frame)
    {
        frame->acquire();
    }

    const char* get_string(rs2_exception_type value)
    {
#define CASE(X) STRCASE(EXCEPTION_TYPE, X)
        switch(value)
        {
            CASE(UNKNOWN)
            CASE(CAMERA_DISCONNECTED)
            CASE(BACKEND)
            CASE(INVALID_VALUE)
            CASE(WRONG_API_CALL_SEQUENCE)
            CASE(NOT_IMPLEMENTED)
            CASE(DEVICE_IN_RECOVERY_MODE)
            CASE(IO)
        default: assert(!is_valid(value)); return UNKNOWN_VALUE;
        }
#undef CASE
    }

    const char* get_string(rs2_stream value)
    {
#define CASE(X) STRCASE(STREAM, X)
        switch (value)
        {
            CASE(ANY)
            CASE(DEPTH)
            CASE(COLOR)
            CASE(INFRARED)
            CASE(FISHEYE)
            CASE(GYRO)
            CASE(ACCEL)
            CASE(GPIO)
            CASE(POSE)
            CASE(CONFIDENCE)
        default: assert(!is_valid(value)); return UNKNOWN_VALUE;
        }
#undef CASE
    }

    const char* get_string(rs2_sr300_visual_preset value)
    {
#define CASE(X) STRCASE(SR300_VISUAL_PRESET, X)
        switch (value)
        {
            CASE(SHORT_RANGE)
            CASE(LONG_RANGE)
            CASE(BACKGROUND_SEGMENTATION)
            CASE(GESTURE_RECOGNITION)
            CASE(OBJECT_SCANNING)
            CASE(FACE_ANALYTICS)
            CASE(FACE_LOGIN)
            CASE(GR_CURSOR)
            CASE(DEFAULT)
            CASE(MID_RANGE)
            CASE(IR_ONLY)
        default: assert(!is_valid(value)); return UNKNOWN_VALUE;
        }
#undef CASE
    }

    const char* get_string(rs2_sensor_mode value)
    {
#define CASE(X) STRCASE(SENSOR_MODE, X)
        switch (value)
        {
            CASE(XGA)
            CASE(VGA)
        default: assert(!is_valid(value)); return UNKNOWN_VALUE;
        }
#undef CASE
    }
    
    const char* get_string(rs2_ambient_light value)
    {
#define CASE(X) STRCASE(AMBIENT_LIGHT, X)
        switch (value)
        {
            CASE(NO_AMBIENT)
            CASE(LOW_AMBIENT)
        default: assert(!is_valid(value)); return UNKNOWN_VALUE;
        }
#undef CASE
    }

    const char* get_string(rs2_extension value)
    {
#define CASE(X) STRCASE(EXTENSION, X)
        switch (value)
        {
            CASE(UNKNOWN)
            CASE(DEBUG)
            CASE(INFO)
            CASE(OPTIONS)
            CASE(MOTION)
            CASE(VIDEO)
            CASE(ROI)
            CASE(DEPTH_SENSOR)
            CASE(VIDEO_FRAME)
            CASE(MOTION_FRAME)
            CASE(COMPOSITE_FRAME)
            CASE(POINTS)
            CASE(DEPTH_FRAME)
            CASE(ADVANCED_MODE)
            CASE(RECORD)
            CASE(VIDEO_PROFILE)
            CASE(PLAYBACK)
            CASE(DEPTH_STEREO_SENSOR)
            CASE(DISPARITY_FRAME)
            CASE(MOTION_PROFILE)
            CASE(POSE_FRAME)
            CASE(POSE_PROFILE)
            CASE(TM2)
            CASE(SOFTWARE_DEVICE)
            CASE(SOFTWARE_SENSOR)
            CASE(DECIMATION_FILTER)
            CASE(THRESHOLD_FILTER)
            CASE(DISPARITY_FILTER)
            CASE(SPATIAL_FILTER)
            CASE(TEMPORAL_FILTER)
            CASE(HOLE_FILLING_FILTER)
            CASE(ZERO_ORDER_FILTER)
            CASE(RECOMMENDED_FILTERS)
            CASE(POSE)
            CASE(POSE_SENSOR)
            CASE(WHEEL_ODOMETER)
            CASE(GLOBAL_TIMER)
            CASE(UPDATABLE)
            CASE(UPDATE_DEVICE)
            CASE(L500_DEPTH_SENSOR)
            CASE(TM2_SENSOR)
            CASE(AUTO_CALIBRATED_DEVICE)
            CASE(COLOR_SENSOR)
            CASE(MOTION_SENSOR)
            CASE(FISHEYE_SENSOR)
            CASE(DEPTH_HUFFMAN_DECODER)
            CASE(SERIALIZABLE)
        default: assert(!is_valid(value)); return UNKNOWN_VALUE;
        }
#undef CASE
    }

    const char* get_string(rs2_playback_status value)
    {
#define CASE(X) STRCASE(PLAYBACK_STATUS, X)
        switch (value)
        {
            CASE(UNKNOWN)
            CASE(STOPPED)
            CASE(PAUSED)
            CASE(PLAYING)
        default: assert(!is_valid(value)); return UNKNOWN_VALUE;
        }
#undef CASE
    }

    const char* get_string(rs2_log_severity value)
    {
#define CASE(X) STRCASE(LOG_SEVERITY, X)
        switch (value)
        {
            CASE(DEBUG)
            CASE(INFO)
            CASE(WARN)
            CASE(ERROR)
            CASE(FATAL)
            CASE(NONE)
        default: assert(!is_valid(value)); return UNKNOWN_VALUE;
        }
#undef CASE
    }

    const char* get_string(rs2_option value)
    {
#define CASE(X) STRCASE(OPTION, X)
        switch (value)
        {
            CASE(BACKLIGHT_COMPENSATION)
            CASE(BRIGHTNESS)
            CASE(CONTRAST)
            CASE(EXPOSURE)
            CASE(GAIN)
            CASE(GAMMA)
            CASE(HUE)
            CASE(SATURATION)
            CASE(SHARPNESS)
            CASE(WHITE_BALANCE)
            CASE(ENABLE_AUTO_EXPOSURE)
            CASE(ENABLE_AUTO_WHITE_BALANCE)
            CASE(LASER_POWER)
            CASE(ACCURACY)
            CASE(MOTION_RANGE)
            CASE(FILTER_OPTION)
            CASE(CONFIDENCE_THRESHOLD)
            CASE(FRAMES_QUEUE_SIZE)
            CASE(VISUAL_PRESET)
            CASE(TOTAL_FRAME_DROPS)
            CASE(EMITTER_ENABLED)
            CASE(AUTO_EXPOSURE_MODE)
            CASE(POWER_LINE_FREQUENCY)
            CASE(ASIC_TEMPERATURE)
            CASE(ERROR_POLLING_ENABLED)
            CASE(PROJECTOR_TEMPERATURE)
            CASE(OUTPUT_TRIGGER_ENABLED)
            CASE(MOTION_MODULE_TEMPERATURE)
            CASE(DEPTH_UNITS)
            CASE(ENABLE_MOTION_CORRECTION)
            CASE(AUTO_EXPOSURE_PRIORITY)
            CASE(HISTOGRAM_EQUALIZATION_ENABLED)
            CASE(MIN_DISTANCE)
            CASE(MAX_DISTANCE)
            CASE(COLOR_SCHEME)
            CASE(TEXTURE_SOURCE)
            CASE(FILTER_MAGNITUDE)
            CASE(FILTER_SMOOTH_ALPHA)
            CASE(FILTER_SMOOTH_DELTA)
            CASE(STEREO_BASELINE)
            CASE(HOLES_FILL)
            CASE(AUTO_EXPOSURE_CONVERGE_STEP)
            CASE(INTER_CAM_SYNC_MODE)
            CASE(STREAM_FILTER)
            CASE(STREAM_FORMAT_FILTER)
            CASE(STREAM_INDEX_FILTER)
            CASE(EMITTER_ON_OFF)
            CASE(ZERO_ORDER_POINT_X)
            CASE(ZERO_ORDER_POINT_Y)
            CASE(LLD_TEMPERATURE)
            CASE(MC_TEMPERATURE)
            CASE(MA_TEMPERATURE)
            CASE(APD_TEMPERATURE)
            CASE(HARDWARE_PRESET)
            CASE(GLOBAL_TIME_ENABLED)
            CASE(ENABLE_MAPPING)
            CASE(ENABLE_RELOCALIZATION)
            CASE(ENABLE_POSE_JUMPING)
            CASE(ENABLE_DYNAMIC_CALIBRATION)
            CASE(DEPTH_OFFSET)
            CASE(LED_POWER)
            CASE(ZERO_ORDER_ENABLED)
            CASE(ENABLE_MAP_PRESERVATION)
            CASE(FREEFALL_DETECTION_ENABLED)
            CASE(AVALANCHE_PHOTO_DIODE)
            CASE(POST_PROCESSING_SHARPENING)
            CASE(PRE_PROCESSING_SHARPENING)
            CASE(NOISE_FILTERING)
            CASE(INVALIDATION_BYPASS)
            CASE(AMBIENT_LIGHT)
            CASE(SENSOR_MODE)
            CASE(EMITTER_ALWAYS_ON)
        default: assert(!is_valid(value)); return UNKNOWN_VALUE;
        }
#undef CASE
    }
   
    const char* get_string(rs2_format value)
    {
#define CASE(X) case RS2_FORMAT_##X: return #X;
        switch (value)
        {
            CASE(ANY)
            CASE(Z16)
            CASE(DISPARITY16)
            CASE(DISPARITY32)
            CASE(XYZ32F)
            CASE(YUYV)
            CASE(RGB8)
            CASE(BGR8)
            CASE(RGBA8)
            CASE(BGRA8)
            CASE(Y8)
            CASE(Y16)
            CASE(RAW10)
            CASE(RAW16)
            CASE(RAW8)
            CASE(UYVY)
            CASE(MOTION_RAW)
            CASE(MOTION_XYZ32F)
            CASE(GPIO_RAW)
            CASE(6DOF)
            CASE(Y10BPACK)
            CASE(DISTANCE)
            CASE(MJPEG)
            CASE(Y8I)
            CASE(Y12I)
            CASE(INZI)
            CASE(INVI)
            CASE(W10)
            CASE(Z16H)
        default: assert(!is_valid(value)); return UNKNOWN_VALUE;
        }
#undef CASE
    }

    const char* get_string(rs2_distortion value)
    {
#define CASE(X) STRCASE(DISTORTION, X)
        switch (value)
        {
            CASE(NONE)
            CASE(MODIFIED_BROWN_CONRADY)
            CASE(INVERSE_BROWN_CONRADY)
            CASE(FTHETA)
            CASE(BROWN_CONRADY)
            CASE(KANNALA_BRANDT4)
        default: assert(!is_valid(value)); return UNKNOWN_VALUE;
        }
#undef CASE
    }

    const char* get_string(rs2_camera_info value)
    {
#define CASE(X) STRCASE(CAMERA_INFO, X)
        switch (value)
        {
            CASE(NAME)
            CASE(SERIAL_NUMBER)
            CASE(FIRMWARE_VERSION)
            CASE(RECOMMENDED_FIRMWARE_VERSION)
            CASE(PHYSICAL_PORT)
            CASE(DEBUG_OP_CODE)
            CASE(ADVANCED_MODE)
            CASE(PRODUCT_ID)
            CASE(CAMERA_LOCKED)
            CASE(PRODUCT_LINE)
            CASE(USB_TYPE_DESCRIPTOR)
            CASE(ASIC_SERIAL_NUMBER)
            CASE(FIRMWARE_UPDATE_ID)
            CASE(IP_ADDRESS)
        default: assert(!is_valid(value)); return UNKNOWN_VALUE;
        }
#undef CASE
    }

    const char* get_string(rs2_frame_metadata_value value)
    {
#define CASE(X) STRCASE(FRAME_METADATA, X)
        switch (value)
        {
            CASE(FRAME_COUNTER)
            CASE(FRAME_TIMESTAMP)
            CASE(SENSOR_TIMESTAMP)
            CASE(ACTUAL_EXPOSURE)
            CASE(GAIN_LEVEL)
            CASE(AUTO_EXPOSURE)
            CASE(WHITE_BALANCE)
            CASE(TIME_OF_ARRIVAL)
            CASE(TEMPERATURE)
            CASE(BACKEND_TIMESTAMP)
            CASE(ACTUAL_FPS)
            CASE(FRAME_LASER_POWER)
            CASE(FRAME_LASER_POWER_MODE)
            CASE(EXPOSURE_PRIORITY)
            CASE(EXPOSURE_ROI_LEFT)
            CASE(EXPOSURE_ROI_RIGHT)
            CASE(EXPOSURE_ROI_TOP)
            CASE(EXPOSURE_ROI_BOTTOM)
            CASE(BRIGHTNESS)
            CASE(CONTRAST)
            CASE(SATURATION)
            CASE(SHARPNESS)
            CASE(AUTO_WHITE_BALANCE_TEMPERATURE)
            CASE(BACKLIGHT_COMPENSATION)
            CASE(GAMMA)
            CASE(HUE)
            CASE(MANUAL_WHITE_BALANCE)
            CASE(POWER_LINE_FREQUENCY)
            CASE(LOW_LIGHT_COMPENSATION)
            CASE(FRAME_EMITTER_MODE)
            CASE(FRAME_LED_POWER)
            CASE(RAW_FRAME_SIZE)
        default: assert(!is_valid(value)); return UNKNOWN_VALUE;
        }
#undef CASE
    }

    const char* get_string(rs2_timestamp_domain value)
    {
#define CASE(X) STRCASE(TIMESTAMP_DOMAIN, X)
        switch (value)
        {
            CASE(HARDWARE_CLOCK)
            CASE(SYSTEM_TIME)
            CASE(GLOBAL_TIME)
        default: assert(!is_valid(value)); return UNKNOWN_VALUE;
        }
#undef CASE
    }

    const char* get_string(rs2_notification_category value)
    {
#define CASE(X) STRCASE(NOTIFICATION_CATEGORY, X)
        switch (value)
        {
            CASE(FRAMES_TIMEOUT)
            CASE(FRAME_CORRUPTED)
            CASE(HARDWARE_ERROR)
            CASE(HARDWARE_EVENT)
            CASE(UNKNOWN_ERROR)
            CASE(FIRMWARE_UPDATE_RECOMMENDED)
            CASE(POSE_RELOCALIZATION)
        default: assert(!is_valid(value)); return UNKNOWN_VALUE;
        }
#undef CASE
    }
    const char* get_string(rs2_matchers value)
    {
#define CASE(X) STRCASE(MATCHER, X)
        switch (value)
        {
            CASE(DI)
            CASE(DI_C)
            CASE(DLR_C)
            CASE(DLR)
            CASE(DEFAULT)
        default: assert(!is_valid(value)); return UNKNOWN_VALUE;
        }

#undef CASE
    }

    const char* get_string(rs2_l500_visual_preset value)
    {
#define CASE(X) STRCASE(L500_VISUAL_PRESET, X)
        switch (value)
        {
            CASE(CUSTOM)
            CASE(DEFAULT)
            CASE(NO_AMBIENT)
            CASE(LOW_AMBIENT)
            CASE(MAX_RANGE)
            CASE(SHORT_RANGE)
        default: assert(!is_valid(value)); return UNKNOWN_VALUE;
        }
#undef CASE
    }
    std::string firmware_version::to_string() const
    {
        if (is_any) return "any";

        std::stringstream s;
        s << std::setfill('0') << std::setw(2) << m_major << "."
            << std::setfill('0') << std::setw(2) << m_minor << "."
            << std::setfill('0') << std::setw(2) << m_patch << "."
            << std::setfill('0') << std::setw(2) << m_build;
        return s.str();
    }

    std::vector<std::string> firmware_version::split(const std::string& str)
    {
        std::vector<std::string> result;
        auto e = str.end();
        auto i = str.begin();
        while (i != e) {
            i = find_if_not(i, e, [](char c) { return c == '.'; });
            if (i == e) break;
            auto j = find(i, e, '.');
            result.emplace_back(i, j);
            i = j;
        }
        return result;
    }

    int firmware_version::parse_part(const std::string& name, int part)
    {
        return atoi(split(name)[part].c_str());
    }

    /// Convert orientation angles stored in rodrigues conventions to rotation matrix
    /// for details: http://mesh.brown.edu/en193s08-2003/notes/en193s08-rots.pdf
    float3x3 calc_rotation_from_rodrigues_angles(const std::vector<double> rot)
    {
        assert(3 == rot.size());
        float3x3 rot_mat{};

        double theta = sqrt(std::inner_product(rot.begin(), rot.end(), rot.begin(), 0.0));
        double r1 = rot[0], r2 = rot[1], r3 = rot[2];
        if (theta <= SQRT_DBL_EPSILON) // identityMatrix
        {
            rot_mat(0, 0) = rot_mat(1, 1) = rot_mat(2, 2) = 1.0;
            rot_mat(0, 1) = rot_mat(0, 2) = rot_mat(1, 0) = rot_mat(1, 2) = rot_mat(2, 0) = rot_mat(2, 1) = 0.0;
        }
        else
        {
            r1 /= theta;
            r2 /= theta;
            r3 /= theta;

            double c = cos(theta);
            double s = sin(theta);
            double g = 1 - c;

            rot_mat(0, 0) = float(c + g * r1 * r1);
            rot_mat(0, 1) = float(g * r1 * r2 - s * r3);
            rot_mat(0, 2) = float(g * r1 * r3 + s * r2);
            rot_mat(1, 0) = float(g * r2 * r1 + s * r3);
            rot_mat(1, 1) = float(c + g * r2 * r2);
            rot_mat(1, 2) = float(g * r2 * r3 - s * r1);
            rot_mat(2, 0) = float(g * r3 * r1 - s * r2);
            rot_mat(2, 1) = float(g * r3 * r2 + s * r1);
            rot_mat(2, 2) = float(c + g * r3 * r3);
        }

        return rot_mat;
    }

    calibration_validator::calibration_validator(std::function<bool(rs2_stream, rs2_stream)> extrinsic_validator, std::function<bool(rs2_stream)> intrinsic_validator)
        : extrinsic_validator(extrinsic_validator), intrinsic_validator(intrinsic_validator)
    {
    }

    calibration_validator::calibration_validator()
        : extrinsic_validator([](rs2_stream, rs2_stream) { return true; }), intrinsic_validator([](rs2_stream) { return true; })
    {
    }

    bool calibration_validator::validate_extrinsics(rs2_stream from_stream, rs2_stream to_stream) const
    {
        return extrinsic_validator(from_stream, to_stream);
    }
    bool calibration_validator::validate_intrinsics(rs2_stream stream) const
    {
        return intrinsic_validator(stream);
    }


#define UPDC32(octet, crc) (crc_32_tab[((crc) ^ (octet)) & 0xff] ^ ((crc) >> 8))

    static const uint32_t crc_32_tab[] = { /* CRC polynomial 0xedb88320 */
        0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f,
        0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
        0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91, 0x1db71064, 0x6ab020f2,
        0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
        0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9,
        0xfa0f3d63, 0x8d080df5, 0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
        0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c,
        0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
        0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423,
        0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
        0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d, 0x76dc4190, 0x01db7106,
        0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
        0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d,
        0x91646c97, 0xe6635c01, 0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
        0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950,
        0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
        0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7,
        0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
        0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9, 0x5005713c, 0x270241aa,
        0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
        0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81,
        0xb7bd5c3b, 0xc0ba6cad, 0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
        0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84,
        0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
        0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb,
        0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
        0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5, 0xd6d6a3e8, 0xa1d1937e,
        0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
        0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55,
        0x316e8eef, 0x4669be79, 0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
        0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28,
        0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
        0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f,
        0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
        0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21, 0x86d3d2d4, 0xf1d4e242,
        0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
        0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69,
        0x616bffd3, 0x166ccf45, 0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
        0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc,
        0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
        0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693,
        0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
        0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
    };

    /// Calculate CRC code for arbitrary characters buffer
    uint32_t calc_crc32(const uint8_t *buf, size_t bufsize)
    {
        uint32_t oldcrc32 = 0xFFFFFFFF;
        for (; bufsize; --bufsize, ++buf)
            oldcrc32 = UPDC32(*buf, oldcrc32);
        return ~oldcrc32;
    }

    notifications_processor::notifications_processor()
        :_dispatcher(10), _callback(nullptr , [](rs2_notifications_callback*) {})
    {
    }

    notifications_processor::~notifications_processor()
    {
        _dispatcher.stop();
    }


    void notifications_processor::set_callback(notifications_callback_ptr callback)
    {

        _dispatcher.stop();

        std::lock_guard<std::mutex> lock(_callback_mutex);
        _callback = std::move(callback);
        _dispatcher.start();
    }
    notifications_callback_ptr notifications_processor::get_callback() const
    {
        return _callback;
    }

    void copy(void* dst, void const* src, size_t size)
    {
        auto from = reinterpret_cast<uint8_t const*>(src);
        std::copy(from, from + size, reinterpret_cast<uint8_t*>(dst));
    }

    void color_sensor::create_snapshot(std::shared_ptr<color_sensor>& snapshot) const
    {
        snapshot = std::make_shared<color_sensor_snapshot>();
    }
}
