// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "record_sensor.h"
#include "api.h"
#include "stream.h"

librealsense::record_sensor::record_sensor(const device_interface& device,
                                            sensor_interface& sensor,
                                            frame_interface_callback_t on_frame,
                                            snapshot_callback_t on_snapshot) :
    m_device_record_snapshot_handler(on_snapshot),
    m_sensor(sensor),
    m_user_notification_callback(nullptr, [](rs2_notifications_callback* n) {}),
    m_record_callback(on_frame),
    m_is_recording(false),
    m_is_pause(false),
    m_parent_device(device)
{
}

librealsense::record_sensor::~record_sensor()
{
}

stream_profiles record_sensor::get_stream_profiles() const
{
    return m_sensor.get_stream_profiles();
}

void librealsense::record_sensor::open(const stream_profiles& requests)
{
    m_sensor.open(requests);

    m_is_recording = true;
    m_curr_configurations = convert_profiles(to_profiles(requests));
    for (auto request : requests)
    {
        std::shared_ptr<stream_profile_interface> snapshot;
        request->create_snapshot(snapshot);
        //TODO: handle non video profiles
        m_device_record_snapshot_handler(RS2_EXTENSION_VIDEO_PROFILE, std::dynamic_pointer_cast<extension_snapshot>(snapshot), [this](const std::string& err) { stop_with_error(err); });
    }
}

void librealsense::record_sensor::close()
{
    m_sensor.close();
    m_is_recording = false;
}

librealsense::option& librealsense::record_sensor::get_option(rs2_option id)
{
//    auto option_it = m_recordable_options_cache.find(id);
//    if(option_it != m_recordable_options_cache.end())
//    {
//        return *option_it->second;
//    }
//    std::shared_ptr<option> option;
//    m_sensor.get_option(id).create_recordable(option, [this](std::shared_ptr<extension_snapshot> snapshot) {
//        record_snapshot(RS2_EXTENSION_OPTION, snapshot);
//    });
//    m_recordable_options_cache[id] = option;
//    return *option;
    return m_sensor.get_option(id);
}
const librealsense::option& librealsense::record_sensor::get_option(rs2_option id) const
{
    return m_sensor.get_option(id);
}
const std::string& librealsense::record_sensor::get_info(rs2_camera_info info) const
{
    return m_sensor.get_info(info);
}
bool librealsense::record_sensor::supports_info(rs2_camera_info info) const
{
    return m_sensor.supports_info(info);
}
bool librealsense::record_sensor::supports_option(rs2_option id) const
{
    return m_sensor.supports_option(id);
}

void librealsense::record_sensor::register_notifications_callback(notifications_callback_ptr callback)
{
    m_user_notification_callback = std::move(callback);
    std::unique_ptr<rs2_notifications_callback, void(*)(rs2_notifications_callback*)> cb(new notification_callback([&](rs2_notification* n)
    {
        if(m_user_notification_callback)
            m_user_notification_callback->on_notification(n);
    }), [](rs2_notifications_callback* p) { p->release(); });
    m_sensor.register_notifications_callback(std::move(cb));
}

void librealsense::record_sensor::start(frame_callback_ptr callback)
{
    if (m_frame_callback != nullptr)
    {
        return; //already started
    }

    //TODO: Also handle case where live sensor is already streaming

    auto record_cb = [this, callback](frame_holder frame)
    {
        record_frame(frame.clone());

        //Raise to user callback
        frame_interface* ref = nullptr;
        std::swap(frame.frame, ref);
        callback->on_frame((rs2_frame*)ref);
    };

    m_frame_callback = std::make_shared<frame_holder_callback>(record_cb);
    m_sensor.start(m_frame_callback);
}

void librealsense::record_sensor::stop()
{
    m_sensor.stop();
    m_frame_callback.reset();
}

bool librealsense::record_sensor::is_streaming() const
{
    return m_sensor.is_streaming();
}

bool librealsense::record_sensor::extend_to(rs2_extension extension_type, void** ext)
{
    //extend_to is called when the device\sensor above the hour glass are expected to provide some extension interface
    //In this case we should return a recordable version of that extension
    switch (extension_type)
    {
    case RS2_EXTENSION_DEBUG :
    {
        auto ptr = dynamic_cast<debug_interface*>(&m_sensor);
        if (!ptr)
        {
            return false;
        }
        std::shared_ptr<debug_interface> api;
        ptr->create_recordable(api, [this, extension_type](std::shared_ptr<extension_snapshot> e)
        {
            record_snapshot(extension_type, e);
        });
        //TODO: Ziv, Verify this doesn't result in memory leaks / memory corruptions
        *ext = api.get();
        return true;
    }
    case RS2_EXTENSION_INFO : *ext = this; return true;
    case RS2_EXTENSION_MOTION :break;
    case RS2_EXTENSION_OPTIONS :*ext = this; return true;
    case RS2_EXTENSION_VIDEO :break;
    case RS2_EXTENSION_ROI :break;
    case RS2_EXTENSION_DEPTH_SENSOR: break;
    case RS2_EXTENSION_ADVANCED_MODE: break;
    case RS2_EXTENSION_PLAYBACK: break;
    case RS2_EXTENSION_RECORD:break;
    case RS2_EXTENSION_COUNT :

    default:
        throw invalid_value_exception(to_string() <<"extension_type " << extension_type << " is not supported");
    }
    return false;
}

const device_interface& record_sensor::get_device()
{
    return m_parent_device;
}

void record_sensor::raise_user_notification(const std::string& str)
{
    notification noti(RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR, 0, RS2_LOG_SEVERITY_ERROR, str);
    rs2_notification rs2_noti(&noti);
    if(m_user_notification_callback) m_user_notification_callback->on_notification(&rs2_noti);
}

void librealsense::record_sensor::record_snapshot(rs2_extension extension_type, const std::shared_ptr<extension_snapshot>& snapshot)
{
    if(m_is_recording)
    {    //Send to recording thread
        m_device_record_snapshot_handler(extension_type, snapshot, [this](const std::string& err)
        { stop_with_error(err); });
    }
}

std::vector<platform::stream_profile> record_sensor::convert_profiles(const std::vector<stream_profile>& profiles)
{
    std::vector<platform::stream_profile> platform_profiles;
    //TODO: Implement
    return platform_profiles;
}
void record_sensor::stop_with_error(const std::string& error_msg)
{
    m_is_recording = false;
    raise_user_notification(to_string() << "Stopping recording for sensor (streaming will continue). (Error: " << error_msg << ")");
}
void record_sensor::record_frame(frame_holder frame)
{
    if(m_is_recording)
    {
        //Send to recording thread
        m_record_callback(std::move(frame), [this](const std::string& err){ stop_with_error(err); });
    }
}
