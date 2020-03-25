// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "ip_device.hh"
#include <ipDeviceCommon/Statistic.h>

#include "api.h"
#include <librealsense2-net/rs_net.h>

#include <chrono>
#include <list>
#include <thread>

#include <NetdevLog.h>

extern std::map<std::pair<int, int>, rs2_extrinsics> minimal_extrinsics_map;

std::string sensors_str[] = {STEREO_SENSOR_NAME, RGB_SENSOR_NAME};

//WA for stop
void ip_device::recover_rtsp_client(int sensor_index)
{
    remote_sensors[sensor_index]->rtsp_client = RsRTSPClient::getRtspClient(std::string("rtsp://" + ip_address + ":8554/" + sensors_str[sensor_index]).c_str(), "ethernet_device");

    ((RsRTSPClient*)remote_sensors[sensor_index]->rtsp_client)->initFunc(&rs_rtp_stream::get_memory_pool());
    ((RsRTSPClient*)remote_sensors[sensor_index]->rtsp_client)->getStreams();
}

ip_device::~ip_device()
{
    DBG << "Destroying ip_device";
    
    try
    {
        is_device_alive = false;

        if (sw_device_status_check.joinable())
        {
            sw_device_status_check.join();
        }

        for (int remote_sensor_index = 0; remote_sensor_index < NUM_OF_SENSORS; remote_sensor_index++)
        {
            update_sensor_state(remote_sensor_index, {}, false);
            delete (remote_sensors[remote_sensor_index]);
        }
    }
    catch (const std::exception &e)
    {
        ERR << e.what();
    }
    DBG << "Destroying ip_device completed";
}

void ip_device::stop_sensor_streams(int sensor_index)
{
    for(long long int key : remote_sensors[sensor_index]->active_streams_keys)
    {
        DBG << "Stopping stream [uid:key] " << streams_collection[key].get()->m_rs_stream.uid << ":" << key << "]";
        streams_collection[key].get()->is_enabled = false;
        if(inject_frames_thread[key].joinable())
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
    DBG << "query_streams";
    std::vector<rs2_video_stream> streams;

    if(remote_sensors[sensor_id]->rtsp_client == NULL)
        return streams;

    //workaround
    if(remote_sensors[sensor_id]->rtsp_client == nullptr)
        recover_rtsp_client(sensor_id);

    streams = remote_sensors[sensor_id]->rtsp_client->getStreams();
    return streams;
}
std::vector<IpDeviceControlData> ip_device::get_controls(int sensor_id)
{
    DBG << "get_controls";
    std::vector<IpDeviceControlData> controls;
    controls = remote_sensors[sensor_id]->rtsp_client->getControls();

    return controls;
}

bool ip_device::init_device_data(rs2::software_device sw_device)
{
    std::vector<rs2::stream_profile> device_streams;
    std::string url, sensor_name = "";
    for(int sensor_id = 0; sensor_id < NUM_OF_SENSORS; sensor_id++)
    {

        url = std::string("rtsp://" + ip_address + ":8554/" + sensors_str[sensor_id]);
        sensor_name = sensors_str[sensor_id];

        remote_sensors[sensor_id] = new ip_sensor();

        remote_sensors[sensor_id]->rtsp_client = RsRTSPClient::getRtspClient(url.c_str(), "ip_device_device");
        ((RsRTSPClient*)remote_sensors[sensor_id]->rtsp_client)->initFunc(&rs_rtp_stream::get_memory_pool());

        rs2::software_sensor tmp_sensor = sw_device.add_sensor(sensor_name);

        remote_sensors[sensor_id]->sw_sensor = std::make_shared<rs2::software_sensor>(tmp_sensor);

        if(sensor_id == 1) //todo: remove hard coded
        {
            std::vector<IpDeviceControlData> controls = get_controls(sensor_id);
            for(auto& control : controls)
            {

                float val = NAN;

                INF << "Init sensor " << control.sensorId << ", option '" << control.option << "', value " << control.range.def;

                if(control.range.min == control.range.max)
                {
                    remote_sensors[control.sensorId]->sw_sensor->add_read_only_option(control.option, control.range.def);
                }
                else
                {
                    remote_sensors[control.sensorId]->sw_sensor->add_option(control.option, {control.range.min, control.range.max, control.range.def, control.range.step});
                }
                remote_sensors[control.sensorId]->sensors_option[control.option] = control.range.def;
                try
                {
                    get_option_value(control.sensorId, control.option, val);
                    if(val != control.range.def && val >= control.range.min && val <= control.range.max)
                    {
                        remote_sensors[control.sensorId]->sw_sensor->set_option(control.option, val);
                    }
                }
                catch(const std::exception& e)
                {
                    ERR << e.what();
                }
            }
        }

        auto streams = query_streams(sensor_id);

        DBG << "Init got " << streams.size() << " streams per sensor " << sensor_id;

        for(int stream_index = 0; stream_index < streams.size(); stream_index++)
        {
            bool is_default=false;
            // just for readable code
            rs2_video_stream st = streams[stream_index];
            long long int stream_key = RsRTSPClient::getStreamProfileUniqueKey(st);

            //check if default value per this stream type were picked
            if(default_streams[std::make_pair(st.type, st.index)] == -1)
            {
                if (st.width==DEFAULT_PROFILE_WIDTH && st.height==DEFAULT_PROFILE_HIGHT && st.fps==DEFAULT_PROFILE_FPS
                    && (st.type != rs2_stream::RS2_STREAM_COLOR || st.fmt == DEFAULT_PROFILE_COLOR_FORMAT))
                {
                    default_streams[std::make_pair(st.type, st.index)] = stream_index;
                    is_default=true;
                }
            }

            auto stream_profile = remote_sensors[sensor_id]->sw_sensor->add_video_stream(st, is_default);
            device_streams.push_back(stream_profile);
            streams_collection[stream_key] = std::make_shared<rs_rtp_stream>(st, stream_profile);
            memory_pool = &rs_rtp_stream::get_memory_pool();
        }
        DBG << "Init done adding streams for sensor ID: " << sensor_id;
    }

    for(auto stream_profile_from : device_streams)
    {
        for(auto stream_profile_to : device_streams)
        {
            int from_key = RsRTSPClient::getPhysicalSensorUniqueKey(stream_profile_from.stream_type(), stream_profile_from.stream_index());
            int to_key = RsRTSPClient::getPhysicalSensorUniqueKey(stream_profile_from.stream_type(), stream_profile_from.stream_index());

            if(minimal_extrinsics_map.find(std::make_pair(from_key, to_key)) == minimal_extrinsics_map.end())
            {
                ERR << "Extrinsics data is missing.";
            }
            rs2_extrinsics extrinisics = minimal_extrinsics_map[std::make_pair(from_key, to_key)];

            stream_profile_from.register_extrinsics_to(stream_profile_to, extrinisics);
        }
    }

    //poll sw device streaming state
    this->sw_device_status_check = std::thread(&ip_device::polling_state_loop, this);
    return true;
}

void ip_device::polling_state_loop()
{
    while(this->is_device_alive)
    {
        try
        {
            bool enabled;
            for(int i = 0; i < NUM_OF_SENSORS; i++)
            {
                //poll start/stop events
                auto sw_sensor = remote_sensors[i]->sw_sensor.get();
   
                enabled = (sw_sensor->get_active_streams().size() > 0); 
   
                if (remote_sensors[i]->is_enabled != enabled)
                {
                    try
                    {
                        //the ftate flag is togled before the actual updatee to avoid endless re-tries on case of failure.
                        remote_sensors[i]->is_enabled = enabled;
                        update_sensor_state(i, sw_sensor->get_active_streams(), true);
                    }
                    catch(const std::exception& e)
                    {
                        ERR << e.what();
                        update_sensor_state(i, {}, true);
                        rs2_software_notification notification;
                        notification.description = e.what();
                        notification.severity = RS2_LOG_SEVERITY_ERROR;
                        notification.type = RS2_EXCEPTION_TYPE_UNKNOWN;
                        remote_sensors[i]->sw_sensor.get()->on_notification(notification);
                        continue;
                    }
                }
                auto sensor_supported_option = sw_sensor->get_supported_options();
                for(rs2_option opt : sensor_supported_option)
                {
                    if(remote_sensors[i]->sensors_option[opt] != (float)sw_sensor->get_option(opt))
                    {
                        //TODO: get from map once to reduce logarithmic complexity
                        remote_sensors[i]->sensors_option[opt] = (float)sw_sensor->get_option(opt);
                        INF << "Option '" << opt << "' has changed to: " << remote_sensors[i]->sensors_option[opt];
                        update_option_value(i, opt, remote_sensors[i]->sensors_option[opt]);
                    }
                }
            }
        }
        catch(const std::exception& e)
        {
            ERR << e.what();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(POLLING_SW_DEVICE_STATE_INTERVAL));
    }
}

void ip_device::update_option_value(int sensor_index, rs2_option opt, float val)
{
    float updated_value = 0;
    set_option_value(sensor_index, opt, val);
    get_option_value(sensor_index, opt, updated_value);
    if(val != updated_value)
    {
        //TODO:: to uncomment after adding exception handling
        //throw std::runtime_error("[update_option_value] error");
        ERR << "Cannot update option value.";
    }
}

void ip_device::set_option_value(int sensor_index, rs2_option opt, float val)
{
    if(sensor_index < (sizeof(remote_sensors) / sizeof(remote_sensors[0])) && remote_sensors[sensor_index] != nullptr)
    {
        remote_sensors[sensor_index]->rtsp_client->setOption(std::string(sensors_str[sensor_index]), opt, val);
    }
}

void ip_device::get_option_value(int sensor_index, rs2_option opt, float& val)
{
    if(sensor_index < sizeof(remote_sensors) && remote_sensors[sensor_index] != nullptr)
    {
        remote_sensors[sensor_index]->rtsp_client->getOption(std::string(sensors_str[sensor_index]), opt, val);
    }
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

void ip_device::update_sensor_state(int sensor_index, std::vector<rs2::stream_profile> updated_streams, bool recover)
{
    //check if need to close all
    if(updated_streams.size() == 0)
    {
        remote_sensors[sensor_index]->rtsp_client->stop();
        remote_sensors[sensor_index]->rtsp_client->close();
        remote_sensors[sensor_index]->rtsp_client = nullptr;
        stop_sensor_streams(sensor_index);
        if(recover)
        {
            recover_rtsp_client(sensor_index);
        }
        return;
    }
    for(size_t i = 0; i < updated_streams.size(); i++)
    {
        rs2::video_stream_profile vst(updated_streams[i]);

        long long int requested_stream_key = RsRTSPClient::getStreamProfileUniqueKey(convert_stream_object(vst));

        if(streams_collection.find(requested_stream_key) == streams_collection.end())
        {
            throw std::runtime_error("[update_sensor_state] stream key: " + std::to_string(requested_stream_key) + " is not found. closing device.");
        }

        rtp_callbacks[requested_stream_key] = new rs_rtp_callback(streams_collection[requested_stream_key]);
        remote_sensors[sensor_index]->rtsp_client->addStream(streams_collection[requested_stream_key].get()->m_rs_stream, rtp_callbacks[requested_stream_key]);
        inject_frames_thread[requested_stream_key] = std::thread(&ip_device::inject_frames_loop, this, streams_collection[requested_stream_key]);
        remote_sensors[sensor_index]->active_streams_keys.push_front(requested_stream_key);
    }

    remote_sensors[sensor_index]->rtsp_client->start();
    INF << "Stream started for sensor " << sensor_index;
}

int stream_type_to_sensor_id(rs2_stream type)
{
    if(type == RS2_STREAM_INFRARED || type == RS2_STREAM_DEPTH)
        return 0;
    return 1;
}

void ip_device::inject_frames_loop(std::shared_ptr<rs_rtp_stream> rtp_stream)
{
    try
    {
        rtp_stream.get()->is_enabled = true;

        rtp_stream.get()->frame_data_buff.frame_number = 0;
        int uid = rtp_stream.get()->m_rs_stream.uid;
        rs2_stream type = rtp_stream.get()->m_rs_stream.type;
        int sensor_id = stream_type_to_sensor_id(type);

        while(rtp_stream.get()->is_enabled == true)
        {
            if(rtp_stream.get()->queue_size() != 0)
            {
                Raw_Frame* frame = rtp_stream.get()->extract_frame();
                rtp_stream.get()->frame_data_buff.pixels = frame->m_buffer;

                rtp_stream.get()->frame_data_buff.timestamp = frame->m_metadata->data.timestamp;

                rtp_stream.get()->frame_data_buff.frame_number++;
                rtp_stream.get()->frame_data_buff.domain = frame->m_metadata->data.timestampDomain;

                remote_sensors[sensor_id]->sw_sensor->set_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP, rtp_stream.get()->frame_data_buff.timestamp);
                remote_sensors[sensor_id]->sw_sensor->set_metadata(RS2_FRAME_METADATA_ACTUAL_FPS, frame->m_metadata->data.actualFps);
                remote_sensors[sensor_id]->sw_sensor->set_metadata(RS2_FRAME_METADATA_FRAME_COUNTER, rtp_stream.get()->frame_data_buff.frame_number);
                remote_sensors[sensor_id]->sw_sensor->set_metadata(RS2_FRAME_METADATA_FRAME_EMITTER_MODE, 1);

                remote_sensors[sensor_id]->sw_sensor->set_metadata(RS2_FRAME_METADATA_TIME_OF_ARRIVAL, std::chrono::duration<double, std::milli>(std::chrono::system_clock::now().time_since_epoch()).count());
                remote_sensors[sensor_id]->sw_sensor->on_video_frame(rtp_stream.get()->frame_data_buff);
            }
        }

        rtp_stream.get()->reset_queue();
        DBG << "Polling data at stream " << rtp_stream.get()->m_rs_stream.uid << " completed";
    }
    catch(const std::exception& ex)
    {
        ERR << ex.what();
    }
}

rs2_device* rs2_create_net_device(int api_version, const char* address, rs2_error** error) BEGIN_API_CALL
{
    verify_version_compatibility(api_version);
    VALIDATE_NOT_NULL(address);

    std::string addr(address);

    // create sw device
    rs2::software_device sw_dev = rs2::software_device([](rs2_device*) {});
    // create IP instance
    ip_device* ip_dev = new ip_device(addr, sw_dev);
    // set client destruction functioun
    sw_dev.set_destruction_callback([ip_dev] { delete ip_dev; });
    // register device info to sw device
    DeviceData data = ip_dev->remote_sensors[0]->rtsp_client->getDeviceData();
    sw_dev.update_info(RS2_CAMERA_INFO_NAME, data.name + " IP Device");
    sw_dev.register_info(rs2_camera_info::RS2_CAMERA_INFO_IP_ADDRESS, addr);
    sw_dev.register_info(rs2_camera_info::RS2_CAMERA_INFO_SERIAL_NUMBER, data.serialNum);
    sw_dev.register_info(rs2_camera_info::RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR, data.usbType);
    // return sw device
    //return sw_dev;

    return sw_dev.get().get();
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, api_version, address)
