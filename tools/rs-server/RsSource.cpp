// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "RsSource.hh"
#include "BasicUsageEnvironment.hh"
#include "RsStatistics.h"
#include <GroupsockHelper.hh>
#include <cassert>
#include <compression/CompressionFactory.h>
#include <ipDeviceCommon/RsCommon.h>
#include <ipDeviceCommon/Statistic.h>
#include <librealsense2/h/rs_sensor.h>

RsDeviceSource* RsDeviceSource::createNew(UsageEnvironment& t_env, rs2::video_stream_profile& t_videoStreamProfile, rs2::frame_queue& t_queue)
{
    return new RsDeviceSource(t_env, t_videoStreamProfile, t_queue);
}

RsDeviceSource::RsDeviceSource(UsageEnvironment& t_env, rs2::video_stream_profile& t_videoStreamProfile, rs2::frame_queue& t_queue)
    : FramedSource(t_env)
{
    m_framesQueue = &t_queue;
    m_streamProfile = &t_videoStreamProfile;
}

RsDeviceSource::~RsDeviceSource() {}

void RsDeviceSource::doGetNextFrame()
{
    // This function is called (by our 'downstream' object) when it asks for new data.

    rs2::frame frame;
    try
    {
        if(!m_framesQueue->poll_for_frame(&frame))
        {
            nextTask() = envir().taskScheduler().scheduleDelayedTask(0, (TaskFunc*)waitForFrame, this);
        }
        else
        {
            frame.keep();
            deliverRSFrame(&frame);
        }
    }
    catch(const std::exception& e)
    {
        envir() << "RsDeviceSource: " << e.what() << '\n';
    }
}

void RsDeviceSource::handleWaitForFrame()
{
    // If a new frame of data is immediately available to be delivered, then do this now:
    rs2::frame frame;
    try
    {
        if(!(getFramesQueue()->poll_for_frame(&frame)))
        {
            nextTask() = envir().taskScheduler().scheduleDelayedTask(0, (TaskFunc*)RsDeviceSource::waitForFrame, this);
        }
        else
        {
            frame.keep();
            deliverRSFrame(&frame);
        }
    }
    catch(const std::exception& e)
    {
        envir() << "RsDeviceSource: " << e.what() << '\n';
    }
}

// The following is called after each delay between packet sends:
void RsDeviceSource::waitForFrame(RsDeviceSource* t_deviceSource)
{
    t_deviceSource->handleWaitForFrame();
}

void RsDeviceSource::deliverRSFrame(rs2::frame* t_frame)
{
    if(!isCurrentlyAwaitingData())
    {
        envir() << "isCurrentlyAwaitingData returned false\n";
        return; // we're not ready for the data yet
    }

    unsigned newFrameSize = t_frame->get_data_size();

    gettimeofday(&fPresentationTime, NULL); // If you have a more accurate time - e.g., from an encoder - then use that instead.
    RsFrameHeader header;
    unsigned char* data;
    if(CompressionFactory::isCompressionSupported(t_frame->get_profile().format(), t_frame->get_profile().stream_type()))
    {
        fFrameSize = ((int*)t_frame->get_data())[0];
        data = (unsigned char*)t_frame->get_data() + sizeof(int);
    }
    else
    {
        fFrameSize = t_frame->get_data_size();
        data = (unsigned char*)t_frame->get_data();
    }
    memmove(fTo + sizeof(RsFrameHeader), data, fFrameSize);
    fFrameSize += sizeof(RsMetadataHeader);
    header.networkHeader.data.frameSize = fFrameSize;
    fFrameSize += sizeof(RsNetworkHeader);
    if(t_frame->supports_frame_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP))
    {
        header.metadataHeader.data.timestamp = t_frame->get_frame_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP) / 1000;
    }
    else
    {
        header.metadataHeader.data.timestamp = t_frame->get_timestamp();
    }

    if(t_frame->supports_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER))
    {
        header.metadataHeader.data.frameCounter = t_frame->get_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER);
    }
    else
    {
        header.metadataHeader.data.frameCounter = t_frame->get_frame_number();
    }

    if(t_frame->supports_frame_metadata(RS2_FRAME_METADATA_ACTUAL_FPS))
    {
        header.metadataHeader.data.actualFps = t_frame->get_frame_metadata(RS2_FRAME_METADATA_ACTUAL_FPS);
    }

    header.metadataHeader.data.timestampDomain = t_frame->get_frame_timestamp_domain();

    memmove(fTo, &header, sizeof(header));

    // After delivering the data, inform the reader that it is now available:
    FramedSource::afterGetting(this);
}
