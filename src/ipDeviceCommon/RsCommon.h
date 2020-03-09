// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#ifndef _RS_COMMON_HH
#define _RS_COMMON_HH
#pragma pack(push, 1)

struct RsNetworkHeader
{
    uint32_t frameSize;
};
struct RsMetadataHeader
{
    double timestamp;
    long long frameCounter;
    int actualFps;
    rs2_timestamp_domain timestampDomain;
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

#pragma pack(pop)
#endif
