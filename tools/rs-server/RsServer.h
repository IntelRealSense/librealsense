#include <iostream>

#include <liveMedia.hh>
#include <GroupsockHelper.hh>
#include <signal.h>
#include "RsUsageEnvironment.h"
#include "RsSource.hh"
#include "RsServerMediaSubsession.h"
#include "RsDevice.hh"
#include "RsRTSPServer.hh"
#include "RsServerMediaSession.h"
#include "RsCommon.h"
#include <compression/CompressionFactory.h>

#include <string>

class server
{
public:
    server(std::string addr, int port);
    ~server();

    void start();
    void stop();

private:
    void server::calculate_extrinsics();

    rs2::device selected_device;
    RsRTSPServer* rtspServer;
    UsageEnvironment* env;
    std::shared_ptr<RsDevice> rsDevice;
    std::vector<rs2::video_stream_profile> supported_stream_profiles; // streams for extrinsics map creation
    std::vector<RsSensor> sensors;
    TaskScheduler* scheduler;
    unsigned int port;
};