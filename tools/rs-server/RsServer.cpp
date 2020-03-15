// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <iostream>

#include <liveMedia.hh>
#include <BasicUsageEnvironment.hh>
#include <GroupsockHelper.hh>
#include <signal.h>
#include "RsSource.hh"
#include "RsServerMediaSubsession.h"
#include "RsDevice.hh"
#include "RsRTSPServer.hh"
#include "RsServerMediaSession.h"

UsageEnvironment *env;
rs2::device selected_device;

RsRTSPServer *rtspServer;
std::shared_ptr<RsDevice> rsDevice;
std::vector<RsSensor> sensors;
TaskScheduler *scheduler;

void sigint_handler(int sig);

int main(int argc, char **argv)
{
  OutPacketBuffer::increaseMaxSizeTo(1280*720*3);
  signal(SIGINT, sigint_handler);

  // Begin by setting up our usage environment:
  scheduler = BasicTaskScheduler::createNew();
  env = BasicUsageEnvironment::createNew(*scheduler);

  rtspServer = RsRTSPServer::createNew(*env, 8554);
  if (rtspServer == NULL)
  {
    *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
    exit(1);
  }

  std::vector<rs2::video_stream_profile> supported_stream_profiles;
  
  rsDevice = std::make_shared<RsDevice>();
  sensors = rsDevice.get()->getSensors();
  int sensorIndex = 0; //TODO::to remove
  for (auto sensor : sensors)
  {
    RsServerMediaSession *sms;
    if (sensorIndex == 0) //TODO: to move to generic exposure when host is ready
    {
      sms = RsServerMediaSession::createNew(*env, sensor, "depth" /*sensor.getSensorName().data()*/, "",
                                            "Session streamed by \"realsense streamer\"",
                                            True);
    }
    else if (sensorIndex == 1)
    {
      sms = RsServerMediaSession::createNew(*env, sensor, "color" /*sensor.getSensorName().data()*/, "",
                                            "Session streamed by \"realsense streamer\"",
                                            True);
    }
    else
    {
      break;
    }
    
    for (auto stream_profile : sensor.getStreamProfiles())
    {
      rs2::video_stream_profile stream = stream_profile.second;
      //TODO: expose all streams when host is ready
      //if (stream.format() != RS2_FORMAT_Y16)// || stream.format() == RS2_FORMAT_Y8 || stream.format() == RS2_FORMAT_Y16 || stream.format() == RS2_FORMAT_RAW16 || stream.format() == RS2_FORMAT_YUYV || stream.format() == RS2_FORMAT_UYVY)
      if (stream.format() == RS2_FORMAT_BGR8 || stream.format() == RS2_FORMAT_RGB8 || stream.format() == RS2_FORMAT_Z16 || stream.format() == RS2_FORMAT_Y8 || stream.format() == RS2_FORMAT_YUYV || stream.format() == RS2_FORMAT_UYVY)
      {
        if (stream.fps() == 6)
        {
          if ((stream.width() == 1280 && stream.height() == 720) ||(stream.width() == 640 && stream.height() == 480)||(stream.width() == 480 && stream.height() == 270) ||(stream.width() == 424 && stream.height() == 240))
          {
          sms->addSubsession(RsServerMediaSubsession::createNew(*env, stream, rsDevice));
          // streams for extrinsics map creation
          supported_stream_profiles.push_back(stream);
          continue;
          }
        }
        else if (stream.fps() == 15 || stream.fps() == 30)
        {
          if ((stream.width() == 640 && stream.height() == 480)||(stream.width() == 480 && stream.height() == 270)||(stream.width() == 424 && stream.height() == 240) )
          {
          sms->addSubsession(RsServerMediaSubsession::createNew(*env, stream, rsDevice));
          supported_stream_profiles.push_back(stream);
          continue;
          }
        }
        else if (stream.fps() == 60)
        {
          if ((stream.width() == 480 && stream.height() == 270)||(stream.width() == 424 && stream.height() == 240))
          {
          sms->addSubsession(RsServerMediaSubsession::createNew(*env, stream, rsDevice));
          supported_stream_profiles.push_back(stream);
          continue;
          }
        }
      }
      *env<<"Ignoring stream: format: "<<stream.format()<<" width: "<<stream.width()<<" height: "<<stream.height()<<" fps: "<<stream.fps()<<"\n";
    }

    //TODO: improve efficiency by go once per physical sensor
    for (auto stream_profile_from : supported_stream_profiles)
    {
      for (auto stream_profile_to : supported_stream_profiles)
      {
        int from_sensor_key = RsDevice::getPhysicalSensorUniqueKey(stream_profile_from.stream_type(),stream_profile_from.stream_index());
        int to_sensor_key = RsDevice::getPhysicalSensorUniqueKey(stream_profile_to.stream_type(),stream_profile_to.stream_index());
        
        rsDevice.get()->minimal_extrinsics_map[std::make_pair(from_sensor_key,to_sensor_key)] = stream_profile_from.get_extrinsics_to(stream_profile_to);
      }
    }
    
    rtspServer->addServerMediaSession(sms);
    char *url = rtspServer->rtspURL(sms);
    *env << "Play this stream using the URL \"" << url << "\"\n";

    // controls
    rtspServer->setSupportedOptions(sensor.getSensorName(), sensor.gerSupportedOptions());

    delete[] url;
    sensorIndex++;
  }

  env->taskScheduler().doEventLoop(); // does not return

  return 0; // only to prevent compiler warning
}

void sigint_handler(int sig)
{
  Medium::close(rtspServer);
  env->reclaim(); env = NULL;
  delete scheduler; scheduler = NULL;
  exit(sig);
}
