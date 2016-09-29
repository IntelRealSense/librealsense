// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#if !defined(MAKEFILE) || ( defined(OFFLINE_TEST) )

#define CATCH_CONFIG_MAIN
#include "catch/catch.hpp"

#include "unit-tests-common.h"
#include "../src/device.h"

#include <sstream>

static std::string unknown = "UNKNOWN"; 

// Helper to produce a not-null pointer to a specific object type, to help validate API methods.
// Use with caution, the resulting pointer does not address a real object!
struct fake_object_pointer { template<class T> operator T * () const { return (T *)(0x100); } };


TEST_CASE("wraparound_mechanism produces correct output", "[offline] [validation]")
{
    auto unsigned_short_max = std::numeric_limits<uint16_t>::max();
    rsimpl::wraparound_mechanism<unsigned long long> wm(1, unsigned_short_max);

    unsigned long long last_number = 65532;
    for (unsigned i = 65533; last_number < unsigned_short_max * 5; ++i)
    {
        if (i > unsigned_short_max)
            i = 1;

        auto new_number = wm.fix(i);
        REQUIRE((new_number - last_number) == 1);
        last_number = new_number;
    }
}

TEST_CASE( "rs_create_context() validates input", "[offline] [validation]" )
{
    REQUIRE(rs_create_context(RS_API_VERSION - 100, require_error("", false)) == nullptr);
    REQUIRE(rs_create_context(RS_API_VERSION + 100, require_error("", false)) == nullptr);
    auto ctx = rs_create_context(RS_API_VERSION + 1, require_no_error()); // make sure changes in patch do not fail context creation
    REQUIRE(ctx != nullptr);
    rs_delete_context(ctx, require_no_error());
}

TEST_CASE( "rs_delete_context() validates input", "[offline] [validation]" )
{
    rs_delete_context(nullptr, require_error("null pointer passed for argument \"context\""));
}

TEST_CASE( "rs_get_device_count() validates input", "[offline] [validation]" )
{
    REQUIRE(rs_get_device_count(nullptr, require_error("null pointer passed for argument \"context\"")) == 0);
}

TEST_CASE( "rs_get_device() validates input", "[offline] [validation]" )
{
    REQUIRE(rs_get_device(nullptr, 0, require_error("null pointer passed for argument \"context\"")) == nullptr);

    REQUIRE(rs_get_device(fake_object_pointer(), -1, require_error("out of range value for argument \"index\"")) == nullptr);
    // NOTE: Index upper bound determined by rs_get_device_count(), can't validate without a live object
}

TEST_CASE( "rs_get_device_name() validates input", "[offline] [validation]" )
{
    REQUIRE(rs_get_device_name(nullptr, require_error("null pointer passed for argument \"device\"")) == nullptr);
}

TEST_CASE( "rs_get_device_serial() validates input", "[offline] [validation]" )
{
    REQUIRE(rs_get_device_serial(nullptr, require_error("null pointer passed for argument \"device\"")) == nullptr);
}

TEST_CASE( "rs_get_device_firmware_version() validates input", "[offline] [validation]" )
{
    REQUIRE(rs_get_device_firmware_version(nullptr, require_error("null pointer passed for argument \"device\"")) == nullptr);
}

TEST_CASE( "rs_get_device_extrinsics() validates input", "[offline] [validation]" )
{
    rs_extrinsics extrin;
    rs_get_device_extrinsics(nullptr,               RS_STREAM_DEPTH,    RS_STREAM_COLOR,    &extrin, require_error("null pointer passed for argument \"device\""));
                                                                                            
    rs_get_device_extrinsics(fake_object_pointer(), (rs_stream)-1,      RS_STREAM_COLOR,    &extrin, require_error("bad enum value for argument \"from\""));
    rs_get_device_extrinsics(fake_object_pointer(), RS_STREAM_COUNT,    RS_STREAM_COLOR,    &extrin, require_error("bad enum value for argument \"from\""));

    rs_get_device_extrinsics(fake_object_pointer(), RS_STREAM_DEPTH,    (rs_stream)-1,      &extrin, require_error("bad enum value for argument \"to\""));
    rs_get_device_extrinsics(fake_object_pointer(), RS_STREAM_DEPTH,    RS_STREAM_COUNT,    &extrin, require_error("bad enum value for argument \"to\""));
                                                                        
    rs_get_device_extrinsics(fake_object_pointer(), RS_STREAM_DEPTH,    RS_STREAM_COLOR,    nullptr, require_error("null pointer passed for argument \"extrin\""));
}

TEST_CASE( "rs_get_device_depth_scale() validates input", "[offline] [validation]" )
{
    REQUIRE(rs_get_device_depth_scale(nullptr, require_error("null pointer passed for argument \"device\"")) == 0.0f);
}

TEST_CASE( "rs_device_supports_option() validates input", "[offline] [validation]" )
{
    REQUIRE(rs_device_supports_option(nullptr,               RS_OPTION_COLOR_GAIN, require_error("null pointer passed for argument \"device\"")) == 0);

    REQUIRE(rs_device_supports_option(fake_object_pointer(), (rs_option)-1,        require_error("bad enum value for argument \"option\"")) == 0);
    REQUIRE(rs_device_supports_option(fake_object_pointer(), RS_OPTION_COUNT,      require_error("bad enum value for argument \"option\"")) == 0);
}

TEST_CASE( "rs_get_stream_mode_count() validates input", "[offline] [validation]" )
{
    REQUIRE(rs_get_stream_mode_count(nullptr,               RS_STREAM_DEPTH,    require_error("null pointer passed for argument \"device\"")) == 0);
                                                                                
    REQUIRE(rs_get_stream_mode_count(fake_object_pointer(), (rs_stream)-1,      require_error("bad enum value for argument \"stream\"")) == 0);
    REQUIRE(rs_get_stream_mode_count(fake_object_pointer(), RS_STREAM_COUNT,    require_error("bad enum value for argument \"stream\"")) == 0);
}

TEST_CASE( "rs_get_stream_mode() validates input", "[offline] [validation]" )
{
    int width, height, framerate; rs_format format;
    rs_get_stream_mode(nullptr,               RS_STREAM_DEPTH,     0, &width, &height, &format, &framerate, require_error("null pointer passed for argument \"device\""));
                                                                   
    rs_get_stream_mode(fake_object_pointer(), (rs_stream)-1,       0, &width, &height, &format, &framerate, require_error("bad enum value for argument \"stream\""));
    rs_get_stream_mode(fake_object_pointer(), RS_STREAM_COUNT,     0, &width, &height, &format, &framerate, require_error("bad enum value for argument \"stream\""));

    rs_get_stream_mode(fake_object_pointer(), RS_STREAM_DEPTH,    -1, &width, &height, &format, &framerate, require_error("out of range value for argument \"index\""));
    // NOTE: Index upper bound determined by rs_get_stream_mode_count(), can't validate without a live object

    // NOTE: width, height, format, framerate are all permitted to be null, nothing to validate
}

TEST_CASE( "rs_enable_stream() validates input", "[offline] [validation]" )
{
    rs_enable_stream(nullptr, RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, 60, require_error("null pointer passed for argument \"device\""));
                                                                                              
    const auto RS_STREAM_MAX_ENUM = (rs_stream)0xFFFF;
    const auto RS_FORMAT_MAX_ENUM = (rs_format)0xFFFF;
    rs_enable_stream(fake_object_pointer(), (rs_stream)-1, 640, 480, RS_FORMAT_Z16, 60, require_error("bad enum value for argument \"stream\""));
    rs_enable_stream(fake_object_pointer(), RS_STREAM_COUNT, 640, 480, RS_FORMAT_Z16, 60, require_error("bad enum value for argument \"stream\""));
    rs_enable_stream(fake_object_pointer(), RS_STREAM_MAX_ENUM, 640, 480, RS_FORMAT_Z16, 60, require_error("bad enum value for argument \"stream\""));
                                                                                              
    rs_enable_stream(fake_object_pointer(), RS_STREAM_DEPTH, -1, 480, RS_FORMAT_Z16, 60, require_error("out of range value for argument \"width\""));
                                                                                              
    rs_enable_stream(fake_object_pointer(), RS_STREAM_DEPTH, 640, -1, RS_FORMAT_Z16, 60, require_error("out of range value for argument \"height\""));
                                                                                              
    rs_enable_stream(fake_object_pointer(), RS_STREAM_DEPTH, 640, 480, (rs_format)-1, 60, require_error("bad enum value for argument \"format\""));
    rs_enable_stream(fake_object_pointer(), RS_STREAM_DEPTH, 640, 480, RS_FORMAT_COUNT, 60, require_error("bad enum value for argument \"format\""));
    rs_enable_stream(fake_object_pointer(), RS_STREAM_DEPTH, 640, 480, RS_FORMAT_MAX_ENUM, 60, require_error("bad enum value for argument \"format\""));
                                                                
    rs_enable_stream(fake_object_pointer(), RS_STREAM_DEPTH, 640, 480, RS_FORMAT_Z16, -1, require_error("out of range value for argument \"framerate\""));
}

TEST_CASE( "rs_enable_stream_preset() validates input", "[offline] [validation]" )
{
    rs_enable_stream_preset(nullptr,               RS_STREAM_DEPTH,    RS_PRESET_BEST_QUALITY, require_error("null pointer passed for argument \"device\""));
                                                                       
    rs_enable_stream_preset(fake_object_pointer(), (rs_stream)-1,      RS_PRESET_BEST_QUALITY, require_error("bad enum value for argument \"stream\""));
    rs_enable_stream_preset(fake_object_pointer(), RS_STREAM_COUNT,    RS_PRESET_BEST_QUALITY, require_error("bad enum value for argument \"stream\""));

    rs_enable_stream_preset(fake_object_pointer(), RS_STREAM_DEPTH,    (rs_preset)-1,          require_error("bad enum value for argument \"preset\""));
    rs_enable_stream_preset(fake_object_pointer(), RS_STREAM_DEPTH,    RS_PRESET_COUNT,        require_error("bad enum value for argument \"preset\""));
}

TEST_CASE( "rs_disable_stream() validates input", "[offline] [validation]" )
{
    rs_disable_stream(nullptr,               RS_STREAM_DEPTH,    require_error("null pointer passed for argument \"device\""));
                                                                 
    rs_disable_stream(fake_object_pointer(), (rs_stream)-1,      require_error("bad enum value for argument \"stream\""));
    rs_disable_stream(fake_object_pointer(), RS_STREAM_COUNT,    require_error("bad enum value for argument \"stream\""));
}

TEST_CASE( "rs_is_stream_enabled() validates input", "[offline] [validation]" )
{
    REQUIRE(rs_is_stream_enabled(nullptr,               RS_STREAM_DEPTH,    require_error("null pointer passed for argument \"device\"")) == 0);
                                                                            
    REQUIRE(rs_is_stream_enabled(fake_object_pointer(), (rs_stream)-1,      require_error("bad enum value for argument \"stream\"")) == 0);
    REQUIRE(rs_is_stream_enabled(fake_object_pointer(), RS_STREAM_COUNT,    require_error("bad enum value for argument \"stream\"")) == 0);
}

TEST_CASE( "rs_get_stream_intrinsics() validates input", "[offline] [validation]" )
{
    rs_intrinsics intrin;
    rs_get_stream_intrinsics(nullptr,               RS_STREAM_DEPTH,    &intrin, require_error("null pointer passed for argument \"device\""));
                                                                        
    rs_get_stream_intrinsics(fake_object_pointer(), (rs_stream)-1,      &intrin, require_error("bad enum value for argument \"stream\""));
    rs_get_stream_intrinsics(fake_object_pointer(), RS_STREAM_COUNT,    &intrin, require_error("bad enum value for argument \"stream\""));

    rs_get_stream_intrinsics(fake_object_pointer(), RS_STREAM_DEPTH,    nullptr, require_error("null pointer passed for argument \"intrin\""));
}

TEST_CASE( "rs_get_stream_format() validates input", "[offline] [validation]" )
{
    REQUIRE(rs_get_stream_format(nullptr,               RS_STREAM_DEPTH,    require_error("null pointer passed for argument \"device\"")) == RS_FORMAT_ANY);
                                                                            
    REQUIRE(rs_get_stream_format(fake_object_pointer(), (rs_stream)-1,      require_error("bad enum value for argument \"stream\"")) == RS_FORMAT_ANY);
    REQUIRE(rs_get_stream_format(fake_object_pointer(), RS_STREAM_COUNT,    require_error("bad enum value for argument \"stream\"")) == RS_FORMAT_ANY);
}

TEST_CASE( "rs_get_stream_framerate() validates input", "[offline] [validation]" )
{
    REQUIRE(rs_get_stream_framerate(nullptr,               RS_STREAM_DEPTH,    require_error("null pointer passed for argument \"device\"")) == 0);
                                                                               
    REQUIRE(rs_get_stream_framerate(fake_object_pointer(), (rs_stream)-1,      require_error("bad enum value for argument \"stream\"")) == 0);
    REQUIRE(rs_get_stream_framerate(fake_object_pointer(), RS_STREAM_COUNT,    require_error("bad enum value for argument \"stream\"")) == 0);
}

TEST_CASE( "rs_start_device() validates input", "[offline] [validation]" )
{
    rs_start_device(nullptr, require_error("null pointer passed for argument \"device\""));
}

TEST_CASE( "rs_stop_device() validates input", "[offline] [validation]" )
{
    rs_stop_device(nullptr, require_error("null pointer passed for argument \"device\""));
}

TEST_CASE( "rs_is_device_streaming() validates input", "[offline] [validation]" )
{
    REQUIRE(rs_is_device_streaming(nullptr, require_error("null pointer passed for argument \"device\"")) == 0);
}

TEST_CASE( "rs_set_device_option() validates input", "[offline] [validation]" )
{
    rs_set_device_option(nullptr,               RS_OPTION_COLOR_GAIN, 100, require_error("null pointer passed for argument \"device\""));

    rs_set_device_option(fake_object_pointer(), (rs_option)-1,        100, require_error("bad enum value for argument \"option\""));
    rs_set_device_option(fake_object_pointer(), RS_OPTION_COUNT,      100, require_error("bad enum value for argument \"option\""));

    // NOTE: Currently no validation is done for valid option ranges at the API level, though specifying an invalid option may fail at the UVC level
    // todo - Add some basic validation for parameter sanity (gain/exposure cannot be negative, depth clamping must be in uint16_t range, etc...)
}

TEST_CASE( "rs_get_device_option() validates input", "[offline] [validation]" )
{
    REQUIRE(rs_get_device_option(nullptr,               RS_OPTION_COLOR_GAIN, require_error("null pointer passed for argument \"device\"")) == 0);

    REQUIRE(rs_get_device_option(fake_object_pointer(), (rs_option)-1,        require_error("bad enum value for argument \"option\"")) == 0);
    REQUIRE(rs_get_device_option(fake_object_pointer(), RS_OPTION_COUNT,      require_error("bad enum value for argument \"option\"")) == 0);
}

TEST_CASE( "rs_wait_for_frames() validates input", "[offline] [validation]" )
{
    rs_wait_for_frames(nullptr, require_error("null pointer passed for argument \"device\""));
}

TEST_CASE( "rs_get_frame_timestamp() validates input", "[offline] [validation]" )
{
    REQUIRE(rs_get_frame_timestamp(nullptr,               RS_STREAM_DEPTH,    require_error("null pointer passed for argument \"device\"")) == 0);
                                                                              
    REQUIRE(rs_get_frame_timestamp(fake_object_pointer(), (rs_stream)-1,      require_error("bad enum value for argument \"stream\"")) == 0);
    REQUIRE(rs_get_frame_timestamp(fake_object_pointer(), RS_STREAM_COUNT,    require_error("bad enum value for argument \"stream\"")) == 0);
}

TEST_CASE( "rs_get_frame_data() validates input", "[offline] [validation]" )
{
    REQUIRE(rs_get_frame_data(nullptr,               RS_STREAM_DEPTH,    require_error("null pointer passed for argument \"device\"")) == nullptr);
                                                                         
    REQUIRE(rs_get_frame_data(fake_object_pointer(), (rs_stream)-1,      require_error("bad enum value for argument \"stream\"")) == nullptr);
    REQUIRE(rs_get_frame_data(fake_object_pointer(), RS_STREAM_COUNT,    require_error("bad enum value for argument \"stream\"")) == nullptr);
}

TEST_CASE( "rs_free_error() gracefully handles invalid input", "[offline] [validation]" )
{
    // Nothing to assert in this case, but calling rs_free_error() with a null pointer should not crash the program
    rs_free_error(nullptr);
}

TEST_CASE( "rs_get_failed_function() gracefully handles invalid input", "[offline] [validation]" )
{
    REQUIRE(rs_get_failed_function(nullptr) == nullptr);
}

TEST_CASE( "rs_get_failed_args() gracefully handles invalid input", "[offline] [validation]" )
{
    REQUIRE(rs_get_failed_args(nullptr) == nullptr);
}

TEST_CASE( "rs_get_error_message() gracefully handles invalid input", "[offline] [validation]" )
{
    REQUIRE(rs_get_error_message(nullptr) == nullptr);
}

TEST_CASE( "rs_stream_to_string() produces correct output", "[offline] [validation]" )
{
    // Valid enum values should return the text that follows the type prefix
    REQUIRE(rs_stream_to_string(RS_STREAM_DEPTH) == std::string("DEPTH"));
    REQUIRE(rs_stream_to_string(RS_STREAM_COLOR) == std::string("COLOR"));
    REQUIRE(rs_stream_to_string(RS_STREAM_INFRARED) == std::string("INFRARED"));
    REQUIRE(rs_stream_to_string(RS_STREAM_INFRARED2) == std::string("INFRARED2"));
    REQUIRE(rs_stream_to_string(RS_STREAM_RECTIFIED_COLOR) == std::string("RECTIFIED_COLOR"));
    REQUIRE(rs_stream_to_string(RS_STREAM_COLOR_ALIGNED_TO_DEPTH) == std::string("COLOR_ALIGNED_TO_DEPTH"));
    REQUIRE(rs_stream_to_string(RS_STREAM_DEPTH_ALIGNED_TO_COLOR) == std::string("DEPTH_ALIGNED_TO_COLOR"));
    REQUIRE(rs_stream_to_string(RS_STREAM_DEPTH_ALIGNED_TO_RECTIFIED_COLOR) == std::string("DEPTH_ALIGNED_TO_RECTIFIED_COLOR"));

    // Invalid enum values should return nullptr
    REQUIRE(rs_stream_to_string((rs_stream)-1) == unknown);
    REQUIRE(rs_stream_to_string(RS_STREAM_COUNT) == unknown);
}

TEST_CASE( "rs_format_to_string() produces correct output", "[offline] [validation]" )
{
    // Valid enum values should return the text that follows the type prefix
    REQUIRE(rs_format_to_string(RS_FORMAT_ANY) == std::string("ANY"));
    REQUIRE(rs_format_to_string(RS_FORMAT_Z16) == std::string("Z16"));
    REQUIRE(rs_format_to_string(RS_FORMAT_DISPARITY16) == std::string("DISPARITY16"));
    REQUIRE(rs_format_to_string(RS_FORMAT_YUYV) == std::string("YUYV"));
    REQUIRE(rs_format_to_string(RS_FORMAT_RGB8) == std::string("RGB8"));
    REQUIRE(rs_format_to_string(RS_FORMAT_BGR8) == std::string("BGR8"));
    REQUIRE(rs_format_to_string(RS_FORMAT_RGBA8) == std::string("RGBA8"));
    REQUIRE(rs_format_to_string(RS_FORMAT_BGRA8) == std::string("BGRA8"));
    REQUIRE(rs_format_to_string(RS_FORMAT_Y8) == std::string("Y8"));
    REQUIRE(rs_format_to_string(RS_FORMAT_Y16) == std::string("Y16"));
    REQUIRE(rs_format_to_string(RS_FORMAT_RAW10) == std::string("RAW10"));

    // Invalid enum values should return nullptr
    REQUIRE(rs_format_to_string((rs_format)-1) == unknown);
    REQUIRE(rs_format_to_string(RS_FORMAT_COUNT) == unknown);
}

TEST_CASE( "rs_preset_to_string() produces correct output", "[offline] [validation]" )
{
    // Valid enum values should return the text that follows the type prefix
    REQUIRE(rs_preset_to_string(RS_PRESET_BEST_QUALITY) == std::string("BEST_QUALITY"));
    REQUIRE(rs_preset_to_string(RS_PRESET_LARGEST_IMAGE) == std::string("LARGEST_IMAGE"));
    REQUIRE(rs_preset_to_string(RS_PRESET_HIGHEST_FRAMERATE) == std::string("HIGHEST_FRAMERATE"));

    // Invalid enum values should return nullptr
    REQUIRE(rs_preset_to_string((rs_preset)-1) == unknown);
    REQUIRE(rs_preset_to_string(RS_PRESET_COUNT) == unknown);
}

TEST_CASE( "rs_distortion_to_string() produces correct output", "[offline] [validation]" )
{
    // Valid enum values should return the text that follows the type prefix
    REQUIRE(rs_distortion_to_string(RS_DISTORTION_NONE) == std::string("NONE"));
    REQUIRE(rs_distortion_to_string(RS_DISTORTION_MODIFIED_BROWN_CONRADY) == std::string("MODIFIED_BROWN_CONRADY"));
    REQUIRE(rs_distortion_to_string(RS_DISTORTION_INVERSE_BROWN_CONRADY) == std::string("INVERSE_BROWN_CONRADY"));

    // Invalid enum values should return nullptr
    REQUIRE(rs_distortion_to_string((rs_distortion)-1) == unknown);
    REQUIRE(rs_distortion_to_string(RS_DISTORTION_COUNT) == unknown);
}

TEST_CASE( "rs_option_to_string() produces correct output", "[offline] [validation]" )
{
    // Valid enum values should return the text that follows the type prefix
    REQUIRE(rs_option_to_string(RS_OPTION_COLOR_BACKLIGHT_COMPENSATION) == std::string("COLOR_BACKLIGHT_COMPENSATION")); 
    REQUIRE(rs_option_to_string(RS_OPTION_COLOR_BRIGHTNESS) == std::string("COLOR_BRIGHTNESS")); 
    REQUIRE(rs_option_to_string(RS_OPTION_COLOR_CONTRAST) == std::string("COLOR_CONTRAST")); 
    REQUIRE(rs_option_to_string(RS_OPTION_COLOR_EXPOSURE) == std::string("COLOR_EXPOSURE")); 
    REQUIRE(rs_option_to_string(RS_OPTION_COLOR_GAIN) == std::string("COLOR_GAIN")); 
    REQUIRE(rs_option_to_string(RS_OPTION_COLOR_GAMMA) == std::string("COLOR_GAMMA")); 
    REQUIRE(rs_option_to_string(RS_OPTION_COLOR_HUE) == std::string("COLOR_HUE")); 
    REQUIRE(rs_option_to_string(RS_OPTION_COLOR_SATURATION) == std::string("COLOR_SATURATION")); 
    REQUIRE(rs_option_to_string(RS_OPTION_COLOR_SHARPNESS) == std::string("COLOR_SHARPNESS")); 
    REQUIRE(rs_option_to_string(RS_OPTION_COLOR_WHITE_BALANCE) == std::string("COLOR_WHITE_BALANCE")); 
    REQUIRE(rs_option_to_string(RS_OPTION_COLOR_ENABLE_AUTO_EXPOSURE) == std::string("COLOR_ENABLE_AUTO_EXPOSURE")); 
    REQUIRE(rs_option_to_string(RS_OPTION_COLOR_ENABLE_AUTO_WHITE_BALANCE) == std::string("COLOR_ENABLE_AUTO_WHITE_BALANCE")); 

    REQUIRE(rs_option_to_string(RS_OPTION_F200_LASER_POWER) == std::string("F200_LASER_POWER")); 
    REQUIRE(rs_option_to_string(RS_OPTION_F200_ACCURACY) == std::string("F200_ACCURACY")); 
    REQUIRE(rs_option_to_string(RS_OPTION_F200_MOTION_RANGE) == std::string("F200_MOTION_RANGE")); 
    REQUIRE(rs_option_to_string(RS_OPTION_F200_FILTER_OPTION) == std::string("F200_FILTER_OPTION")); 
    REQUIRE(rs_option_to_string(RS_OPTION_F200_CONFIDENCE_THRESHOLD) == std::string("F200_CONFIDENCE_THRESHOLD")); 
    REQUIRE(rs_option_to_string(RS_OPTION_F200_DYNAMIC_FPS) == std::string("F200_DYNAMIC_FPS")); 
    
    REQUIRE(rs_option_to_string(RS_OPTION_R200_LR_AUTO_EXPOSURE_ENABLED) == std::string("R200_LR_AUTO_EXPOSURE_ENABLED")); 
    REQUIRE(rs_option_to_string(RS_OPTION_R200_LR_GAIN) == std::string("R200_LR_GAIN")); 
    REQUIRE(rs_option_to_string(RS_OPTION_R200_LR_EXPOSURE) == std::string("R200_LR_EXPOSURE")); 
    REQUIRE(rs_option_to_string(RS_OPTION_R200_EMITTER_ENABLED) == std::string("R200_EMITTER_ENABLED"));
    REQUIRE(rs_option_to_string(RS_OPTION_R200_DEPTH_UNITS) == std::string("R200_DEPTH_UNITS")); 
    REQUIRE(rs_option_to_string(RS_OPTION_R200_DEPTH_CLAMP_MIN) == std::string("R200_DEPTH_CLAMP_MIN")); 
    REQUIRE(rs_option_to_string(RS_OPTION_R200_DEPTH_CLAMP_MAX) == std::string("R200_DEPTH_CLAMP_MAX")); 
    REQUIRE(rs_option_to_string(RS_OPTION_R200_DISPARITY_MULTIPLIER) == std::string("R200_DISPARITY_MULTIPLIER")); 
    REQUIRE(rs_option_to_string(RS_OPTION_R200_DISPARITY_SHIFT) == std::string("R200_DISPARITY_SHIFT")); 

    // Invalid enum values should return nullptr
    REQUIRE(rs_option_to_string((rs_option)-1) == unknown);
    REQUIRE(rs_option_to_string(RS_OPTION_COUNT) == unknown);
}

TEST_CASE( "rs_create_context() returns a valid context", "[offline] [validation]" )
{
    safe_context ctx;
    REQUIRE(rs_get_device_count(ctx, require_no_error()) >= 0);
}

TEST_CASE( "rs_context has singleton semantics", "[offline] [validation]" )
{
    safe_context ctx;
    safe_context second_ctx;
    REQUIRE(second_ctx == ctx);
}

TEST_CASE("rs API version verification", "[offline] [validation]")
{
    safe_context ctx;
    std::cout << "Librealsense API version is " << RS_API_VERSION_STR << std::endl;
    std::cout << "Librealsense API version number is " << RS_API_VERSION << std::endl;

    std::string api_ver_str(RS_API_VERSION_STR);
    // API  version is within [10000..999999] range
    REQUIRE(RS_API_VERSION > 0);
    REQUIRE(RS_API_VERSION <= 999999);
    // Version string is in ["1.0.0".. "99.99.99"] range
    REQUIRE(api_ver_str.size() >= 5);
    REQUIRE(api_ver_str.size() <= 8);
}

#endif /* !defined(MAKEFILE) || ( defined(OFFLINE_TEST) ) */
