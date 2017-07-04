//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
//
//#include "conversions.h"
//#include <cstring>
//
//using namespace rs::core;
//using namespace rs::storage::serializers;
//
//bool conversions::convert(const pixel_format& source, rs::file_format::file_types::pixel_format& target)
//{
//    switch(source)
//    {
//        case pixel_format::z16:
//            target = rs::file_format::file_types::pixel_format::z16; break;
//        case pixel_format::disparity16:
//            target = rs::file_format::file_types::pixel_format::disparity16; break;
//        case pixel_format::xyz32f:
//            target = rs::file_format::file_types::pixel_format::xyz32f; break;
//        case pixel_format::yuyv:
//            target = rs::file_format::file_types::pixel_format::yuyv; break;
//        case pixel_format::rgb8:
//            target = rs::file_format::file_types::pixel_format::rgb8; break;
//        case pixel_format::bgr8:
//            target = rs::file_format::file_types::pixel_format::bgr8; break;
//        case pixel_format::rgba8:
//            target = rs::file_format::file_types::pixel_format::rgba8; break;
//        case pixel_format::bgra8:
//            target = rs::file_format::file_types::pixel_format::bgra8; break;
//        case pixel_format::y8:
//            target = rs::file_format::file_types::pixel_format::y8; break;
//        case pixel_format::y16:
//            target = rs::file_format::file_types::pixel_format::y16; break;
//        case pixel_format::raw8:
//            target = rs::file_format::file_types::pixel_format::raw8; break;
//        case pixel_format::raw10:
//            target = rs::file_format::file_types::pixel_format::raw10; break;
//        case pixel_format::raw16:
//            target = rs::file_format::file_types::pixel_format::raw16; break;
//        case pixel_format::uyvy:
//            target = rs::file_format::file_types::pixel_format::uyvy; break;
//    default: return false;
//    }
//    return true;
//}
//
//bool conversions::convert(const rs::file_format::file_types::pixel_format& source,
//             pixel_format& target)
//{
//    switch(source)
//    {
//        case rs::file_format::file_types::pixel_format::z16:
//            target = pixel_format::z16; break;
//        case rs::file_format::file_types::pixel_format::disparity16:
//            target = pixel_format::disparity16; break;
//        case rs::file_format::file_types::pixel_format::xyz32f:
//            target = pixel_format::xyz32f; break;
//        case rs::file_format::file_types::pixel_format::yuyv:
//            target = pixel_format::yuyv; break;
//        case rs::file_format::file_types::pixel_format::rgb8:
//            target = pixel_format::rgb8; break;
//        case rs::file_format::file_types::pixel_format::bgr8:
//            target = pixel_format::bgr8; break;
//        case rs::file_format::file_types::pixel_format::rgba8:
//            target = pixel_format::rgba8; break;
//        case rs::file_format::file_types::pixel_format::bgra8:
//            target = pixel_format::bgra8; break;
//        case rs::file_format::file_types::pixel_format::y8:
//            target = pixel_format::y8; break;
//        case rs::file_format::file_types::pixel_format::y16:
//            target = pixel_format::y16; break;
//        case rs::file_format::file_types::pixel_format::raw8:
//            target = pixel_format::raw8; break;
//        case rs::file_format::file_types::pixel_format::raw10:
//            target = pixel_format::raw10; break;
//        case rs::file_format::file_types::pixel_format::raw16:
//            target = pixel_format::raw16; break;
//        case rs::file_format::file_types::pixel_format::uyvy:
//            target = pixel_format::uyvy; break;
//    default: return false;
//
//    }
//    return true;
//
//}
//
//bool conversions::convert(stream_type source, std::string& target)
//{
//    switch(source)
//    {
//    case stream_type::depth:
//        target = rs::file_format::file_types::stream_type::DEPTH; break;
//    case stream_type::color:
//        target = rs::file_format::file_types::stream_type::COLOR; break;
//    case stream_type::infrared:
//        target = rs::file_format::file_types::stream_type::INFRARED; break;
//    case stream_type::infrared2:
//        target = rs::file_format::file_types::stream_type::INFRARED2; break;
//    case stream_type::fisheye:
//        target = rs::file_format::file_types::stream_type::FISHEYE; break;
//    case stream_type::rectified_color:
//        target = rs::file_format::file_types::stream_type::RECTIFIED_COLOR; break;
//    default: return false;
//
//    }
//    return true;
//
//}
//
//bool conversions::convert(const std::string& source, stream_type& target)
//{
//    if(source == rs::file_format::file_types::stream_type::DEPTH)
//        target = stream_type::depth;
//    else if(source ==  rs::file_format::file_types::stream_type::COLOR)
//        target = stream_type::color;
//    else if(source ==  rs::file_format::file_types::stream_type::INFRARED)
//        target = stream_type::infrared;
//    else if(source ==  rs::file_format::file_types::stream_type::INFRARED2)
//        target = stream_type::infrared2;
//    else if(source ==  rs::file_format::file_types::stream_type::FISHEYE)
//        target = stream_type::fisheye;
//    else if(source ==  rs::file_format::file_types::stream_type::RECTIFIED_COLOR)
//        target = stream_type::rectified_color;
//    else return false;
//    return true;
//}
//
//bool conversions::convert(motion_type source, rs::file_format::file_types::motion_type& target)
//{
//    switch(source)
//    {
//    case motion_type::accel:
//        target = rs::file_format::file_types::motion_type::accel; break;
//    case motion_type::gyro:
//        target = rs::file_format::file_types::motion_type::gyro; break;
//    default: return false;
//
//    }
//    return true;
//
//}
//
//bool conversions::convert(rs::file_format::file_types::motion_type source,
//                                                            motion_type& target)
//{
//    switch(source)
//    {
//    case rs::file_format::file_types::motion_type::accel:
//        target = motion_type::accel; break;
//    case rs::file_format::file_types::motion_type::gyro:
//        target = motion_type::gyro; break;
//    default: return false;
//
//    }
//    return true;
//
//}
//
//bool conversions::convert(timestamp_domain source, rs::file_format::file_types::timestamp_domain& target)
//{
//    switch(source)
//    {
//    case timestamp_domain::camera:
//        target = rs::file_format::file_types::timestamp_domain::camera; break;
//    case timestamp_domain::microcontroller:
//        target = rs::file_format::file_types::timestamp_domain::microcontroller; break;
//    case timestamp_domain::system_time:
//        target = rs::file_format::file_types::timestamp_domain::system_time; break;
//    default: return false;
//
//    }
//    return true;
//}
//
//bool conversions::convert(rs::file_format::file_types::timestamp_domain source,
//             timestamp_domain& target)
//{
//    switch(source)
//    {
//    case rs::file_format::file_types::timestamp_domain::camera:
//        target = timestamp_domain::camera; break;
//    case rs::file_format::file_types::timestamp_domain::hardware_clock:
//        target = timestamp_domain::hardware_clock; break;
//    case rs::file_format::file_types::timestamp_domain::microcontroller:
//        target = timestamp_domain::microcontroller; break;
//    case rs::file_format::file_types::timestamp_domain::system_time:
//        target = timestamp_domain::system_time; break;
//    default: return false;
//
//    }
//    return true;
//
//}
//
//bool conversions::convert(distortion_type source, std::string& target)
//{
//    switch(source)
//    {
//    case distortion_type::distortion_ftheta:
//        target = rs::file_format::file_types::distortion::DISTORTION_FTHETA; break;
//    case distortion_type::inverse_brown_conrady:
//        target = rs::file_format::file_types::distortion::DISTORTION_INVERSE_BROWN_CONRADY; break;
//    case distortion_type::modified_brown_conrady:
//        target = rs::file_format::file_types::distortion::DISTORTION_MODIFIED_BROWN_CONRADY; break;
//    case distortion_type::none:
//        target = rs::file_format::file_types::distortion::DISTORTION_NONE; break;
//    case distortion_type::unmodified_brown_conrady:
//        target = rs::file_format::file_types::distortion::DISTORTION_UNMODIFIED_BROWN_CONRADY; break;
//    default: return false;
//
//    }
//    return true;
//}
//
//bool conversions::convert(const std::string& source, distortion_type& target)
//{
//    if(source == rs::file_format::file_types::distortion::DISTORTION_FTHETA)
//        target = distortion_type::distortion_ftheta;
//    else if(source ==  rs::file_format::file_types::distortion::DISTORTION_INVERSE_BROWN_CONRADY)
//        target = distortion_type::inverse_brown_conrady;
//    else if(source ==  rs::file_format::file_types::distortion::DISTORTION_MODIFIED_BROWN_CONRADY)
//        target = distortion_type::modified_brown_conrady;
//    else if(source ==  rs::file_format::file_types::distortion::DISTORTION_NONE)
//        target = distortion_type::none;
//    else if(source ==  rs::file_format::file_types::distortion::DISTORTION_UNMODIFIED_BROWN_CONRADY)
//        target = distortion_type::unmodified_brown_conrady;
//    else return false;
//    return true;
//}
//
//bool conversions::convert(const rs::core::intrinsics& source,
//             rs::file_format::file_types::intrinsics& target)
//{
//    target.fx = source.fx;
//    target.fy = source.fy;
//    target.ppx = source.ppx;
//    target.ppy = source.ppy;
//    if(conversions::convert(source.model, target.model) == false)
//    {
//        return false;
//    }
//
//    std::memcpy(target.coeffs, source.coeffs, sizeof(source.coeffs));
//    return true;
//}
//
//bool conversions::convert(const rs::file_format::file_types::intrinsics& source,
//                                                            rs::core::intrinsics& target)
//{
//    target.fx = source.fx;
//    target.fy = source.fy;
//    target.ppx = source.ppx;
//    target.ppy = source.ppy;
//    if(conversions::convert(source.model, target.model) == false)
//    {
//        return false;
//    }
//
//    std::memcpy(target.coeffs, source.coeffs, sizeof(source.coeffs));
//    return true;
//}
//
//rs::file_format::file_types::motion_intrinsics conversions::convert(const rs::core::motion_device_intrinsics& source)
//{
//	rs::file_format::file_types::motion_intrinsics target;
//    std::memcpy(target.bias_variances,
//                source.bias_variances,
//                sizeof(source.bias_variances));
//    std::memcpy(target.noise_variances,
//                source.noise_variances,
//                sizeof(source.noise_variances));
//    std::memcpy(target.data,
//                source.data,
//                sizeof(source.data));
//	return target;
//}
//
//rs::core::motion_device_intrinsics conversions::convert(const rs::file_format::file_types::motion_intrinsics& source)
//{
//	rs::core::motion_device_intrinsics target;
//    std::memcpy(target.bias_variances,
//                source.bias_variances,
//                sizeof(source.bias_variances));
//    std::memcpy(target.noise_variances,
//                source.noise_variances,
//                sizeof(source.noise_variances));
//    std::memcpy(target.data,
//                source.data,
//                sizeof(source.data));
//
//	return target;
//}
//
//
//bool conversions::convert(rs::core::metadata_type source, rs::file_format::file_types::metadata_type& target)
//{
//    switch(source)
//    {
//    case metadata_type::actual_exposure:
//        target = rs::file_format::file_types::metadata_type::actual_exposure; break;
//    case metadata_type::actual_fps:
//        target = rs::file_format::file_types::metadata_type::actual_fps; break;
//    default: return false;
//    }
//    return true;
//}
//
//bool conversions::convert(rs::file_format::file_types::metadata_type source, rs::core::metadata_type& target)
//{
//    switch(source)
//    {
//    case rs::file_format::file_types::metadata_type::actual_exposure:
//        target = metadata_type::actual_exposure; break;
//    case rs::file_format::file_types::metadata_type::actual_fps:
//        target = metadata_type::actual_fps; break;
//    default: return false;
//    }
//    return true;
//}
//
//rs::core::extrinsics conversions::convert(const rs::file_format::file_types::extrinsics& source)
//{
//	rs::core::extrinsics target;
//    memcpy(target.rotation, source.rotation, sizeof(source.rotation));
//    memcpy(target.translation, source.translation, sizeof(source.translation));
//
//	return target;
//}
//
//rs::file_format::file_types::extrinsics conversions::convert(const rs::core::extrinsics& source)
//{
//	rs::file_format::file_types::extrinsics target;
//
//    memcpy(target.rotation, source.rotation, sizeof(source.rotation));
//    memcpy(target.translation, source.translation, sizeof(source.translation));
//
//	return target;
//}
//
//rs::file_format::ros_data_objects::pose_info conversions::convert(const rs::core::pose& source)
//{
//    rs::file_format::ros_data_objects::pose_info target {};
//
//    target.translation.x = source.translation.x;
//    target.translation.y = source.translation.y;
//    target.translation.z = source.translation.z;
//
//    target.rotation.x = source.rotation.x;
//    target.rotation.y = source.rotation.y;
//    target.rotation.z = source.rotation.z;
//    target.rotation.w = source.rotation.w;
//
//    target.velocity.x = source.velocity.x;
//    target.velocity.y = source.velocity.y;
//    target.velocity.z = source.velocity.z;
//
//    target.angular_velocity.x = source.angular_velocity.x;
//    target.angular_velocity.y = source.angular_velocity.y;
//    target.angular_velocity.z = source.angular_velocity.z;
//
//    target.acceleration.x = source.acceleration.x;
//    target.acceleration.y = source.acceleration.y;
//    target.acceleration.z = source.acceleration.z;
//
//    target.angular_acceleration.x = source.angular_acceleration.x;
//    target.angular_acceleration.y = source.angular_acceleration.y;
//    target.angular_acceleration.z = source.angular_acceleration.z;
//
//    target.timestamp = rs::file_format::file_types::nanoseconds(source.timestamp);
//
//    target.system_timestamp = rs::file_format::file_types::nanoseconds(source.system_timestamp);
//
//    target.capture_time = rs::file_format::file_types::nanoseconds::max();
//
//    target.device_id = std::numeric_limits<uint32_t>::max();
//
//    return target;
//}
//
//rs::core::pose conversions::convert(const rs::file_format::ros_data_objects::pose_info& source)
//{
//	rs::core::pose target;
//
//	target.translation.x = source.translation.x;
//	target.translation.y = source.translation.y;
//	target.translation.z = source.translation.z;
//
//	target.rotation.x = source.rotation.x;
//	target.rotation.y = source.rotation.y;
//	target.rotation.z = source.rotation.z;
//	target.rotation.w = source.rotation.w;
//
//	target.velocity.x = source.velocity.x;
//	target.velocity.y = source.velocity.y;
//	target.velocity.z = source.velocity.z;
//
//	target.angular_velocity.x = source.angular_velocity.x;
//	target.angular_velocity.y = source.angular_velocity.y;
//	target.angular_velocity.z = source.angular_velocity.z;
//
//	target.acceleration.x = source.acceleration.x;
//	target.acceleration.y = source.acceleration.y;
//	target.acceleration.z = source.acceleration.z;
//
//	target.angular_acceleration.x = source.angular_acceleration.x;
//	target.angular_acceleration.y = source.angular_acceleration.y;
//	target.angular_acceleration.z = source.angular_acceleration.z;
//
//
//	target.timestamp = source.timestamp.count();
//	target.system_timestamp = source.system_timestamp.count();
//
//	return target;
//}
