// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#ifndef _RS_SIMPLE_RTP_SINK_HH
#define _RS_SIMPLE_RTP_SINK_HH


#define SDP_MAX_LINE_LENGHT 4000

#define RTP_TIMESTAMP_FREQ 90000

#include <librealsense2/hpp/rs_internal.hpp>
#include "RsDevice.hh"
#include "liveMedia.hh"

class RsSimpleRTPSink : public SimpleRTPSink
{
public:
        static RsSimpleRTPSink *createNew(UsageEnvironment &env, Groupsock *RTPgs,
                                                                          unsigned char rtpPayloadFormat,
                                                                          unsigned rtpTimestampFrequency,
                                                                          char const *sdpMediaTypeString,
                                                                          char const *rtpPayloadFormatName,
                                                                          rs2::video_stream_profile &video_stream,
                                                                          std::shared_ptr<RsDevice> device,
                                                                          unsigned numChannels = 1,
                                                                          Boolean allowMultipleFramesPerPacket = True,
                                                                          Boolean doNormalMBitRule = True);

protected:
        RsSimpleRTPSink(UsageEnvironment &env, Groupsock *RTPgs,
                                        unsigned char rtpPayloadFormat,
                                        unsigned rtpTimestampFrequency,
                                        char const *sdpMediaTypeString,
                                        char const *rtpPayloadFormatName,
                                        rs2::video_stream_profile &video_stream,
                                        std::shared_ptr<RsDevice> device,
                                        unsigned numChannels = 1,
                                        Boolean allowMultipleFramesPerPacket = True,
                                        Boolean doNormalMBitRule = True);

private:
        char *m_fFmtpSDPLine;
        virtual char const *auxSDPLine(); // for the "a=fmtp:" SDP line
};

#endif //_RS_SIMPLE_RTP_SINK_HH
