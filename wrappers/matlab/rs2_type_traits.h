#pragma once
#include "librealsense2/rs.hpp"
#include <type_traits>

// extra checks to help specialization priority resoultion
template <typename T> using extra_checks = std::bool_constant<std::is_base_of<rs2::frame, T>::value>;

// rs_context.hpp
template<> struct MatlabParamParser::type_traits<rs2::context> { using rs2_internal_t = std::shared_ptr<rs2_context>; };
template<> struct MatlabParamParser::type_traits<rs2::device_hub> { using rs2_internal_t = std::shared_ptr<rs2_device_hub>; }
;
// rs_device.hpp
template<> struct MatlabParamParser::type_traits<rs2::device> { using rs2_internal_t = std::shared_ptr<rs2_device>; };
template<> struct MatlabParamParser::type_traits<rs2::device_list> { using rs2_internal_t = std::shared_ptr<rs2_device_list>; };

// rs_frame.hpp
template<> struct MatlabParamParser::type_traits<rs2::stream_profile> { using rs2_internal_t = const rs2_stream_profile*; };
template<typename T> struct MatlabParamParser::type_traits<T, typename std::enable_if<std::is_base_of<rs2::frame, T>::value>::type> { using rs2_internal_t = rs2_frame *; };

// rs_processing.hpp

// rs_record_playback.hpp
template<> struct MatlabParamParser::type_traits<rs2::playback> : MatlabParamParser::type_traits<rs2::device> {};

// rs_sensor.hpp
template<typename T> struct MatlabParamParser::type_traits<T, typename std::enable_if<std::is_base_of<rs2::sensor, T>::value>::type> { using rs2_internal_t = std::shared_ptr<rs2_sensor>; };

// rs_pipeline.hpp
template<> struct MatlabParamParser::type_traits<rs2::pipeline> { using rs2_internal_t = std::shared_ptr<rs2_pipeline>; };