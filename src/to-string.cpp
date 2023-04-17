// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#include "types.h"

#include <rsutils/string/from.h>


#define CASE( INDEX, STR ) INDEX, STR,
// For most enums, their values run 0..COUNT so we can use array indexing for O(1)
#define GET_STRING_IMPL( ENUM, COUNT, ... )                                                                            \
    static auto const ENUM##_strings = init_static_string_array( #ENUM, COUNT, __VA_ARGS__ 0, 0 );                     \
    std::string const & get_string( ENUM value )                                                                       \
    {                                                                                                                  \
        if( ! is_valid( value ) )                                                                                      \
            return unknown_value_str;                                                                                  \
        return ENUM##_strings[value];                                                                                  \
    }

// Special cases need a map, because their enums are not 0..COUNT
#define GET_STRING_IMPL_MAP( ENUM, COUNT, ... )                                                                        \
    static auto const ENUM##_strings = init_static_string_map( #ENUM, COUNT, __VA_ARGS__ 0, 0 );                       \
    std::string const & get_string( ENUM value )                                                                       \
    {                                                                                                                  \
        auto it = ENUM##_strings.find( value );                                                                        \
        if( it == ENUM##_strings.end() )                                                                               \
            return unknown_value_str;                                                                                  \
        return it->second;                                                                                             \
    }


namespace librealsense {


static std::string const unknown_value_str( UNKNOWN_VALUE );


#if 0
std::vector< std::string > init_static_string_array( char const * const enum_str, size_t count, ... )
{
    std::vector< std::string > arr( count );
    std::va_list args;
    va_start( args, count );
    size_t n_actual = 0;
    for( size_t i = 0; i < count; ++i )
    {
        int index = va_arg( args, int );
        char const * str = va_arg( args, char const * );
        if( ! str )
            break;
        if( index < 0 || index >= count )
            throw std::runtime_error( rsutils::string::from()
                                      << enum_str << " '" << str << "' index " << index << " out of bounds" );
        std::string & val = arr[index];
        if( ! val.empty() )
            throw std::runtime_error( rsutils::string::from() << enum_str << " index " << index
                                                              << " has already been given value '" << val << "'" );
        val = str;  // calls strlen, alloc
        ++n_actual;
    }
    va_end( args );
    if( n_actual != count )
        throw std::runtime_error( rsutils::string::from() << "invalid count for " << enum_str << ": expecting " << count
                                                          << " but got " << n_actual );
    return arr;
}

std::map< int, std::string > init_static_string_map( char const * const enum_str, size_t count, ... )
{
    std::map< int, std::string > map;
    std::va_list args;
    va_start( args, count );
    size_t n_actual = 0;
    for( size_t i = 0; i < count; ++i )
    {
        int index = va_arg( args, int );
        char const * str = va_arg( args, char const * );
        if( ! str )
            break;
        std::string & val = map[index];
        if( ! val.empty() )
            throw std::runtime_error( rsutils::string::from() << enum_str << " index " << index
                                                              << " has already been given value '" << val << "'" );
        val = str;  // calls strlen, alloc
        ++n_actual;
    }
    va_end( args );
    if( n_actual != count )
        throw std::runtime_error( rsutils::string::from() << "invalid count for " << enum_str << ": expecting " << count
                                                          << " but got " << n_actual );
    return map;
}
#endif


static void foreach_index_string( std::va_list args,
                                  char const * const enum_str,
                                  size_t const count,
                                  std::function< void( int, char const * ) > && code )
{
    size_t n_actual = 0;
    for( size_t i = 0; i < count; ++i )
    {
        int index = va_arg( args, int );
        char const * str = va_arg( args, char const * );
        if( ! str )
            break;
        code( index, str );
        ++n_actual;
    }
    if( n_actual != count )
        throw std::runtime_error( rsutils::string::from() << "invalid count for " << enum_str << ": expecting " << count
                                                          << " but got " << n_actual );
}


static std::vector< std::string > init_static_string_array( char const * const enum_str, size_t count, ... )
{
    std::vector< std::string > arr( count );
    std::va_list args;
    va_start( args, count );
    foreach_index_string( args, enum_str, count,
        [&]( int index, char const * str )
        {
            if( index < 0 || index >= count )
                throw std::runtime_error( rsutils::string::from()
                                          << enum_str << " '" << str << "' index " << index << " out of bounds" );
            std::string & val = arr[index];
            if( ! val.empty() )
                throw std::runtime_error( rsutils::string::from() << enum_str << " index " << index
                                                                  << " has already been given value '" << val << "'" );
            val = str;
        } );
    va_end( args );
    return arr;
}


static std::map< int, std::string > init_static_string_map( char const * const enum_str, size_t count, ... )
{
    std::map< int, std::string > map;
    std::va_list args;
    va_start( args, count );
    foreach_index_string( args, enum_str, count,
        [&]( int index, char const * str )
        {
            std::string & val = map[index];
            if( ! val.empty() )
                throw std::runtime_error( rsutils::string::from() << enum_str << " index " << index
                                                                  << " has already been given value '" << val << "'" );
            val = str;
        } );
    va_end( args );
    return map;
}


GET_STRING_IMPL( rs2_exception_type,
                 RS2_EXCEPTION_TYPE_COUNT,
                 CASE( RS2_EXCEPTION_TYPE_UNKNOWN, "Unknown" )
                 CASE( RS2_EXCEPTION_TYPE_CAMERA_DISCONNECTED, "Camera Disconnected" )
                 CASE( RS2_EXCEPTION_TYPE_BACKEND, "Backend" )
                 CASE( RS2_EXCEPTION_TYPE_INVALID_VALUE, "Invalid Value" )
                 CASE( RS2_EXCEPTION_TYPE_WRONG_API_CALL_SEQUENCE, "Wrong API Call Sequence" )
                 CASE( RS2_EXCEPTION_TYPE_NOT_IMPLEMENTED, "Not Implemented" )
                 CASE( RS2_EXCEPTION_TYPE_DEVICE_IN_RECOVERY_MODE, "Device In Recovery Mode" )
                 CASE( RS2_EXCEPTION_TYPE_IO, "IO" ) )

GET_STRING_IMPL( rs2_stream,
                 RS2_STREAM_COUNT,
                 CASE( RS2_STREAM_ANY, "Any" )
                 CASE( RS2_STREAM_DEPTH, "Depth" )
                 CASE( RS2_STREAM_COLOR, "Color" )
                 CASE( RS2_STREAM_INFRARED, "Infrared" )
                 CASE( RS2_STREAM_FISHEYE, "Fisheye" )
                 CASE( RS2_STREAM_GYRO, "Gyro" )
                 CASE( RS2_STREAM_ACCEL, "Accel" )
                 CASE( RS2_STREAM_GPIO, "GPIO" )
                 CASE( RS2_STREAM_POSE, "Pose" )
                 CASE( RS2_STREAM_CONFIDENCE, "Confidence" ) )

GET_STRING_IMPL( rs2_sr300_visual_preset,
                 RS2_SR300_VISUAL_PRESET_COUNT,
                 CASE( RS2_SR300_VISUAL_PRESET_SHORT_RANGE, "Short Range" )
                 CASE( RS2_SR300_VISUAL_PRESET_LONG_RANGE, "Long Range" )
                 CASE( RS2_SR300_VISUAL_PRESET_BACKGROUND_SEGMENTATION, "Background Segmentation" )
                 CASE( RS2_SR300_VISUAL_PRESET_GESTURE_RECOGNITION, "Gesture Recognition" )
                 CASE( RS2_SR300_VISUAL_PRESET_OBJECT_SCANNING, "Object Scanning" )
                 CASE( RS2_SR300_VISUAL_PRESET_FACE_ANALYTICS, "Face Analytics" )
                 CASE( RS2_SR300_VISUAL_PRESET_FACE_LOGIN, "Face Login" )
                 CASE( RS2_SR300_VISUAL_PRESET_GR_CURSOR, "GR Cursor" )
                 CASE( RS2_SR300_VISUAL_PRESET_DEFAULT, "Default" )
                 CASE( RS2_SR300_VISUAL_PRESET_MID_RANGE, "Mid Range" )
                 CASE( RS2_SR300_VISUAL_PRESET_IR_ONLY, "IR Only" ) )

GET_STRING_IMPL( rs2_rs400_visual_preset,
                 RS2_RS400_VISUAL_PRESET_COUNT,
                 CASE( RS2_RS400_VISUAL_PRESET_CUSTOM, "Custom" )
                 CASE( RS2_RS400_VISUAL_PRESET_HAND, "Hand" )
                 CASE( RS2_RS400_VISUAL_PRESET_HIGH_ACCURACY, "High Accuracy" )
                 CASE( RS2_RS400_VISUAL_PRESET_HIGH_DENSITY, "High Density" )
                 CASE( RS2_RS400_VISUAL_PRESET_MEDIUM_DENSITY, "Medium Density" )
                 CASE( RS2_RS400_VISUAL_PRESET_DEFAULT, "Default" )
                 CASE( RS2_RS400_VISUAL_PRESET_REMOVE_IR_PATTERN, "Remove IR Pattern" ) )

GET_STRING_IMPL( rs2_sensor_mode,
                 RS2_SENSOR_MODE_COUNT,
                 CASE( RS2_SENSOR_MODE_VGA, "VGA")
                 CASE( RS2_SENSOR_MODE_XGA, "XGA" )
                 CASE( RS2_SENSOR_MODE_QVGA, "QVGA" ) )

GET_STRING_IMPL( rs2_calibration_type,
                 RS2_CALIBRATION_TYPE_COUNT,
                 CASE( RS2_CALIBRATION_AUTO_DEPTH_TO_RGB, "Auto Depth To RGB" )
                 CASE( RS2_CALIBRATION_MANUAL_DEPTH_TO_RGB, "Manual Depth to RGB" )
                 CASE( RS2_CALIBRATION_THERMAL, "Thermal" ) )

GET_STRING_IMPL_MAP( rs2_calibration_status,
                     RS2_CALIBRATION_STATUS_COUNT,
                     CASE( RS2_CALIBRATION_TRIGGERED, "Triggered" )
                     CASE( RS2_CALIBRATION_SPECIAL_FRAME, "Special Frame" )
                     CASE( RS2_CALIBRATION_STARTED, "Started" )
                     CASE( RS2_CALIBRATION_NOT_NEEDED, "Not Needed" )
                     CASE( RS2_CALIBRATION_SUCCESSFUL, "Successful" )
                     CASE( RS2_CALIBRATION_BAD_CONDITIONS, "Bad Conditions" )
                     CASE( RS2_CALIBRATION_FAILED, "Failed" )
                     CASE( RS2_CALIBRATION_SCENE_INVALID, "Scene Invalid" )
                     CASE( RS2_CALIBRATION_BAD_RESULT, "Bad Result" )
                     CASE( RS2_CALIBRATION_RETRY, "Retry" ) )

GET_STRING_IMPL_MAP( rs2_ambient_light,
                     2,
                     CASE( RS2_AMBIENT_LIGHT_NO_AMBIENT, "No Ambient" )
                     CASE( RS2_AMBIENT_LIGHT_LOW_AMBIENT, "Low Ambient" ) )

GET_STRING_IMPL( rs2_digital_gain,
                 3,
                 CASE( RS2_DIGITAL_GAIN_AUTO, "Auto" )
                 CASE( RS2_DIGITAL_GAIN_HIGH, "High" )
                 CASE( RS2_DIGITAL_GAIN_LOW, "Low" ) )

GET_STRING_IMPL( rs2_host_perf_mode,
                 RS2_HOST_PERF_COUNT,
                 CASE( RS2_HOST_PERF_DEFAULT, "Default" )
                 CASE( RS2_HOST_PERF_LOW, "Low" )
                 CASE( RS2_HOST_PERF_HIGH, "High" ) )

GET_STRING_IMPL( rs2_emitter_frequency_mode,
                 RS2_EMITTER_FREQUENCY_COUNT,
                 CASE( RS2_EMITTER_FREQUENCY_57_KHZ, "57 Khz" )
                 CASE( RS2_EMITTER_FREQUENCY_91_KHZ, "91 Khz" ) )

GET_STRING_IMPL( rs2_depth_auto_exposure_mode,
                 RS2_DEPTH_AUTO_EXPOSURE_COUNT,
                 CASE( RS2_DEPTH_AUTO_EXPOSURE_REGULAR, "Regular" )
                 CASE( RS2_DEPTH_AUTO_EXPOSURE_ACCELERATED, "Accelerated" ) )

GET_STRING_IMPL( rs2_extension,
                 RS2_EXTENSION_COUNT,
                 CASE( RS2_EXTENSION_UNKNOWN, "Unknown" )
                 CASE( RS2_EXTENSION_DEBUG, "Debug" )
                 CASE( RS2_EXTENSION_INFO, "Info" )
                 CASE( RS2_EXTENSION_OPTIONS, "Options" )
                 CASE( RS2_EXTENSION_MOTION, "Motion" )
                 CASE( RS2_EXTENSION_VIDEO, "Video" )
                 CASE( RS2_EXTENSION_ROI, "ROI" )
                 CASE( RS2_EXTENSION_DEPTH_SENSOR, "Depth Sensor" )
                 CASE( RS2_EXTENSION_VIDEO_FRAME, "Video Frame" )
                 CASE( RS2_EXTENSION_MOTION_FRAME, "Motion Frame" )
                 CASE( RS2_EXTENSION_COMPOSITE_FRAME, "Composite Frame" )
                 CASE( RS2_EXTENSION_POINTS, "Points" )
                 CASE( RS2_EXTENSION_DEPTH_FRAME, "Depth Frame" )
                 CASE( RS2_EXTENSION_ADVANCED_MODE, "Advanced Mode" )
                 CASE( RS2_EXTENSION_RECORD, "Record" )
                 CASE( RS2_EXTENSION_VIDEO_PROFILE, "Video Profile" )
                 CASE( RS2_EXTENSION_PLAYBACK, "Playback" )
                 CASE( RS2_EXTENSION_DEPTH_STEREO_SENSOR, "Depth_Stereo Sensor" )
                 CASE( RS2_EXTENSION_DISPARITY_FRAME, "Disparity Frame" )
                 CASE( RS2_EXTENSION_MOTION_PROFILE, "Motion Profile" )
                 CASE( RS2_EXTENSION_POSE_FRAME, "Pose Frame" )
                 CASE( RS2_EXTENSION_POSE_PROFILE, "Pose Profile" )
                 CASE( RS2_EXTENSION_TM2, "TM2" )
                 CASE( RS2_EXTENSION_SOFTWARE_DEVICE, "Software Device" )
                 CASE( RS2_EXTENSION_SOFTWARE_SENSOR, "Software Sensor" )
                 CASE( RS2_EXTENSION_DECIMATION_FILTER, "Decimation Filter" )
                 CASE( RS2_EXTENSION_THRESHOLD_FILTER, "Threshold Filter" )
                 CASE( RS2_EXTENSION_DISPARITY_FILTER, "Disparity Filter" )
                 CASE( RS2_EXTENSION_SPATIAL_FILTER, "Spatial Filter" )
                 CASE( RS2_EXTENSION_TEMPORAL_FILTER, "Temporal Filter" )
                 CASE( RS2_EXTENSION_HOLE_FILLING_FILTER, "Hole Filling Filter" )
                 CASE( RS2_EXTENSION_ZERO_ORDER_FILTER, "Zero Order Filter" )
                 CASE( RS2_EXTENSION_RECOMMENDED_FILTERS, "Recommended Filters" )
                 CASE( RS2_EXTENSION_POSE, "Pose" )
                 CASE( RS2_EXTENSION_POSE_SENSOR, "Pose Sensor" )
                 CASE( RS2_EXTENSION_WHEEL_ODOMETER, "Wheel Odometer" )
                 CASE( RS2_EXTENSION_GLOBAL_TIMER, "Global Timer" )
                 CASE( RS2_EXTENSION_UPDATABLE, "Updatable" )
                 CASE( RS2_EXTENSION_UPDATE_DEVICE, "Update Device" )
                 CASE( RS2_EXTENSION_L500_DEPTH_SENSOR, "L500 Depth Sensor" )
                 CASE( RS2_EXTENSION_TM2_SENSOR, "TM2 Sensor" )
                 CASE( RS2_EXTENSION_AUTO_CALIBRATED_DEVICE, "Auto Calibrated Device" )
                 CASE( RS2_EXTENSION_COLOR_SENSOR, "Color Sensor" )
                 CASE( RS2_EXTENSION_MOTION_SENSOR, "Motion Sensor" )
                 CASE( RS2_EXTENSION_FISHEYE_SENSOR, "Fisheye Sensor" )
                 CASE( RS2_EXTENSION_DEPTH_HUFFMAN_DECODER, "Depth Huffman Decoder" ) // Deprecated
                 CASE( RS2_EXTENSION_SERIALIZABLE, "Serializable" )
                 CASE( RS2_EXTENSION_FW_LOGGER, "FW Logger" )
                 CASE( RS2_EXTENSION_AUTO_CALIBRATION_FILTER, "Auto Calibration Filter" )
                 CASE( RS2_EXTENSION_DEVICE_CALIBRATION, "Device Calibration" )
                 CASE( RS2_EXTENSION_CALIBRATED_SENSOR, "Calibrated Sensor" )
                 CASE( RS2_EXTENSION_SEQUENCE_ID_FILTER, "Sequence ID Filter" )
                 CASE( RS2_EXTENSION_HDR_MERGE, "HDR Merge" )
                 CASE( RS2_EXTENSION_MAX_USABLE_RANGE_SENSOR, "Max Usable Range Sensor" )
                 CASE( RS2_EXTENSION_DEBUG_STREAM_SENSOR, "Debug Stream Sensor" )
                 CASE( RS2_EXTENSION_CALIBRATION_CHANGE_DEVICE, "Calibration Change Device" ) )

GET_STRING_IMPL( rs2_playback_status,
                 RS2_PLAYBACK_STATUS_COUNT,
                 CASE( RS2_PLAYBACK_STATUS_UNKNOWN, "Unknown" )
                 CASE( RS2_PLAYBACK_STATUS_STOPPED, "Stopped" )
                 CASE( RS2_PLAYBACK_STATUS_PAUSED, "Paused" )
                 CASE( RS2_PLAYBACK_STATUS_PLAYING, "Playing" ) )

GET_STRING_IMPL( rs2_log_severity,
                 RS2_LOG_SEVERITY_COUNT,
                 CASE( RS2_LOG_SEVERITY_DEBUG, "Debug" )
                 CASE( RS2_LOG_SEVERITY_INFO, "Info" )
                 CASE( RS2_LOG_SEVERITY_WARN, "Warn" )
                 CASE( RS2_LOG_SEVERITY_ERROR, "Error" )
                 CASE( RS2_LOG_SEVERITY_FATAL, "Fatal" )
                 CASE( RS2_LOG_SEVERITY_NONE, "None" ) )

GET_STRING_IMPL( rs2_option,
                 RS2_OPTION_COUNT,
                 CASE( RS2_OPTION_BACKLIGHT_COMPENSATION, "Backlight Compensation" )
                 CASE( RS2_OPTION_BRIGHTNESS, "Brightness" )
                 CASE( RS2_OPTION_CONTRAST, "Contrast" )
                 CASE( RS2_OPTION_EXPOSURE, "Exposure" )
                 CASE( RS2_OPTION_GAIN, "Gain" )
                 CASE( RS2_OPTION_GAMMA, "Gamma" )
                 CASE( RS2_OPTION_HUE, "Hue" )
                 CASE( RS2_OPTION_SATURATION, "Saturation" )
                 CASE( RS2_OPTION_SHARPNESS, "Sharpness" )
                 CASE( RS2_OPTION_WHITE_BALANCE, "White Balance" )
                 CASE( RS2_OPTION_ENABLE_AUTO_EXPOSURE, "Enable Auto Exposure" )
                 CASE( RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE, "Enable Auto White Balance" )
                 CASE( RS2_OPTION_LASER_POWER, "Laser Power" )
                 CASE( RS2_OPTION_ACCURACY, "Accuracy" )
                 CASE( RS2_OPTION_MOTION_RANGE, "Motion Range" )
                 CASE( RS2_OPTION_FILTER_OPTION, "Filter Option" )
                 CASE( RS2_OPTION_CONFIDENCE_THRESHOLD, "Confidence Threshold" )
                 CASE( RS2_OPTION_FRAMES_QUEUE_SIZE, "Frames Queue Size" )
                 CASE( RS2_OPTION_VISUAL_PRESET, "Visual Preset" )
                 CASE( RS2_OPTION_TOTAL_FRAME_DROPS, "Total Frame Drops" )
                 CASE( RS2_OPTION_EMITTER_ENABLED, "Emitter Enabled" )
                 CASE( RS2_OPTION_AUTO_EXPOSURE_MODE, "Fisheye Auto Exposure Mode" )
                 CASE( RS2_OPTION_POWER_LINE_FREQUENCY, "Power Line Frequency" )
                 CASE( RS2_OPTION_ASIC_TEMPERATURE, "ASIC Temperature" )
                 CASE( RS2_OPTION_ERROR_POLLING_ENABLED, "Error Polling Enabled" )
                 CASE( RS2_OPTION_PROJECTOR_TEMPERATURE, "Projector Temperature" )
                 CASE( RS2_OPTION_OUTPUT_TRIGGER_ENABLED, "Output Trigger Enabled" )
                 CASE( RS2_OPTION_MOTION_MODULE_TEMPERATURE, "Motion Module Temperature" )
                 CASE( RS2_OPTION_DEPTH_UNITS, "Depth Units" )
                 CASE( RS2_OPTION_ENABLE_MOTION_CORRECTION, "Enable Motion Correction" )
                 CASE( RS2_OPTION_AUTO_EXPOSURE_PRIORITY, "Auto Exposure Priority" )
                 CASE( RS2_OPTION_HISTOGRAM_EQUALIZATION_ENABLED, "Histogram Equalization Enabled" )
                 CASE( RS2_OPTION_MIN_DISTANCE, "Min Distance" )
                 CASE( RS2_OPTION_MAX_DISTANCE, "Max Distance" )
                 CASE( RS2_OPTION_COLOR_SCHEME, "Color Scheme" )
                 CASE( RS2_OPTION_TEXTURE_SOURCE, "Texture Source" )
                 CASE( RS2_OPTION_FILTER_MAGNITUDE, "Filter Magnitude" )
                 CASE( RS2_OPTION_FILTER_SMOOTH_ALPHA, "Filter Smooth Alpha" )
                 CASE( RS2_OPTION_FILTER_SMOOTH_DELTA, "Filter Smooth Delta" )
                 CASE( RS2_OPTION_STEREO_BASELINE, "Stereo Baseline" )
                 CASE( RS2_OPTION_HOLES_FILL, "Holes Fill" )
                 CASE( RS2_OPTION_AUTO_EXPOSURE_CONVERGE_STEP, "Auto Exposure Converge Step" )
                 CASE( RS2_OPTION_INTER_CAM_SYNC_MODE, "Inter Cam Sync Mode" )
                 CASE( RS2_OPTION_STREAM_FILTER, "Stream Filter" )
                 CASE( RS2_OPTION_STREAM_FORMAT_FILTER, "Stream Format Filter" )
                 CASE( RS2_OPTION_STREAM_INDEX_FILTER, "Stream Index Filter" )
                 CASE( RS2_OPTION_EMITTER_ON_OFF, "Emitter On Off" )
                 CASE( RS2_OPTION_ZERO_ORDER_POINT_X, "Zero Order Point X" )
                 CASE( RS2_OPTION_ZERO_ORDER_POINT_Y, "Zero Order Point Y" )
                 CASE( RS2_OPTION_LLD_TEMPERATURE, "LDD temperature" )
                 CASE( RS2_OPTION_MC_TEMPERATURE, "MC Temperature" )
                 CASE( RS2_OPTION_MA_TEMPERATURE, "MA Temperature" )
                 CASE( RS2_OPTION_APD_TEMPERATURE, "APD Temperature" )
                 CASE( RS2_OPTION_HARDWARE_PRESET, "Hardware Reset" )
                 CASE( RS2_OPTION_GLOBAL_TIME_ENABLED, "Global Time Enabled" )
                 CASE( RS2_OPTION_ENABLE_MAPPING, "Enable Mapping" )
                 CASE( RS2_OPTION_ENABLE_RELOCALIZATION, "Enable Relocalization" )
                 CASE( RS2_OPTION_ENABLE_POSE_JUMPING, "Enable Pose Jumping" )
                 CASE( RS2_OPTION_ENABLE_DYNAMIC_CALIBRATION, "Enable Dynamic Calibration" )
                 CASE( RS2_OPTION_DEPTH_OFFSET, "Depth Offset" )
                 CASE( RS2_OPTION_LED_POWER, "LED Power" )
                 CASE( RS2_OPTION_ZERO_ORDER_ENABLED, "Zero Order Enabled" )
                 CASE( RS2_OPTION_ENABLE_MAP_PRESERVATION, "Enable Map Preservation" )
                 CASE( RS2_OPTION_FREEFALL_DETECTION_ENABLED, "Freefall Detection Enabled" )
                 CASE( RS2_OPTION_AVALANCHE_PHOTO_DIODE, "Receiver Gain" )
                 CASE( RS2_OPTION_POST_PROCESSING_SHARPENING, "Post Processing Sharpening" )
                 CASE( RS2_OPTION_PRE_PROCESSING_SHARPENING, "Pre Processing Sharpening" )
                 CASE( RS2_OPTION_NOISE_FILTERING, "Noise Filtering" )
                 CASE( RS2_OPTION_INVALIDATION_BYPASS, "Invalidation Bypass" )
              // CASE( RS2_OPTION_AMBIENT_LIGHT ) // Deprecated - replaced by "DIGITAL_GAIN" option
                 CASE( RS2_OPTION_DIGITAL_GAIN, "Digital Gain" )
                 CASE( RS2_OPTION_SENSOR_MODE, "Sensor Mode" )
                 CASE( RS2_OPTION_EMITTER_ALWAYS_ON, "Emitter Always On" )
                 CASE( RS2_OPTION_THERMAL_COMPENSATION, "Thermal Compensation" )
                 CASE( RS2_OPTION_TRIGGER_CAMERA_ACCURACY_HEALTH, "Trigger Camera Accuracy Health" )
                 CASE( RS2_OPTION_RESET_CAMERA_ACCURACY_HEALTH, "Reset Camera Accuracy Health" )
                 CASE( RS2_OPTION_HOST_PERFORMANCE, "Host Performance" )
                 CASE( RS2_OPTION_HDR_ENABLED, "HDR Enabled" )
                 CASE( RS2_OPTION_SEQUENCE_NAME, "Sequence Name" )
                 CASE( RS2_OPTION_SEQUENCE_SIZE, "Sequence Size" )
                 CASE( RS2_OPTION_SEQUENCE_ID, "Sequence ID" )
                 CASE( RS2_OPTION_HUMIDITY_TEMPERATURE, "Humidity Temperature" )
                 CASE( RS2_OPTION_ENABLE_MAX_USABLE_RANGE, "Enable Max Usable Range" )
                 CASE( RS2_OPTION_ALTERNATE_IR, "Alternate IR" )
                 CASE( RS2_OPTION_NOISE_ESTIMATION, "Noise Estimation" )
                 CASE( RS2_OPTION_ENABLE_IR_REFLECTIVITY, "Enable IR Reflectivity" )
                 CASE( RS2_OPTION_AUTO_EXPOSURE_LIMIT, "Auto Exposure Limit" )
                 CASE( RS2_OPTION_AUTO_GAIN_LIMIT, "Auto Gain Limit" )
                 CASE( RS2_OPTION_AUTO_RX_SENSITIVITY, "Auto Rx Sensitivity" )
                 CASE( RS2_OPTION_TRANSMITTER_FREQUENCY, "Transmitter Frequency" )
                 CASE( RS2_OPTION_VERTICAL_BINNING, "Vertical Binning" )
                 CASE( RS2_OPTION_RECEIVER_SENSITIVITY, "Receiver Sensitivity" )
                 CASE( RS2_OPTION_AUTO_EXPOSURE_LIMIT_TOGGLE, "Auto Exposure Limit Toggle" )
                 CASE( RS2_OPTION_AUTO_GAIN_LIMIT_TOGGLE, "Auto Gain Limit Toggle" )
                 CASE( RS2_OPTION_EMITTER_FREQUENCY, "Emitter Frequency" )
                 CASE( RS2_OPTION_DEPTH_AUTO_EXPOSURE_MODE, "Auto Exposure Mode" ) )

GET_STRING_IMPL( rs2_format,
                 RS2_FORMAT_COUNT,
                 CASE( RS2_FORMAT_ANY, "Any" )
                 CASE( RS2_FORMAT_Z16, "Z16" )
                 CASE( RS2_FORMAT_DISPARITY16, "Disparity16" )
                 CASE( RS2_FORMAT_DISPARITY32, "Disparity32" )
                 CASE( RS2_FORMAT_XYZ32F, "XYZ32F" )
                 CASE( RS2_FORMAT_YUYV, "YUYV" )
                 CASE( RS2_FORMAT_RGB8, "RGB8" )
                 CASE( RS2_FORMAT_BGR8, "BGR8" )
                 CASE( RS2_FORMAT_RGBA8, "RGBA8" )
                 CASE( RS2_FORMAT_BGRA8, "BGRA8" )
                 CASE( RS2_FORMAT_Y8, "Y8" )
                 CASE( RS2_FORMAT_Y16, "Y16" )
                 CASE( RS2_FORMAT_RAW10, "Raw10" )
                 CASE( RS2_FORMAT_RAW16, "Raw16" )
                 CASE( RS2_FORMAT_RAW8, "Raw8" )
                 CASE( RS2_FORMAT_UYVY, "UYVY" )
                 CASE( RS2_FORMAT_MOTION_RAW, "Motion Raw" )
                 CASE( RS2_FORMAT_MOTION_XYZ32F, "Motion Xyz32f" )
                 CASE( RS2_FORMAT_GPIO_RAW, "GPIO_raw" )
                 CASE( RS2_FORMAT_6DOF, "6dof" )
                 CASE( RS2_FORMAT_Y10BPACK, "Y10bpack" )
                 CASE( RS2_FORMAT_DISTANCE, "Distance" )
                 CASE( RS2_FORMAT_MJPEG, "Mjpeg" )
                 CASE( RS2_FORMAT_Y8I, "Y8I" )
                 CASE( RS2_FORMAT_Y12I, "Y12I" )
                 CASE( RS2_FORMAT_INZI, "Inzi" )
                 CASE( RS2_FORMAT_INVI, "Invi" )
                 CASE( RS2_FORMAT_W10, "W10" )
                 CASE( RS2_FORMAT_Z16H, "Z16H" )
                 CASE( RS2_FORMAT_FG, "FG" )
                 CASE( RS2_FORMAT_Y411, "Y411" ) )

GET_STRING_IMPL( rs2_distortion,
                 RS2_DISTORTION_COUNT,
                 CASE( RS2_DISTORTION_NONE, "None" )
                 CASE( RS2_DISTORTION_MODIFIED_BROWN_CONRADY, "Modified Brown Conrady" )
                 CASE( RS2_DISTORTION_INVERSE_BROWN_CONRADY, "Inverse Brown Conrady")
                 CASE( RS2_DISTORTION_FTHETA, "Ftheta" )
                 CASE( RS2_DISTORTION_BROWN_CONRADY, "Brown Conrady" )
                 CASE( RS2_DISTORTION_KANNALA_BRANDT4, "Kannala Brandt4" ) )

GET_STRING_IMPL( rs2_camera_info,
                 RS2_CAMERA_INFO_COUNT,
                 CASE( RS2_CAMERA_INFO_NAME, "Name" )
                 CASE( RS2_CAMERA_INFO_SERIAL_NUMBER, "Serial Number" )
                 CASE( RS2_CAMERA_INFO_FIRMWARE_VERSION, "Firmware Version" )
                 CASE( RS2_CAMERA_INFO_RECOMMENDED_FIRMWARE_VERSION, "Recommended Firmware Version" )
                 CASE( RS2_CAMERA_INFO_PHYSICAL_PORT, "Physical Port" )
                 CASE( RS2_CAMERA_INFO_DEBUG_OP_CODE, "Debug Op Code" )
                 CASE( RS2_CAMERA_INFO_ADVANCED_MODE, "Advanced Mode" )
                 CASE( RS2_CAMERA_INFO_PRODUCT_ID, "Product ID" )
                 CASE( RS2_CAMERA_INFO_CAMERA_LOCKED, "Camera Locked" )
                 CASE( RS2_CAMERA_INFO_PRODUCT_LINE, "Product Line" )
                 CASE( RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR, "USB Type Descriptor" )
                 CASE( RS2_CAMERA_INFO_ASIC_SERIAL_NUMBER, "ASIC Serial Number" )
                 CASE( RS2_CAMERA_INFO_FIRMWARE_UPDATE_ID, "Firmware Update ID" )
                 CASE( RS2_CAMERA_INFO_IP_ADDRESS, "IP Address" ) )

GET_STRING_IMPL( rs2_frame_metadata_value,
                 RS2_FRAME_METADATA_COUNT,
                 CASE( RS2_FRAME_METADATA_FRAME_COUNTER, "Frame Counter" )
                 CASE( RS2_FRAME_METADATA_FRAME_TIMESTAMP, "Frame Timestamp" )
                 CASE( RS2_FRAME_METADATA_SENSOR_TIMESTAMP, "Sensor Timestamp" )
                 CASE( RS2_FRAME_METADATA_ACTUAL_EXPOSURE, "Actual Exposure" )
                 CASE( RS2_FRAME_METADATA_GAIN_LEVEL, "Gain Level" )
                 CASE( RS2_FRAME_METADATA_AUTO_EXPOSURE, "Auto Exposure" )
                 CASE( RS2_FRAME_METADATA_WHITE_BALANCE, "White Balance" )
                 CASE( RS2_FRAME_METADATA_TIME_OF_ARRIVAL, "Time of Arrival" )
                 CASE( RS2_FRAME_METADATA_TEMPERATURE, "Temperature" )
                 CASE( RS2_FRAME_METADATA_BACKEND_TIMESTAMP, "Backend Timestamp" )
                 CASE( RS2_FRAME_METADATA_ACTUAL_FPS, "Actual FPS" )
                 CASE( RS2_FRAME_METADATA_FRAME_LASER_POWER, "Frame Laser Power" )
                 CASE( RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE, "Frame Laser Power More" )
                 CASE( RS2_FRAME_METADATA_EXPOSURE_PRIORITY, "Exposure Priority" )
                 CASE( RS2_FRAME_METADATA_EXPOSURE_ROI_LEFT, "Exposure ROI Left" )
                 CASE( RS2_FRAME_METADATA_EXPOSURE_ROI_RIGHT, "Exposure ROI Right" )
                 CASE( RS2_FRAME_METADATA_EXPOSURE_ROI_TOP, "Exposure ROI Top" )
                 CASE( RS2_FRAME_METADATA_EXPOSURE_ROI_BOTTOM, "Exposure ROI Bottom" )
                 CASE( RS2_FRAME_METADATA_BRIGHTNESS, "Brightness" )
                 CASE( RS2_FRAME_METADATA_CONTRAST, "Contrast" )
                 CASE( RS2_FRAME_METADATA_SATURATION, "Saturation" )
                 CASE( RS2_FRAME_METADATA_SHARPNESS, "Sharpness" )
                 CASE( RS2_FRAME_METADATA_AUTO_WHITE_BALANCE_TEMPERATURE, "Auto White Balance Temperature" )
                 CASE( RS2_FRAME_METADATA_BACKLIGHT_COMPENSATION, "Backlight Compensation" )
                 CASE( RS2_FRAME_METADATA_GAMMA, "Gamma" )
                 CASE( RS2_FRAME_METADATA_HUE, "Hue" )
                 CASE( RS2_FRAME_METADATA_MANUAL_WHITE_BALANCE, "Manual White Balance" )
                 CASE( RS2_FRAME_METADATA_POWER_LINE_FREQUENCY, "Power Line Frequency" )
                 CASE( RS2_FRAME_METADATA_LOW_LIGHT_COMPENSATION, "Low Light Compensation" )
                 CASE( RS2_FRAME_METADATA_FRAME_EMITTER_MODE, "Frame Emitter Mode" )
                 CASE( RS2_FRAME_METADATA_FRAME_LED_POWER, "Frame LED Power" )
                 CASE( RS2_FRAME_METADATA_RAW_FRAME_SIZE, "Raw Frame Size" )
                 CASE( RS2_FRAME_METADATA_GPIO_INPUT_DATA, "GPIO Input Data" )
                 CASE( RS2_FRAME_METADATA_SEQUENCE_NAME, "Sequence Name" )
                 CASE( RS2_FRAME_METADATA_SEQUENCE_ID, "Sequence ID" )
                 CASE( RS2_FRAME_METADATA_SEQUENCE_SIZE, "Sequence Size" )
                 CASE( RS2_FRAME_METADATA_TRIGGER, "Trigger" )
                 CASE( RS2_FRAME_METADATA_PRESET, "Preset" )
                 CASE( RS2_FRAME_METADATA_INPUT_WIDTH, "Input Width" )
                 CASE( RS2_FRAME_METADATA_INPUT_HEIGHT, "Input Height" )
                 CASE( RS2_FRAME_METADATA_SUB_PRESET_INFO, "Sub Preset Info" )
                 CASE( RS2_FRAME_METADATA_CALIB_INFO, "Calib Info" )
                 CASE( RS2_FRAME_METADATA_CRC, "CRC" ) )

GET_STRING_IMPL( rs2_timestamp_domain,
                 RS2_TIMESTAMP_DOMAIN_COUNT,
                 CASE( RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, "Hardware Clock" )
                 CASE( RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME, "System Time" )
                 CASE( RS2_TIMESTAMP_DOMAIN_GLOBAL_TIME, "Global Time" ) )

GET_STRING_IMPL( rs2_calib_target_type,
                 RS2_CALIB_TARGET_COUNT,
                 CASE( RS2_CALIB_TARGET_RECT_GAUSSIAN_DOT_VERTICES, "Rect Gaussian Dot Vertices" )
                 CASE( RS2_CALIB_TARGET_ROI_RECT_GAUSSIAN_DOT_VERTICES, "ROI Rect Gaussian Dot Vertices" )
                 CASE( RS2_CALIB_TARGET_POS_GAUSSIAN_DOT_VERTICES, "Pos Gaussian Dot Vertices" ) )

GET_STRING_IMPL( rs2_notification_category,
                 RS2_NOTIFICATION_CATEGORY_COUNT,
                 CASE( RS2_NOTIFICATION_CATEGORY_FRAMES_TIMEOUT, "Frames Timeout" )
                 CASE( RS2_NOTIFICATION_CATEGORY_FRAME_CORRUPTED, "Frame Corrupted" )
                 CASE( RS2_NOTIFICATION_CATEGORY_HARDWARE_ERROR, "Hardware Error" )
                 CASE( RS2_NOTIFICATION_CATEGORY_HARDWARE_EVENT, "Hardware Event" )
                 CASE( RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR, "Unknown Error" )
                 CASE( RS2_NOTIFICATION_CATEGORY_FIRMWARE_UPDATE_RECOMMENDED, "Firmware Update Recommended" )
                 CASE( RS2_NOTIFICATION_CATEGORY_POSE_RELOCALIZATION, "Pose Relocalization" ) )

GET_STRING_IMPL( rs2_matchers,
                 RS2_MATCHER_COUNT,
                 CASE( RS2_MATCHER_DI, "DI" )
                 CASE( RS2_MATCHER_DI_C, "DI C" )
                 CASE( RS2_MATCHER_DLR_C, "DLR C" )
                 CASE( RS2_MATCHER_DLR, "DLR" )
                 CASE( RS2_MATCHER_DIC, "DIC" )
                 CASE( RS2_MATCHER_DIC_C, "DIC C" )
                 CASE( RS2_MATCHER_DEFAULT, "Default" ) )

GET_STRING_IMPL( rs2_l500_visual_preset,
                 RS2_L500_VISUAL_PRESET_COUNT,
                 CASE( RS2_L500_VISUAL_PRESET_CUSTOM, "Custom" )
                 CASE( RS2_L500_VISUAL_PRESET_DEFAULT, "Default" )
                 CASE( RS2_L500_VISUAL_PRESET_NO_AMBIENT, "No Ambient Light" )
                 CASE( RS2_L500_VISUAL_PRESET_LOW_AMBIENT, "Low Ambient Light" )
                 CASE( RS2_L500_VISUAL_PRESET_MAX_RANGE, "Max Range" )
                 CASE( RS2_L500_VISUAL_PRESET_SHORT_RANGE, "Short Range" )
                 CASE( RS2_L500_VISUAL_PRESET_AUTOMATIC, "Automatic" ) )


}  // namespace librealsense

const char * rs2_stream_to_string( rs2_stream stream ) { return librealsense::get_string( stream ).c_str(); }
const char * rs2_format_to_string( rs2_format format ) { return librealsense::get_string( format ).c_str(); }
const char * rs2_distortion_to_string( rs2_distortion distortion ) { return librealsense::get_string( distortion ).c_str(); }
const char * rs2_option_to_string( rs2_option option ) { return librealsense::get_string( option ).c_str(); }
const char * rs2_camera_info_to_string( rs2_camera_info info ) { return librealsense::get_string( info ).c_str(); }
const char * rs2_timestamp_domain_to_string( rs2_timestamp_domain info ) { return librealsense::get_string( info ).c_str(); }
const char * rs2_notification_category_to_string( rs2_notification_category category ) { return librealsense::get_string( category ).c_str(); }
const char * rs2_calib_target_type_to_string( rs2_calib_target_type type ) { return librealsense::get_string( type ).c_str(); }
const char * rs2_sr300_visual_preset_to_string( rs2_sr300_visual_preset preset ) { return librealsense::get_string( preset ).c_str(); }
const char * rs2_rs400_visual_preset_to_string( rs2_rs400_visual_preset preset ) { return librealsense::get_string( preset ).c_str(); }
const char * rs2_log_severity_to_string( rs2_log_severity severity ) { return librealsense::get_string( severity ).c_str(); }
const char * rs2_exception_type_to_string( rs2_exception_type type ) { return librealsense::get_string( type ).c_str(); }
const char * rs2_playback_status_to_string( rs2_playback_status status ) { return librealsense::get_string( status ).c_str(); }
const char * rs2_extension_type_to_string( rs2_extension type ) { return librealsense::get_string( type ).c_str(); }
const char * rs2_matchers_to_string( rs2_matchers matcher ) { return librealsense::get_string( matcher ).c_str(); }
const char * rs2_frame_metadata_to_string( rs2_frame_metadata_value metadata ) { return librealsense::get_string( metadata ).c_str(); }
const char * rs2_extension_to_string( rs2_extension type ) { return rs2_extension_type_to_string( type ); }
const char * rs2_frame_metadata_value_to_string( rs2_frame_metadata_value metadata ) { return rs2_frame_metadata_to_string( metadata ); }
const char * rs2_l500_visual_preset_to_string( rs2_l500_visual_preset preset ) { return librealsense::get_string( preset ).c_str(); }
const char * rs2_sensor_mode_to_string( rs2_sensor_mode mode ) { return librealsense::get_string( mode ).c_str(); }
const char * rs2_ambient_light_to_string( rs2_ambient_light ambient ) { return librealsense::get_string( ambient ).c_str(); }
const char * rs2_digital_gain_to_string( rs2_digital_gain gain ) { return librealsense::get_string( gain ).c_str(); }
const char * rs2_cah_trigger_to_string( int mode ) { return "DEPRECATED as of 2.46"; }
const char * rs2_calibration_type_to_string( rs2_calibration_type type ) { return librealsense::get_string( type ).c_str(); }
const char * rs2_calibration_status_to_string( rs2_calibration_status status ) { return librealsense::get_string( status ).c_str(); }
const char * rs2_host_perf_mode_to_string( rs2_host_perf_mode mode ) { return librealsense::get_string( mode ).c_str(); }
const char * rs2_emitter_frequency_mode_to_string( rs2_emitter_frequency_mode mode ) { return librealsense::get_string( mode ).c_str(); }
const char * rs2_depth_auto_exposure_mode_to_string( rs2_depth_auto_exposure_mode mode ) { return librealsense::get_string( mode ).c_str(); }
