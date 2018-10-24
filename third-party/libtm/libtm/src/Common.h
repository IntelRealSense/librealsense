// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include "TrackingCommon.h"
#include "TrackingData.h"
#include "Message.h"
#include "Device.h"
#include "Utils.h"
#include <type_traits>

void print_data(IN unsigned char * data, IN unsigned int data_len);

extern perc::Status fwToHostStatus(perc::MESSAGE_STATUS status);
perc::Status slamToHostStatus(perc::SLAM_STATUS_CODE status);
perc::Status slamErrorToHostStatus(perc::SLAM_ERROR_CODE error);
perc::Status controllerCalibrationToHostStatus(perc::CONTROLLER_CALIBRATION_STATUS_CODE status);
std::string messageCodeToString(libusb_transfer_type type, uint16_t code);
std::string statusCodeToString(perc::MESSAGE_STATUS status);
std::string slamStatusCodeToString(perc::SLAM_STATUS_CODE status);
std::string slamErrorCodeToString(perc::SLAM_ERROR_CODE error);
std::string controllerCalibrationStatusCodeToString(perc::CONTROLLER_CALIBRATION_STATUS_CODE status);
perc::TrackingData::PoseFrame poseMessageToClass(perc::pose_data msg, uint8_t sensorIndex, nsecs_t systemTime);
perc::TrackingData::CameraIntrinsics cameraIntrinsicsMessageToClass(perc::camera_intrinsics msg);
perc::TrackingData::MotionIntrinsics motionIntrinsicsMessageToClass(perc::motion_intrinsics msg);
perc::motion_intrinsics motionIntrinsicsClassToMessage(perc::TrackingData::MotionIntrinsics intrinsics);
perc::TrackingData::SensorExtrinsics sensorExtrinsicsMessageToClass(perc::sensor_extrinsics msg);
std::string sensorToString(perc::SensorType sensorType);

template<typename E>
constexpr auto toUnderlying(E e) -> typename std::underlying_type<E>::type
{
    return static_cast<typename std::underlying_type<E>::type>(e);
}

template< typename T >
struct arrayDeleter
{
    void operator ()(T const * p)
    {
        delete[] p;
    }
};

