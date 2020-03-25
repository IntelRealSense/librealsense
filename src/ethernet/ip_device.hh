// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "RsRtspClient.h"
#include "ip_sensor.hh"
#include "rs_rtp_callback.hh"

#include "option.h"
#include "software-device.h"
#include <librealsense2/hpp/rs_internal.hpp>
#include <librealsense2/rs.hpp>

#include <list>

#define MAX_ACTIVE_STREAMS 4

#define NUM_OF_SENSORS 2

#define POLLING_SW_DEVICE_STATE_INTERVAL 100

#define DEFAULT_PROFILE_FPS 15

#define DEFAULT_PROFILE_WIDTH 424

#define DEFAULT_PROFILE_HIGHT 240

#define DEFAULT_PROFILE_COLOR_FORMAT RS2_FORMAT_RGB8 

class ip_device
{

public:
    ip_device(std::string ip_address, rs2::software_device sw_device);
    ~ip_device();

    ip_sensor* remote_sensors[NUM_OF_SENSORS];

private:
    bool is_device_alive;

    //TODO: get smart ptr
    MemoryPool* memory_pool;

    std::string ip_address;

    //todo: consider wrapp all maps to single container
    std::map<long long int, std::shared_ptr<rs_rtp_stream>> streams_collection;

    std::map<long long int, std::thread> inject_frames_thread;

    std::map<long long int, rs_rtp_callback*> rtp_callbacks;

    std::thread sw_device_status_check;

    bool init_device_data(rs2::software_device sw_device);

    void polling_state_loop();

    void inject_frames_loop(std::shared_ptr<rs_rtp_stream> rtp_stream);

    void stop_sensor_streams(int sensor_id);

    void update_sensor_state(int sensor_index, std::vector<rs2::stream_profile> updated_streams, bool recover);
    void update_option_value(int sensor_index, rs2_option opt, float val);

    void set_option_value(int sensor_index, rs2_option opt, float val);
    void get_option_value(int sensor_index, rs2_option opt, float& val);

    std::vector<rs2_video_stream> query_streams(int sensor_id);

    std::vector<IpDeviceControlData> get_controls(int sensor_id);

    void recover_rtsp_client(int sensor_index);

    // default device stream index per type + sensor_index
    // streams will be loaded at runtime so here the place holder  
    std::map<std::pair<rs2_stream,int>,int> default_streams = 
    { 
        { std::make_pair(rs2_stream::RS2_STREAM_COLOR,0),-1},
        { std::make_pair(rs2_stream::RS2_STREAM_DEPTH,0),-1}
    };
};
