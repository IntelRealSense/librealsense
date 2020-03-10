// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "RsServerMediaSession.h"

////////// ServerMediaSession //////////

RsServerMediaSession *RsServerMediaSession ::createNew(UsageEnvironment &t_env, RsSensor &t_sensor,
                                                       char const *t_streamName, char const *t_info,
                                                       char const *t_description, Boolean t_isSSM, char const *t_miscSDPLines)
{
  return new RsServerMediaSession(t_env, t_sensor, t_streamName, t_info, t_description,
                                  t_isSSM, t_miscSDPLines);
}

static char const *const libNameStr = "LIVE555 Streaming Media v";
char const *const libVersionStr = LIVEMEDIA_LIBRARY_VERSION_STRING;

RsServerMediaSession::RsServerMediaSession(UsageEnvironment &t_env,
                                           RsSensor &t_sensor,
                                           char const *t_streamName,
                                           char const *t_info,
                                           char const *t_description,
                                           Boolean t_isSSM, char const *t_miscSDPLines)
    : ServerMediaSession(t_env, t_streamName, t_info, t_description, t_isSSM, t_miscSDPLines), m_rsSensor(t_sensor), m_isActive(false)
{
}

RsServerMediaSession::~RsServerMediaSession()
{
  //TODO:: to check if i need to delete rsSensor
}

int RsServerMediaSession::openRsCamera(std::unordered_map<long long int, rs2::frame_queue> &t_streamProfiles)
{
  m_rsSensor.open(t_streamProfiles);
  m_rsSensor.start(t_streamProfiles);
  m_isActive = true;
  return EXIT_SUCCESS;
}

int RsServerMediaSession::closeRsCamera()
{
  if(m_isActive)
  {
  m_rsSensor.getRsSensor().stop();
  m_rsSensor.getRsSensor().close();
  m_isActive = false;
  }
  return EXIT_SUCCESS;
}

RsSensor &RsServerMediaSession::getRsSensor()
{
  return m_rsSensor;
}
