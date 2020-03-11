// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "ip_device.hh"
#include "api.h"
#include <librealsense2-net/rs_net.h>

#include <ipDeviceCommon/Statistic.h>
#include <list>

#include <chrono>
#include <thread>

std::string sensors_str[] = {"depth", "color"};

//WA for stop
void ip_device::recover_rtsp_client(int sensor_index)
{
    remote_sensors[sensor_index]->rtsp_client = RsRTSPClient::getRtspClient(std::string("rtsp://" + ip_address + ":8554/" + sensors_str[sensor_index]).c_str(), "ethernet_device");

    ((RsRTSPClient *)remote_sensors[sensor_index]->rtsp_client)->initFunc(&rs_rtp_stream::get_memory_pool());
    ((RsRTSPClient *)remote_sensors[sensor_index]->rtsp_client)->getStreams();
}

ip_device::~ip_device()
{
    is_device_alive = false;

    for (int remote_sensor_index = 0; remote_sensor_index < NUM_OF_SENSORS; remote_sensor_index++)
    {
        stop_sensor_streams(remote_sensor_index);
    }

    if (sw_device_status_check.joinable())
    {
        sw_device_status_check.join();
    }
    std::cout << "destroy ip_device\n";
}

void ip_device::stop_sensor_streams(int sensor_index)
{
    for (long long int key : remote_sensors[sensor_index]->active_streams_keys)
    {
        std::cout << "\t@@@ stopping stream [uid:key] " << streams_collection[key].get()->m_rs_stream.uid << ":" << key << "]" << std::endl;
        streams_collection[key].get()->is_enabled = false;
        if (inject_frames_thread[key].joinable())
            inject_frames_thread[key].join();
    }
    remote_sensors[sensor_index]->active_streams_keys.clear();
}

ip_device::ip_device(std::string ip_address, rs2::software_device sw_device)
{
    this->ip_address = ip_address;
    this->is_device_alive = true;

    //init device data
    init_device_data(sw_device);
}

std::vector<rs2_video_stream> ip_device::query_streams(int sensor_id)
{
    std::cout << "Querry Sensors\n";
    std::vector<rs2_video_stream> streams;

    if (remote_sensors[sensor_id]->rtsp_client == NULL)
        return streams;

    //workaround
    if (remote_sensors[sensor_id]->rtsp_client == nullptr)
        recover_rtsp_client(sensor_id);

    streams = remote_sensors[sensor_id]->rtsp_client->getStreams();
    return streams;
}
std::vector<IpDeviceControlData> ip_device::get_controls(int sensor_id)
{
    std::cout << "GetControls\n";
    std::vector<IpDeviceControlData> controls;
    controls = remote_sensors[sensor_id]->rtsp_client->getControls();

    return controls;
}

bool ip_device::init_device_data(rs2::software_device sw_device)
{
    std::string url, sensor_name = "";
    for (int sensor_id = 0; sensor_id < NUM_OF_SENSORS; sensor_id++)
    {

        url = std::string("rtsp://" + ip_address + ":8554/" + sensors_str[sensor_id]);
        sensor_name = sensors_str[sensor_id] + std::string(" (Remote)");

        remote_sensors[sensor_id] = new ip_sensor();

        remote_sensors[sensor_id]->rtsp_client = RsRTSPClient::getRtspClient(url.c_str(), "ip_device_device");
        ((RsRTSPClient *)remote_sensors[sensor_id]->rtsp_client)->initFunc(&rs_rtp_stream::get_memory_pool());

        rs2::software_sensor tmp_sensor = sw_device.add_sensor(sensor_name);

        remote_sensors[sensor_id]->sw_sensor = std::make_shared<rs2::software_sensor>(tmp_sensor);

        //hard_coded
        if (sensor_id == 1) //todo: remove hard
        {
            std::vector<IpDeviceControlData> controls = get_controls(sensor_id);
            for (auto &control : controls)
            {
                remote_sensors[control.sensorId]->sw_sensor->add_option(control.option, {control.range.min, control.range.max, control.range.def, control.range.step});
                remote_sensors[control.sensorId]->sensors_option[control.option] = control.range.def;
            }
        }

        auto streams = query_streams(sensor_id);

        std::cout << "\t@@@ got " << streams.size() << " streams per sensor " << sensor_id << std::endl;

        for (int stream_index = 0; stream_index < streams.size(); stream_index++)
        {
            // just for readable code
            rs2_video_stream st = streams[stream_index];

            //todo: remove
            st.intrinsics = st.intrinsics;
            //nhershko: check why profile with type 0
            long long int stream_key = RsRTSPClient::getStreamProfileUniqueKey(st);
            streams_collection[stream_key] = std::make_shared<rs_rtp_stream>(st, remote_sensors[sensor_id]->sw_sensor->add_video_stream(st, stream_index == 0));
            memory_pool = &rs_rtp_stream::get_memory_pool();
        }
        std::cout << "\t@@@ done adding streams for sensor ID: " << sensor_id << std::endl;
    }

    //poll sw device streaming state
    this->sw_device_status_check = std::thread(&ip_device::polling_state_loop, this);
    return true;
}

void ip_device::polling_state_loop()
{
    while (this->is_device_alive)
    {
        bool enabled;
        for(int i=0 ; i < NUM_OF_SENSORS ; i++ )
        {
            //poll start/stop events
            auto sw_sensor = remote_sensors[i]->sw_sensor.get();
            //auto current_active_streams = sw_sensor->get_active_streams();
            
            if (sw_sensor->get_active_streams().size() > 0)
                enabled = true;
            else
                enabled = false;

            if (remote_sensors[i]->is_enabled != enabled)
            {
                update_sensor_state(i, sw_sensor->get_active_streams());
                remote_sensors[i]->is_enabled = enabled;
            }
            auto sensor_supported_option = sw_sensor->get_supported_options();
            for (rs2_option opt : sensor_supported_option)
                if (remote_sensors[i]->sensors_option[opt] != (float)sw_sensor->get_option(opt))
                {
                    //TODO: get from map once to reduce logarithmic complexity
                    remote_sensors[i]->sensors_option[opt] = (float)sw_sensor->get_option(opt);
                    std::cout << "option: " << opt << " has changed to:  " << remote_sensors[i]->sensors_option[opt] << std::endl;
                    update_option_value(i, opt, remote_sensors[i]->sensors_option[opt]);
                }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(POLLING_SW_DEVICE_STATE_INTERVAL));
    }
}

void ip_device::update_option_value(int sensor_index, rs2_option opt, float val)
{
    remote_sensors[sensor_index]->rtsp_client->setOption(opt, val);
}

rs2_video_stream convert_stream_object(rs2::video_stream_profile sp)
{
    rs2_video_stream retVal;
    retVal.fmt = sp.format();
    retVal.type = sp.stream_type();
    retVal.fps = sp.fps();
    retVal.width = sp.width();
    retVal.height = sp.height();
    retVal.index = sp.stream_index();

    return retVal;
}

void ip_device::update_sensor_state(int sensor_index, std::vector<rs2::stream_profile> updated_streams)
{
    //check if need to close all
    if (updated_streams.size() == 0)
    {
        remote_sensors[sensor_index]->rtsp_client->stop();
        remote_sensors[sensor_index]->rtsp_client->close();
        remote_sensors[sensor_index]->rtsp_client = nullptr;
        stop_sensor_streams(sensor_index);
        return;
    }
    for (size_t i = 0; i < updated_streams.size(); i++)
    {
        rs2::video_stream_profile vst(updated_streams[i]);

        long long int requested_stream_key = RsRTSPClient::getStreamProfileUniqueKey(convert_stream_object(vst));

        if (streams_collection.find(requested_stream_key) == streams_collection.end())
        {
            exit(-1);
        }

        //temporary nhershko workaround for start after stop
        if (remote_sensors[sensor_index]->rtsp_client == nullptr)
        {
            recover_rtsp_client(sensor_index);
        }

        rtp_callbacks[requested_stream_key] = new rs_rtp_callback(streams_collection[requested_stream_key]);
        remote_sensors[sensor_index]->rtsp_client->addStream(streams_collection[requested_stream_key].get()->m_rs_stream, rtp_callbacks[requested_stream_key]);
        inject_frames_thread[requested_stream_key] = std::thread(&ip_device::inject_frames_loop, this, streams_collection[requested_stream_key]);
        //streams_uid_per_sensor[sensor_index].push_front(requested_stream_key);
        remote_sensors[sensor_index]->active_streams_keys.push_front(requested_stream_key);
    }

    remote_sensors[sensor_index]->rtsp_client->start();
    std::cout << "stream started for sensor index: " << sensor_index << "  \n";
}

int stream_type_to_sensor_id(rs2_stream type)
{
    if (type == RS2_STREAM_INFRARED || type == RS2_STREAM_DEPTH)
        return 0;
    return 1;
}

void ip_device::inject_frames_loop(std::shared_ptr<rs_rtp_stream> rtp_stream)
{
    rtp_stream.get()->is_enabled = true;

    rtp_stream.get()->frame_data_buff.frame_number = 0;
    int uid = rtp_stream.get()->m_rs_stream.uid;
    rs2_stream type = rtp_stream.get()->m_rs_stream.type;
    int sensor_id = stream_type_to_sensor_id(type);

    while (rtp_stream.get()->is_enabled == true)
    {
        if (rtp_stream.get()->queue_size() != 0)
        {
            Raw_Frame *frame = rtp_stream.get()->extract_frame();
            rtp_stream.get()->frame_data_buff.pixels = frame->m_buffer;
            //rtp_stream.get()->frame_data_buff.timestamp = (frame->m_timestamp.tv_sec*1000)+(frame->m_timestamp.tv_usec/1000); // convert to milliseconds
            rtp_stream.get()->frame_data_buff.timestamp = frame->m_metadata->timestamp;

            rtp_stream.get()->frame_data_buff.frame_number++;
            // TODO Michal: change this to HW time once we pass the metadata
            //rtp_stream.get()->frame_data_buff.domain = RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME;
            rtp_stream.get()->frame_data_buff.domain = frame->m_metadata->timestampDomain;

            remote_sensors[sensor_id]->sw_sensor->set_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP, rtp_stream.get()->frame_data_buff.timestamp);
            remote_sensors[sensor_id]->sw_sensor->set_metadata(RS2_FRAME_METADATA_ACTUAL_FPS, rtp_stream.get()->m_rs_stream.fps);
            remote_sensors[sensor_id]->sw_sensor->set_metadata(RS2_FRAME_METADATA_FRAME_COUNTER, rtp_stream.get()->frame_data_buff.frame_number);
            remote_sensors[sensor_id]->sw_sensor->set_metadata(RS2_FRAME_METADATA_FRAME_EMITTER_MODE, 1);

            //nhershko todo: set it at actuqal arrivial time
            remote_sensors[sensor_id]->sw_sensor->set_metadata(RS2_FRAME_METADATA_TIME_OF_ARRIVAL,
                                                               std::chrono::duration<double, std::milli>(std::chrono::system_clock::now().time_since_epoch()).count());
#ifdef STATISTICS
            StreamStatistic *st = Statistic::getStatisticStreams()[rtp_stream.get()->stream_type()];
            std::chrono::system_clock::time_point clockEnd = std::chrono::system_clock::now();
            st->m_processingTime = clockEnd - st->m_clockBeginVec.front();
            st->m_clockBeginVec.pop();
            st->m_avgProcessingTime += st->m_processingTime.count();
            printf("STATISTICS: streamType: %d, processing time: %0.2fm, average: %0.2fm, counter: %d\n", type, st->m_processingTime * 1000, (st->m_avgProcessingTime * 1000) / st->m_frameCounter, st->m_frameCounter);
#endif
            remote_sensors[sensor_id]->sw_sensor->on_video_frame(rtp_stream.get()->frame_data_buff);
            //std::cout<<"\t@@@ added frame from type " << type << " with uid " << rtp_stream.get()->m_rs_stream.uid << " time stamp: " << (double)rtp_stream.get()->frame_data_buff.frame_number <<" profile: " << rtp_stream.get()->frame_data_buff.profile->profile->get_stream_type() << "   \n";
        }
    }

    rtp_stream.get()->reset_queue();
    std::cout << "polling data at stream index " << rtp_stream.get()->m_rs_stream.uid << " is done\n";
}

rs2_device* rs2_create_net_device(int api_version, const char* address, rs2_error** error) BEGIN_API_CALL
{
    verify_version_compatibility(api_version);
    VALIDATE_NOT_NULL(address);

    std::string addr(address);

    // create sw device
    rs2::software_device sw_dev = rs2::software_device([](rs2_device*) {});
    // create IP instance
    ip_device *ip_dev = new ip_device(addr, sw_dev);
    // set client destruction functioun
    sw_dev.set_destruction_callback([ip_dev] { delete ip_dev; });
    // register device info to sw device
    DeviceData data = ip_dev->remote_sensors[0]->rtsp_client->getDeviceData();
    sw_dev.update_info(RS2_CAMERA_INFO_NAME, data.name + "\n IP Device");
    sw_dev.register_info(rs2_camera_info::RS2_CAMERA_INFO_IP_ADDRESS, addr);
    sw_dev.register_info(rs2_camera_info::RS2_CAMERA_INFO_SERIAL_NUMBER, data.serialNum);
    sw_dev.register_info(rs2_camera_info::RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR, data.usbType);
    // return sw device
    //return sw_dev;

    return sw_dev.get().get();
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, api_version, address)