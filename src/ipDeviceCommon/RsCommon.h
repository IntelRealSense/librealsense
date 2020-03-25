// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "string.h"
#include <librealsense2/rs.hpp>

#pragma pack(push, 1)

union RsNetworkHeader { //IMPORTANT:: RsNetworkHeader should be alligned to 16 bytes, this enables frame data to start on 16 bit alligned address
    char maxHeaderSize[128];
    struct
    {
        uint32_t frameSize;
    } data;
};

union RsMetadataHeader { //IMPORTANT:: RsNetworkHeader should be alligned to 16 bytes, this enables frame data to start on 16 bit alligned address
    char maxHeaderSize[128];
    struct 
    {
        double timestamp;
        long long frameCounter;
        int actualFps;
        rs2_timestamp_domain timestampDomain;
    } data;
}; 

struct RsFrameHeader
{
    RsNetworkHeader networkHeader;
    RsMetadataHeader metadataHeader;
};

struct IpDeviceControlData
{
    int sensorId;
    rs2_option option;
    rs2::option_range range;
};

const std::string STEREO_SENSOR_NAME("Stereo Module");
const std::string RGB_SENSOR_NAME("RGB Camera");
const std::string RS_MEDIA_TYPE("RS_VIDEO");
const std::string RS_PAYLOAD_FORMAT("RS_FORMAT");
const int MAX_WIDTH = 1280;
const int MAX_HEIGHT = 720;
const int MAX_BPP = 3;
const int MAX_FRAME_SIZE = MAX_WIDTH * MAX_HEIGHT * MAX_BPP;
const int MAX_MESSAGE_SIZE = MAX_FRAME_SIZE + sizeof(RsFrameHeader);
const unsigned int SDP_MAX_LINE_LENGHT = 4000;
const unsigned int RTP_TIMESTAMP_FREQ = 90000;

#pragma pack(pop)
