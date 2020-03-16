// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "RsSource.hh"
#include <GroupsockHelper.hh>
#include <librealsense2/h/rs_sensor.h>
#include "BasicUsageEnvironment.hh"
#include <ipDeviceCommon/RsCommon.h>
#include <cassert>
#include <ipDeviceCommon/Statistic.h>
#include "RsStatistics.h"
#include <compression/CompressionFactory.h>


RsDeviceSource *
    RsDeviceSource::createNew(UsageEnvironment &t_env, rs2::video_stream_profile &t_videoStreamProfile, rs2::frame_queue &t_queue)
{
  return new RsDeviceSource(t_env, t_videoStreamProfile, t_queue);
}

RsDeviceSource::RsDeviceSource(UsageEnvironment &t_env, rs2::video_stream_profile &t_videoStreamProfile, rs2::frame_queue &t_queue) : FramedSource(t_env)
{
  m_framesQueue = &t_queue;
  m_streamProfile = &t_videoStreamProfile;
#ifdef STATISTICS
  if(Statistic::getStatisticStreams().find(t_videoStreamProfile.stream_type()) == Statistic::getStatisticStreams().end()) {
      Statistic::getStatisticStreams().insert(std::pair<int,StreamStatistic *>(t_videoStreamProfile.stream_type(),new StreamStatistic()));
  } 
  //todo:change to uid instead of type.
#endif
}

RsDeviceSource::~RsDeviceSource()
{
}

void RsDeviceSource::doGetNextFrame()
{
  // This function is called (by our 'downstream' object) when it asks for new data.
  m_getFrame = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> timeSpan = std::chrono::duration_cast<std::chrono::duration<double>>(m_getFrame - RsStatistics::getResetPacketStartTp());
  
  std::chrono::high_resolution_clock::time_point* tp = &RsStatistics::getSendPacketTp();
  m_networkTimeSpan = std::chrono::duration_cast<std::chrono::duration<double>>(m_getFrame-*tp);
  *tp = std::chrono::high_resolution_clock::now();

  if (0) //TODO:: to check if needed
  { // the source stops being readable
    handleClosure();
    return;
  }
  
  rs2::frame frame;
  try
  {
    if (!m_framesQueue->poll_for_frame(&frame))
    {
      nextTask() = envir().taskScheduler().scheduleDelayedTask(0, (TaskFunc *)waitForFrame, this);
    }
    else
    {
      frame.keep();
      m_gotFrame = std::chrono::high_resolution_clock::now();
      deliverRSFrame(&frame);
    }
  }
  catch (const std::exception &e)
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
    if (!(getFramesQueue()->poll_for_frame(&frame)))
    {
      nextTask() = envir().taskScheduler().scheduleDelayedTask(0, (TaskFunc *)RsDeviceSource::waitForFrame, this);
    }
    else
    {
      frame.keep();
      m_gotFrame = std::chrono::high_resolution_clock::now();
      deliverRSFrame(&frame);
    }
  }
  catch (const std::exception &e)
  {
    envir() << "RsDeviceSource: " << e.what() << '\n';
  }
}

// The following is called after each delay between packet sends:
void RsDeviceSource::waitForFrame(RsDeviceSource* t_deviceSource)
{
  t_deviceSource->handleWaitForFrame();
}

void RsDeviceSource::deliverRSFrame(rs2::frame *t_frame)
{
  if (!isCurrentlyAwaitingData())
  {
    envir() << "isCurrentlyAwaitingData returned false\n";
    return; // we're not ready for the data yet
  }

  unsigned newFrameSize = t_frame->get_data_size();

  gettimeofday(&fPresentationTime, NULL); // If you have a more accurate time - e.g., from an encoder - then use that instead.
  RsFrameHeader header;
  unsigned char * data;
  if (CompressionFactory::isCompressionSupported(t_frame->get_profile().format(),t_frame->get_profile().stream_type()))
  {
    fFrameSize = ((int *)t_frame->get_data())[0];
    data = (unsigned char *)t_frame->get_data() + sizeof(int);
  }
  else
  {
    fFrameSize = t_frame->get_data_size();
    data = (unsigned char *)t_frame->get_data();
  }
  memmove(fTo + sizeof(RsFrameHeader), data, fFrameSize);
  fFrameSize += sizeof(RsMetadataHeader);
  header.networkHeader.frameSize = fFrameSize;
  fFrameSize += sizeof(RsNetworkHeader);
  if (t_frame->supports_frame_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP))
  {
    header.metadataHeader.timestamp = t_frame->get_frame_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP);
  }
  else
  {
    header.metadataHeader.timestamp = t_frame->get_timestamp();
  }
  
  if (t_frame->supports_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER))
  {
    header.metadataHeader.frameCounter = t_frame->get_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER);
  }
  else
  {
    header.metadataHeader.frameCounter = t_frame->get_frame_number();
  }
  
  if (t_frame->supports_frame_metadata(RS2_FRAME_METADATA_ACTUAL_FPS))
  {
    header.metadataHeader.actualFps = t_frame->get_frame_metadata(RS2_FRAME_METADATA_ACTUAL_FPS);
  }
  
  header.metadataHeader.timestampDomain = t_frame->get_frame_timestamp_domain();

  memmove(fTo, &header, sizeof(header));
  assert(fMaxSize > fFrameSize); //TODO: to remove on release

  std::chrono::high_resolution_clock::time_point* tp = &RsStatistics::getSendPacketTp();
  std::chrono::high_resolution_clock::time_point curTime = std::chrono::high_resolution_clock::now();
  m_waitingTimeSpan = std::chrono::duration_cast<std::chrono::duration<double>>(m_gotFrame-m_getFrame);
  m_processingTimeSpan = std::chrono::duration_cast<std::chrono::duration<double>>(curTime-m_gotFrame);
  *tp = curTime;
  //printf ("stream %d:tranfer time is %f, waiting time was %f, processing time was %f, sum is %f\n",frame->get_profile().format(),networkTimeSpan*1000,waitingTimeSpan*1000,processingTimeSpan*1000,(networkTimeSpan+waitingTimeSpan+processingTimeSpan)*1000);
  // After delivering the data, inform the reader that it is now available:
  FramedSource::afterGetting(this);
}
