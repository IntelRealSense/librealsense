#pragma once

//////////////////////////////////////////////////////////////////////////
#ifdef _WIN32

#define _WINSOCKAPI_

#include <windows.h>
#pragma comment(lib, "Ws2_32.lib")

#include <SIGNAL.H>

#endif
//////////////////////////////////////////////////////////////////////////

#include <librealsense2/rs.hpp>
#include <librealsense2/hpp/rs_internal.hpp>
#include "option.h"

#include "RsRtspClient.h"
#include "software-device.h"

#include <list>

#include "rs_rtp_stream.hh"
#include "rs_rtp_callback.hh"
#include "ip_sensor.hh"

#define MAX_ACTIVE_STREAMS 4

#define NUM_OF_SENSORS 2

#define POLLING_SW_DEVICE_STATE_INTERVAL 100

class ip_device
{

public:
#ifdef _WIN32
    __declspec(dllexport)
#endif
    
    ip_device(std::string ip_address, rs2::software_device sw_device);

    ~ip_device();

    ip_sensor *remote_sensors[NUM_OF_SENSORS];

private:
    bool is_device_alive;

    //TODO: get smart ptr
    MemoryPool *memory_pool;

    std::string ip_address;

    //todo: consider wrapp all maps to single container
    std::map<long long int, std::shared_ptr<rs_rtp_stream>> streams_collection;

    std::map<long long int, std::thread> inject_frames_thread;

    std::map<long long int, rs_rtp_callback *> rtp_callbacks;

    std::thread sw_device_status_check;

    bool init_device_data(rs2::software_device sw_device);

    void polling_state_loop();

    void inject_frames_loop(std::shared_ptr<rs_rtp_stream> rtp_stream);

    void stop_sensor_streams(int sensor_id);

    void update_sensor_state(int sensor_index, std::vector<rs2::stream_profile> updated_streams);
    void update_option_value(int sensor_index, rs2_option opt, float val);

    void set_option_value(int sensor_index, rs2_option opt, float val);
    void get_option_value(int sensor_index, rs2_option opt, float& val);

    std::vector<rs2_video_stream> query_streams(int sensor_id);

    std::vector<IpDeviceControlData> get_controls(int sensor_id);

    void recover_rtsp_client(int sensor_index);
};
