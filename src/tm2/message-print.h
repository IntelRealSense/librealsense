// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <iostream>

#include "t265-messages.h"
using namespace t265;

#define map_str(X) {X, #X}
std::map<BULK_MESSAGE_ID, std::string> bulk_message_names = {
       map_str(DEV_GET_DEVICE_INFO),
       map_str(DEV_GET_TIME),
       map_str(DEV_GET_AND_CLEAR_EVENT_LOG),
       map_str(DEV_GET_SUPPORTED_RAW_STREAMS),
       map_str(DEV_RAW_STREAMS_CONTROL),
       map_str(DEV_GET_CAMERA_INTRINSICS),
       map_str(DEV_GET_MOTION_INTRINSICS),
       map_str(DEV_GET_EXTRINSICS),
       map_str(DEV_SET_CAMERA_INTRINSICS),
       map_str(DEV_SET_MOTION_INTRINSICS),
       map_str(DEV_SET_EXTRINSICS),
       map_str(DEV_LOG_CONTROL),
       map_str(DEV_STREAM_CONFIG),
       map_str(DEV_RAW_STREAMS_PLAYBACK_CONTROL),
       map_str(DEV_READ_EEPROM),
       map_str(DEV_WRITE_EEPROM),
       map_str(DEV_SAMPLE),
       map_str(DEV_START),
       map_str(DEV_STOP),
       map_str(DEV_STATUS),
       map_str(DEV_GET_POSE),
       map_str(DEV_EXPOSURE_MODE_CONTROL),
       map_str(DEV_SET_EXPOSURE),
       map_str(DEV_GET_TEMPERATURE),
       map_str(DEV_SET_TEMPERATURE_THRESHOLD),
       map_str(DEV_FIRMWARE_UPDATE),
       map_str(DEV_GPIO_CONTROL),
       map_str(DEV_TIMEOUT_CONFIGURATION),
       map_str(DEV_SNAPSHOT),
       map_str(DEV_READ_CONFIGURATION),
       map_str(DEV_WRITE_CONFIGURATION),
       map_str(DEV_RESET_CONFIGURATION),
       map_str(DEV_LOCK_CONFIGURATION),
       map_str(DEV_LOCK_EEPROM),
       map_str(DEV_SET_LOW_POWER_MODE),
       map_str(SLAM_STATUS),
       map_str(SLAM_GET_OCCUPANCY_MAP_TILES),
       map_str(SLAM_GET_LOCALIZATION_DATA),
       map_str(SLAM_SET_LOCALIZATION_DATA_STREAM),
       map_str(SLAM_SET_6DOF_INTERRUPT_RATE),
       map_str(SLAM_6DOF_CONTROL),
       map_str(SLAM_OCCUPANCY_MAP_CONTROL),
       map_str(SLAM_GET_LOCALIZATION_DATA_STREAM),
       map_str(SLAM_SET_STATIC_NODE),
       map_str(SLAM_GET_STATIC_NODE),
       map_str(SLAM_APPEND_CALIBRATION),
       map_str(SLAM_CALIBRATION),
       map_str(SLAM_RELOCALIZATION_EVENT),
       map_str(DEV_ERROR),
       map_str(SLAM_ERROR),
       map_str(MAX_MESSAGE_ID),
};

std::map<CONTROL_MESSAGE_ID, std::string> control_message_names = {
        map_str(CONTROL_USB_RESET),
        map_str(CONTROL_GET_INTERFACE_VERSION),
        map_str(MAX_CONTROL_ID),
};


template <typename T>
static std::string message_name(const T & header) {
    auto id = BULK_MESSAGE_ID(header.wMessageID);
    if(bulk_message_names.count(id) != 0) {
        return bulk_message_names.at(id);
    }
    std::stringstream s;
    s << std::hex << "UNKNOWN ID 0x" << id;
    return s.str();
}

std::map<MESSAGE_STATUS, std::string> message_status_names = {
       map_str(SUCCESS),
       map_str(UNKNOWN_MESSAGE_ID),
       map_str(INVALID_REQUEST_LEN),
       map_str(INVALID_PARAMETER),
       map_str(INTERNAL_ERROR),
       map_str(UNSUPPORTED),
       map_str(LIST_TOO_BIG),
       map_str(MORE_DATA_AVAILABLE),
       map_str(DEVICE_BUSY),
       map_str(TIMEOUT),
       map_str(TABLE_NOT_EXIST),
       map_str(TABLE_LOCKED),
       map_str(DEVICE_STOPPED),
       map_str(TEMPERATURE_WARNING),
       map_str(TEMPERATURE_STOP),
       map_str(CRC_ERROR),
       map_str(INCOMPATIBLE),
       map_str(AUTH_ERROR),
       map_str(DEVICE_RESET),
       map_str(SLAM_NO_DICTIONARY),
};

template <typename T>
static std::string status_name(const T & header) {
    auto id = MESSAGE_STATUS(header.wStatus);
    if(message_status_names.count(id) != 0) {
        return message_status_names.at(id);
    }
    std::stringstream s;
    s << std::hex << "UNKNOWN STATUS at 0x" << id;
    return s.str();
}

