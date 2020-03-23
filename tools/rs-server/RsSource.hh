// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#ifndef _DEVICE_SOURCE_HH
#include "DeviceSource.hh"
#endif

#ifndef _FRAMED_SOURCE_HH
#include "FramedSource.hh"
#endif

#include <rs.hpp> // Include RealSense Cross Platform API
#include <mutex>
#include <condition_variable>

class RsDeviceSource : public FramedSource
{
public:
  static RsDeviceSource *createNew(UsageEnvironment &t_env, rs2::video_stream_profile &t_videoStreamProfile, rs2::frame_queue &t_queue);
  void handleWaitForFrame();
  static void waitForFrame(RsDeviceSource* t_deviceSource);
protected:
  RsDeviceSource(UsageEnvironment &t_env, rs2::video_stream_profile &t_videoStreamProfile, rs2::frame_queue &t_queue);
  virtual ~RsDeviceSource();
private:
  virtual void doGetNextFrame();
 
  rs2::frame_queue* getFramesQueue(){return m_framesQueue;};
  //virtual void doStopGettingFrames(); // optional

private:
  void deliverRSFrame(rs2::frame *t_frame);

private:

  rs2::frame_queue *m_framesQueue;
  rs2::video_stream_profile *m_streamProfile;
std::chrono::high_resolution_clock::time_point m_getFrame,m_gotFrame;
std::chrono::duration<double> m_networkTimeSpan,m_waitingTimeSpan,m_processingTimeSpan;
};
