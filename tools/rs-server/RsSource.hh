/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2019 Live Networks, Inc.  All rights reserved.
// A template for a MediaSource encapsulating an audio/video input device
//
// NOTE: Sections of this code labeled "%%% TO BE WRITTEN %%%" are incomplete, and needto be written by the programmer
// (depending on the features of the particulardevice).
// C++ header

#ifndef _RS_DEVICE_SOURCE_HH
#define _RS_DEVICE_SOURCE_HH

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

#endif
