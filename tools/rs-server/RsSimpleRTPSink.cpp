// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "RsSimpleRTPSink.h"
#include "RsDevice.hh"
#include <algorithm>
#include <compression/CompressionFactory.h>
#include <iostream>
#include <sstream>
#include <string>

RsSimpleRTPSink* RsSimpleRTPSink::createNew(UsageEnvironment& t_env,
                                            Groupsock* t_RTPgs,
                                            unsigned char t_rtpPayloadFormat,
                                            unsigned t_rtpTimestampFrequency,
                                            char const* t_sdpMediaTypeString,
                                            char const* t_rtpPayloadFormatName,
                                            rs2::video_stream_profile& t_videoStream,
                                            std::shared_ptr<RsDevice> device,
                                            unsigned t_numChannels,
                                            Boolean t_allowMultipleFramesPerPacket,
                                            Boolean t_doNormalMBitRule)
{
    CompressionFactory::getIsEnabled() = IS_COMPRESSION_ENABLED;
    return new RsSimpleRTPSink(t_env, t_RTPgs, t_rtpPayloadFormat, t_rtpTimestampFrequency, t_sdpMediaTypeString, t_rtpPayloadFormatName, t_videoStream, device, t_numChannels, t_allowMultipleFramesPerPacket, t_doNormalMBitRule);
}

std::string getSdpLineForField(const char* t_name, int t_val)
{
    std::ostringstream oss;
    oss << t_name << "=" << t_val << ";";
    return oss.str();
}

std::string getSdpLineForField(const char* t_name, const char* t_val)
{
    std::ostringstream oss;
    oss << t_name << "=" << t_val << ";";
    return oss.str();
}

std::string extrinsics_to_string(rs2_extrinsics extrinsics)
{
    std::string str;
    str.append("rotation:");
    for(float r : extrinsics.rotation)
    {
        str.append(std::to_string(r));
        str.append(",");
    }
    str.pop_back();
    str.append("translation:");
    for(float r : extrinsics.translation)
    {
        str.append(std::to_string(r));
        str.append(",");
    }
    str.pop_back();

    return str;
}

std::string get_extrinsics_string_per_stream(std::shared_ptr<RsDevice> device, rs2::video_stream_profile stream)
{

    if(device == nullptr)
        return "";

    std::string str;

    for(auto relation : device.get()->minimal_extrinsics_map)
    {
        //check at map for this stream relations
        if(relation.first.first == (RsDevice::getPhysicalSensorUniqueKey(stream.stream_type(), stream.stream_index())))
        {
            str.append("<to_sensor_");
            //write the 'to' stream key
            str.append(std::to_string(relation.first.second));
            str.append(">");
            str.append(extrinsics_to_string(relation.second));
            str.append("&");
        }
    }

    return str;
}

std::string getSdpLineForVideoStream(rs2::video_stream_profile& t_videoStream, std::shared_ptr<RsDevice> device)
{
    std::string str;
    str.append(getSdpLineForField("width", t_videoStream.width()));
    str.append(getSdpLineForField("height", t_videoStream.height()));
    str.append(getSdpLineForField("format", t_videoStream.format()));
    str.append(getSdpLineForField("uid", t_videoStream.unique_id()));
    str.append(getSdpLineForField("fps", t_videoStream.fps()));
    str.append(getSdpLineForField("stream_index", t_videoStream.stream_index()));
    str.append(getSdpLineForField("stream_type", t_videoStream.stream_type()));
    str.append(getSdpLineForField("bpp", RsSensor::getStreamProfileBpp(t_videoStream.format())));
    str.append(getSdpLineForField("cam_serial_num", device.get()->getDevice().get_info(RS2_CAMERA_INFO_SERIAL_NUMBER)));
    str.append(getSdpLineForField("usb_type", device.get()->getDevice().get_info(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR)));
    str.append(getSdpLineForField("compression", CompressionFactory::getIsEnabled()));

    str.append(getSdpLineForField("ppx", t_videoStream.get_intrinsics().ppx));
    str.append(getSdpLineForField("ppy", t_videoStream.get_intrinsics().ppy));
    str.append(getSdpLineForField("fx", t_videoStream.get_intrinsics().fx));
    str.append(getSdpLineForField("fy", t_videoStream.get_intrinsics().fy));
    str.append(getSdpLineForField("model", t_videoStream.get_intrinsics().model));

    for(size_t i = 0; i < 5; i++)
    {
        str.append(getSdpLineForField("coeff_" + i, t_videoStream.get_intrinsics().coeffs[i]));
    }

    str.append(getSdpLineForField("extrinsics", get_extrinsics_string_per_stream(device, t_videoStream).c_str()));

    std::string name = device.get()->getDevice().get_info(RS2_CAMERA_INFO_NAME);
    // We don't want to sent spaces over SDP , replace all spaces with '^'
    std::replace(name.begin(), name.end(), ' ', '^');
    str.append(getSdpLineForField("cam_name", name.c_str()));

    return str;
}

RsSimpleRTPSink ::RsSimpleRTPSink(UsageEnvironment& t_env,
                                  Groupsock* t_RTPgs,
                                  unsigned char t_rtpPayloadFormat,
                                  unsigned t_rtpTimestampFrequency,
                                  char const* t_sdpMediaTypeString,
                                  char const* t_rtpPayloadFormatName,
                                  rs2::video_stream_profile& t_videoStream,
                                  std::shared_ptr<RsDevice> device,
                                  unsigned t_numChannels,
                                  Boolean t_allowMultipleFramesPerPacket,
                                  Boolean t_doNormalMBitRule)
    : SimpleRTPSink(t_env, t_RTPgs, t_rtpPayloadFormat, t_rtpTimestampFrequency, t_sdpMediaTypeString, t_rtpPayloadFormatName, t_numChannels, t_allowMultipleFramesPerPacket, t_doNormalMBitRule)
{
    // Then use this 'config' string to construct our "a=fmtp:" SDP line:
    unsigned fmtpSDPLineMaxSize = SDP_MAX_LINE_LENGHT;
    m_fFmtpSDPLine = new char[fmtpSDPLineMaxSize];
    std::string sdpStr = getSdpLineForVideoStream(t_videoStream, device);
    sprintf(m_fFmtpSDPLine, "a=fmtp:%d;%s\r\n", rtpPayloadType(), sdpStr.c_str());
}

char const* RsSimpleRTPSink::auxSDPLine()
{
    return m_fFmtpSDPLine;
}
