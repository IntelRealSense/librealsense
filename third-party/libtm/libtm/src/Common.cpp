// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <stdio.h>
#include <string>
#include "Common.h"
#include "Message.h"
#include "TrackingData.h"
#include "Utils.h"

using namespace perc;

/* Description: This function prints data in hexadecimal format */
/* Parameters:  IN data - data to print                         */
/*              IN data_len - data length                       */
/* Returns:     Void                                            */
void print_data(IN unsigned char * data, IN unsigned int data_len)
{
    unsigned int i;
    for (i = 0; i < data_len; i++)
    {
        printf("0x%X ", data[i]);
        if (i % 16 == 15)
        {
            printf("\n");
        }
    }
    printf("\n\n");
}

perc::Status fwToHostStatus(perc::MESSAGE_STATUS status) 
{
    switch (status)
    {
        case MESSAGE_STATUS::INTERNAL_ERROR: return perc::Status::ERROR_FW_INTERNAL;
        case MESSAGE_STATUS::SUCCESS: return perc::Status::SUCCESS;
        case MESSAGE_STATUS::MORE_DATA_AVAILABLE: return perc::Status::SUCCESS;
        case MESSAGE_STATUS::UNKNOWN_MESSAGE_ID: return Status::NOT_SUPPORTED_BY_FW;
        case MESSAGE_STATUS::INVALID_PARAMETER: return Status::ERROR_PARAMETER_INVALID;
        case MESSAGE_STATUS::DEVICE_BUSY: return Status::DEVICE_BUSY;
        case MESSAGE_STATUS::TIMEOUT: return Status::TIMEOUT;
        case MESSAGE_STATUS::TABLE_NOT_EXIST: return Status::TABLE_NOT_EXIST;
        case MESSAGE_STATUS::TABLE_LOCKED: return Status::TABLE_LOCKED;
        case MESSAGE_STATUS::DEVICE_STOPPED: return Status::DEVICE_STOPPED;
        case MESSAGE_STATUS::TEMPERATURE_WARNING: return Status::TEMPERATURE_WARNING;
        case MESSAGE_STATUS::TEMPERATURE_STOP: return Status::TEMPERATURE_STOP;
        case MESSAGE_STATUS::CRC_ERROR: return Status::CRC_ERROR;
        case MESSAGE_STATUS::INCOMPATIBLE: return Status::INCOMPATIBLE;
        case MESSAGE_STATUS::SLAM_NO_DICTIONARY: return Status::SLAM_NO_DICTIONARY;
        case MESSAGE_STATUS::AUTH_ERROR: return Status::AUTH_ERROR;
        case MESSAGE_STATUS::DEVICE_RESET: return Status::DEVICE_RESET;
        case MESSAGE_STATUS::LIST_TOO_BIG: return Status::LIST_TOO_BIG;
        default:
            return perc::Status::COMMON_ERROR;
    }
}


perc::Status slamToHostStatus(perc::SLAM_STATUS_CODE status)
{
    switch (status)
    {
        case SLAM_STATUS_CODE::SLAM_STATUS_CODE_SUCCESS: return perc::Status::SUCCESS;
        case SLAM_STATUS_CODE::SLAM_STATUS_CODE_LOCALIZATION_DATA_SET_SUCCESS: return perc::Status::SLAM_LOCALIZATION_DATA_SET_SUCCESS;
        default:
            return perc::Status::COMMON_ERROR;
    }
}

perc::Status slamErrorToHostStatus(perc::SLAM_ERROR_CODE error)
{
    switch (error)
    {
        case SLAM_ERROR_CODE::SLAM_ERROR_CODE_NONE: return perc::Status::SUCCESS;
        case SLAM_ERROR_CODE::SLAM_ERROR_CODE_VISION: return perc::Status::SLAM_ERROR_CODE_VISION;
        case SLAM_ERROR_CODE::SLAM_ERROR_CODE_SPEED: return perc::Status::SLAM_ERROR_CODE_SPEED;
        case SLAM_ERROR_CODE::SLAM_ERROR_CODE_OTHER: return perc::Status::SLAM_ERROR_CODE_OTHER;
        default:
            return perc::Status::COMMON_ERROR;
    }
}

perc::Status controllerCalibrationToHostStatus(perc::CONTROLLER_CALIBRATION_STATUS_CODE status)
{
    switch (status)
    {
        case CONTROLLER_CALIBRATION_STATUS_CODE::CONTROLLER_CALIBRATION_STATUS_SUCCEEDED: return perc::Status::SUCCESS;
        case CONTROLLER_CALIBRATION_STATUS_CODE::CONTROLLER_CALIBRATION_STATUS_VALIDATION_FAILURE: return perc::Status::CONTROLLER_CALIBRATION_VALIDATION_FAILURE;
        case CONTROLLER_CALIBRATION_STATUS_CODE::CONTROLLER_CALIBRATION_STATUS_FLASH_ACCESS_FAILURE: return perc::Status::CONTROLLER_CALIBRATION_FLASH_ACCESS_FAILURE;
        case CONTROLLER_CALIBRATION_STATUS_CODE::CONTROLLER_CALIBRATION_STATUS_IMU_FAILURE: return perc::Status::CONTROLLER_CALIBRATION_IMU_FAILURE;
        case CONTROLLER_CALIBRATION_STATUS_CODE::CONTROLLER_CALIBRATION_STATUS_INTERNAL_FAILURE: return perc::Status::CONTROLLER_CALIBRATION_INTERNAL_FAILURE;
        default: return perc::Status::CONTROLLER_CALIBRATION_INTERNAL_FAILURE;
    }
}

TrackingData::PoseFrame poseMessageToClass(perc::pose_data msg, uint8_t sourceIndex, nsecs_t systemTime)
{
    TrackingData::PoseFrame pose;

    /* Make sure systemTime doesn't go backward */
    static nsecs_t prevSystemTime = 0;
    if (systemTime < prevSystemTime)
    {
        systemTime = prevSystemTime;
    }
    else
    {
        prevSystemTime = systemTime;
    }

    pose.sourceIndex = sourceIndex;
    pose.acceleration.set(msg.flAx, msg.flAy, msg.flAz);
    pose.angularAcceleration.set(msg.flAAX, msg.flAAY, msg.flAAZ);
    pose.angularVelocity.set(msg.flVAX, msg.flVAY, msg.flVAZ);
    pose.rotation.set(msg.flQi, msg.flQj, msg.flQk, msg.flQr);
    pose.timestamp = msg.llNanoseconds;
    pose.systemTimestamp = systemTime;
    pose.translation.set(msg.flX, msg.flY, msg.flZ);
    pose.velocity.set(msg.flVx, msg.flVy, msg.flVz);
    pose.trackerConfidence = msg.dwTrackerConfidence;
    pose.mapperConfidence = msg.dwMapperConfidence;
    pose.trackerState = msg.dwTrackerState;

    return pose;
}

TrackingData::CameraIntrinsics cameraIntrinsicsMessageToClass(perc::camera_intrinsics msg)
{
    TrackingData::CameraIntrinsics intrinsics;

    intrinsics.width = msg.dwWidth;
    intrinsics.height = msg.dwHeight;
    intrinsics.ppx = msg.flPpx;
    intrinsics.ppy = msg.flPpy;
    intrinsics.fx = msg.flFx;
    intrinsics.fy = msg.flFy;
    intrinsics.distortionModel = msg.dwDistortionModel;
    for (uint8_t i = 0; i < 5; i++)
    {
        intrinsics.coeffs[i] = msg.flCoeffs[i];
    }

    return intrinsics;
}

TrackingData::MotionIntrinsics motionIntrinsicsMessageToClass(perc::motion_intrinsics msg)
{
    TrackingData::MotionIntrinsics intrinsics;

    for (uint8_t i = 0; i < 3; i++)
    {
        for (uint8_t j = 0; j < 4; j++)
        {
            intrinsics.data[i][j] = msg.flData[i][j];
        }
    }

    for (uint8_t i = 0; i < 3; i++)
    {
        intrinsics.noiseVariances[i] = msg.flNoiseVariances[i];
    }

    for (uint8_t i = 0; i < 3; i++)
    {
        intrinsics.biasVariances[i] = msg.flBiasVariances[i];
    }

    return intrinsics;
}

motion_intrinsics motionIntrinsicsClassToMessage(perc::TrackingData::MotionIntrinsics intrinsics)
{
    motion_intrinsics msg;

    for (uint8_t i = 0; i < 3; i++)
    {
        for (uint8_t j = 0; j < 4; j++)
        {
            msg.flData[i][j] = intrinsics.data[i][j];
        }
    }

    for (uint8_t i = 0; i < 3; i++)
    {
        msg.flNoiseVariances[i] = intrinsics.noiseVariances[i];
    }

    for (uint8_t i = 0; i < 3; i++)
    {
        msg.flBiasVariances[i] = intrinsics.biasVariances[i];
    }

    return msg;
}

TrackingData::SensorExtrinsics sensorExtrinsicsMessageToClass(perc::sensor_extrinsics msg)
{
    TrackingData::SensorExtrinsics extrinsics;

    for (uint8_t i = 0; i < 9; i++)
    {
        extrinsics.rotation[i] = msg.flRotation[i];
    }

    for (uint8_t i = 0; i < 3; i++)
    {
        extrinsics.translation[i] = msg.flTranslation[i];
    }

    extrinsics.referenceSensorId = msg.bReferenceSensorID;

    return extrinsics;
}


std::string messageCodeToString(libusb_transfer_type type, uint16_t code)
{
    switch (type)
    {
        case LIBUSB_TRANSFER_TYPE_BULK:
            switch (code)
            {
                case DEV_GET_DEVICE_INFO:                   return "DEV_GET_DEVICE_INFO";
                case DEV_GET_TIME:                          return "DEV_GET_TIME";
                case DEV_GET_AND_CLEAR_EVENT_LOG:           return "DEV_GET_AND_CLEAR_EVENT_LOG";
                case DEV_GET_SUPPORTED_RAW_STREAMS:         return "DEV_GET_SUPPORTED_RAW_STREAMS";
                case DEV_RAW_STREAMS_CONTROL:               return "DEV_RAW_STREAMS_CONTROL";
                case DEV_GET_CAMERA_INTRINSICS:             return "DEV_GET_CAMERA_INTRINSICS";
                case DEV_GET_MOTION_INTRINSICS:             return "DEV_GET_MOTION_INTRINSICS";
                case DEV_GET_EXTRINSICS:                    return "DEV_GET_EXTRINSICS";
                case DEV_SET_CAMERA_INTRINSICS:             return "DEV_SET_CAMERA_INTRINSICS";
                case DEV_SET_MOTION_INTRINSICS:             return "DEV_SET_MOTION_INTRINSICS";
                case DEV_SET_EXTRINSICS:                    return "DEV_SET_EXTRINSICS";
                case DEV_LOG_CONTROL:                       return "DEV_LOG_CONTROL";
                case DEV_STREAM_CONFIG:                     return "DEV_STREAM_CONFIG";
                case DEV_RAW_STREAMS_PLAYBACK_CONTROL:      return "DEV_RAW_STREAMS_PLAYBACK_CONTROL";
                case DEV_READ_EEPROM:                       return "DEV_READ_EEPROM";
                case DEV_WRITE_EEPROM:                      return "DEV_WRITE_EEPROM";
                case DEV_SAMPLE:                            return "DEV_SAMPLE";
                case DEV_START:                             return "DEV_START";
                case DEV_STOP:                              return "DEV_STOP";
                case DEV_STATUS:                            return "DEV_STATUS";   
                case DEV_GET_POSE:                          return "DEV_GET_POSE";
                case DEV_EXPOSURE_MODE_CONTROL:             return "DEV_EXPOSURE_MODE_CONTROL";
                case DEV_SET_EXPOSURE:                      return "DEV_SET_EXPOSURE";
                case DEV_GET_TEMPERATURE:                   return "DEV_GET_TEMPERATURE";
                case DEV_SET_TEMPERATURE_THRESHOLD:         return "DEV_SET_TEMPERATURE_THRESHOLD";
                case DEV_SET_GEO_LOCATION:                  return "DEV_SET_GEO_LOCATION";
                case DEV_FLUSH:                             return "DEV_FLUSH";
                case DEV_FIRMWARE_UPDATE:                   return "DEV_FIRMWARE_UPDATE";
                case DEV_GPIO_CONTROL:                      return "DEV_GPIO_CONTROL";
                case DEV_TIMEOUT_CONFIGURATION:             return "DEV_TIMEOUT_CONFIGURATION";
                case DEV_SNAPSHOT:                          return "DEV_SNAPSHOT";
                case DEV_READ_CONFIGURATION:                return "DEV_READ_CONFIGURATION";
                case DEV_WRITE_CONFIGURATION:               return "DEV_WRITE_CONFIGURATION";
                case DEV_RESET_CONFIGURATION:               return "DEV_RESET_CONFIGURATION";
                case DEV_LOCK_CONFIGURATION:                return "DEV_LOCK_CONFIGURATION";
                case DEV_LOCK_EEPROM:                       return "DEV_LOCK_EEPROM";
                case DEV_SET_LOW_POWER_MODE:                return "DEV_SET_LOW_POWER_MODE";
                case SLAM_STATUS:                           return "SLAM_STATUS";
                case SLAM_GET_OCCUPANCY_MAP_TILES:          return "SLAM_GET_OCCUPANCY_MAP_TILES";
                case SLAM_GET_LOCALIZATION_DATA:            return "SLAM_GET_LOCALIZATION_DATA";
                case SLAM_SET_LOCALIZATION_DATA_STREAM:     return "SLAM_SET_LOCALIZATION_DATA_STREAM";
                case SLAM_SET_6DOF_INTERRUPT_RATE:          return "SLAM_SET_6DOF_INTERRUPT_RATE";
                case SLAM_6DOF_CONTROL:                     return "SLAM_6DOF_CONTROL";
                case SLAM_OCCUPANCY_MAP_CONTROL:            return "SLAM_OCCUPANCY_MAP_CONTROL";
                case SLAM_RESET_LOCALIZATION_DATA:          return "SLAM_RESET_LOCALIZATION_DATA";
                case SLAM_GET_LOCALIZATION_DATA_STREAM:     return "SLAM_GET_LOCALIZATION_DATA_STREAM";
                case SLAM_SET_STATIC_NODE:                  return "SLAM_SET_STATIC_NODE";
                case SLAM_GET_STATIC_NODE:                  return "SLAM_GET_STATIC_NODE";
                case SLAM_APPEND_CALIBRATION:               return "SLAM_APPEND_CALIBRATION";
                case SLAM_RELOCALIZATION_EVENT:             return "SLAM_RELOCALIZATION_EVENT";
                case CONTROLLER_POSE_CONTROL:               return "CONTROLLER_POSE_CONTROL";
                case CONTROLLER_STATUS_CHANGE_EVENT:        return "CONTROLLER_STATUS_CHANGE_EVENT";
                case CONTROLLER_DEVICE_CONNECT:             return "CONTROLLER_DEVICE_CONNECT";
                case CONTROLLER_DEVICE_DISCOVERY_EVENT:     return "CONTROLLER_DEVICE_DISCOVERY_EVENT";
                case CONTROLLER_DEVICE_DISCONNECT:          return "CONTROLLER_DEVICE_DISCONNECT";
                case CONTROLLER_READ_ASSOCIATED_DEVICES:    return "CONTROLLER_READ_ASSOCIATED_DEVICES";
                case CONTROLLER_WRITE_ASSOCIATED_DEVICES:   return "CONTROLLER_WRITE_ASSOCIATED_DEVICES";
                case CONTROLLER_DEVICE_DISCONNECTED_EVENT:  return "CONTROLLER_DEVICE_DISCONNECTED_EVENT";
                case CONTROLLER_DEVICE_CONNECTED_EVENT:     return "CONTROLLER_DEVICE_CONNECTED_EVENT";
                case CONTROLLER_RSSI_TEST_CONTROL:          return "CONTROLLER_RSSI_TEST_CONTROL";
                case CONTROLLER_SEND_DATA:                  return "CONTROLLER_SEND_DATA";
                case CONTROLLER_START_CALIBRATION:          return "CONTROLLER_START_CALIBRATION";
                case CONTROLLER_CALIBRATION_STATUS_EVENT:   return "CONTROLLER_CALIBRATION_STATUS_EVENT";
                case CONTROLLER_DEVICE_LED_INTENSITY_EVENT: return "CONTROLLER_DEVICE_LED_INTENSITY_EVENT";
                case DEV_ERROR:                             return "DEV_ERROR";
                case SLAM_ERROR:                            return "SLAM_ERROR";
                case CONTROLLER_ERROR:                      return "CONTROLLER_ERROR";
                default:                                    return "UNKNOWN MESSAGE CODE";
            }

        case LIBUSB_TRANSFER_TYPE_CONTROL:
            switch (code)
            {
                case  CONTROL_USB_RESET:                return "CONTROL_USB_RESET";
                case  CONTROL_GET_INTERFACE_VERSION:    return "CONTROL_GET_INTERFACE_VERSION";
                default:                                return "UNKNOWN MESSAGE CODE";
            }
        default: return "UNKNOWN TRANSFER TYPE";
    }
}


std::string statusCodeToString(perc::MESSAGE_STATUS status)
{
    switch (status)
    {
        case MESSAGE_STATUS::SUCCESS:                  return "SUCCESS";
        case MESSAGE_STATUS::UNKNOWN_MESSAGE_ID:       return "UNKNOWN_MESSAGE_ID";
        case MESSAGE_STATUS::INVALID_REQUEST_LEN:      return "INVALID_REQUEST_LEN";
        case MESSAGE_STATUS::INVALID_PARAMETER:        return "INVALID_PARAMETER";
        case MESSAGE_STATUS::INTERNAL_ERROR:           return "INTERNAL_ERROR";
        case MESSAGE_STATUS::UNSUPPORTED:              return "UNSUPPORTED";
        case MESSAGE_STATUS::LIST_TOO_BIG:             return "LIST_TOO_BIG";
        case MESSAGE_STATUS::MORE_DATA_AVAILABLE:      return "MORE_DATA_AVAILABLE";
        case MESSAGE_STATUS::DEVICE_BUSY:              return "DEVICE_BUSY";
        case MESSAGE_STATUS::TIMEOUT:                  return "TIMEOUT";
        case MESSAGE_STATUS::TABLE_NOT_EXIST:          return "TABLE_NOT_EXIST";
        case MESSAGE_STATUS::TABLE_LOCKED:             return "TABLE_LOCKED";
        case MESSAGE_STATUS::DEVICE_STOPPED:           return "DEVICE_STOPPED";
        case MESSAGE_STATUS::TEMPERATURE_WARNING:      return "TEMPERATURE_WARNING";
        case MESSAGE_STATUS::TEMPERATURE_STOP:         return "TEMPERATURE_STOP";
        case MESSAGE_STATUS::CRC_ERROR:                return "CRC_ERROR";
        case MESSAGE_STATUS::INCOMPATIBLE:             return "INCOMPATIBLE";
        case MESSAGE_STATUS::SLAM_NO_DICTIONARY:       return "SLAM_NO_DICTIONARY";
        default:                                       return "UNKNOWN STATUS";
    } 
}

std::string slamStatusCodeToString(SLAM_STATUS_CODE status)
{
    switch (status)
    {
        case SLAM_STATUS_CODE_SUCCESS:                       return "SLAM_STATUS_CODE_SUCCESS";
        case SLAM_STATUS_CODE_LOCALIZATION_DATA_SET_SUCCESS: return "SLAM_STATUS_CODE_LOCALIZATION_DATA_SET_SUCCESS";
        default:                                             return "UNKNOWN STATUS";
    }
}

std::string slamErrorCodeToString(SLAM_ERROR_CODE error)
{
    switch (error)
    {
        case SLAM_ERROR_CODE_NONE:   return "SLAM_ERROR_CODE_NONE";
        case SLAM_ERROR_CODE_VISION: return "SLAM_ERROR_CODE_VISION";
        case SLAM_ERROR_CODE_SPEED:  return "SLAM_ERROR_CODE_SPEED";
        case SLAM_ERROR_CODE_OTHER:  return "SLAM_ERROR_CODE_OTHER";
        default:                     return "UNKNOWN ERROR";
    }
}

std::string controllerCalibrationStatusCodeToString(CONTROLLER_CALIBRATION_STATUS_CODE status)
{
    switch (status)
    {
        case CONTROLLER_CALIBRATION_STATUS_SUCCEEDED:            return "CONTROLLER_CALIBRATION_STATUS_SUCCEEDED";
        case CONTROLLER_CALIBRATION_STATUS_VALIDATION_FAILURE:   return "CONTROLLER_CALIBRATION_STATUS_VALIDATION_FAILURE";
        case CONTROLLER_CALIBRATION_STATUS_FLASH_ACCESS_FAILURE: return "CONTROLLER_CALIBRATION_STATUS_FLASH_ACCESS_FAILURE";
        case CONTROLLER_CALIBRATION_STATUS_IMU_FAILURE:          return "CONTROLLER_CALIBRATION_STATUS_IMU_FAILURE";
        case CONTROLLER_CALIBRATION_STATUS_INTERNAL_FAILURE:     return "CONTROLLER_CALIBRATION_STATUS_INTERNAL_FAILURE";
        default:                                                 return "UNKNOWN ERROR";
    }
}

std::string sensorToString(perc::SensorType sensorType)
{
    switch (sensorType)
    {
        case  Fisheye:          return "Fisheye";
        case  Gyro:             return "Gyro";
        case  Accelerometer:    return "Accelerometer";
        case  Controller:       return "Controller";
        case  Rssi:             return "Rssi";
        case  Velocimeter:      return "Velocimeter";
        default:                return "Unknown";
    }
}

