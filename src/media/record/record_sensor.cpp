// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "record_sensor.h"
#include "api.h"

librealsense::record_sensor::record_sensor(const device_interface& device,
                                            sensor_interface& sensor, 
                                            frame_interface_callback_t on_frame, 
                                            std::function<void(rs2_extension, const std::shared_ptr<extension_snapshot>&)>) :
    m_device_record_snapshot_handler(),
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

std::vector<librealsense::stream_profile> librealsense::record_sensor::get_principal_requests()
{
    return m_sensor.get_principal_requests();
}

void librealsense::record_sensor::open(const std::vector<librealsense::stream_profile>& requests)
{
    m_sensor.open(requests);
    m_curr_configurations = convert_profiles(requests);
    //raise_user_notification("Opened sensor");
    //TODO: write to file
}
void librealsense::record_sensor::close()
{
    m_sensor.close();
}
librealsense::option& librealsense::record_sensor::get_option(rs2_option id)
{
    //std::shared_ptr<option> o;
    //m_sensor.get_option(id).create_recordable(o, [this](std::shared_ptr<extension_snapshot> e) {
    //    m_record_callback();
    //});
    //return o;
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
    
    //TODO: Handle case where live sensor is already streaming
    auto record_cb = [this, callback](frame_holder frame)
    {
        m_record_callback(frame.clone());
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
        //TODO: Ziv, Verify this doesn't result in memory leaks
        *ext = api.get();
        return true;
    }
    case RS2_EXTENSION_INFO : *ext = this; return true;
    case RS2_EXTENSION_MOTION :break;
    case RS2_EXTENSION_OPTIONS :*ext = this; return true;
    case RS2_EXTENSION_VIDEO :break;
    case RS2_EXTENSION_ROI :break;
    case RS2_EXTENSION_UNKNOWN:
    case RS2_EXTENSION_COUNT :
    default:
        throw invalid_value_exception(std::string("extension_type ") + std::to_string(extension_type) + " is not supported");
    }
    return false;
}

const device_interface& record_sensor::get_device()
{
    return m_parent_device;
}

rs2_extrinsics record_sensor::get_extrinsics_to(rs2_stream from, const sensor_interface& other, rs2_stream to) const
{
    return m_sensor.get_extrinsics_to(from, other, to);
}

const std::vector<platform::stream_profile>& record_sensor::get_curr_configurations() const
{
    return m_curr_configurations;
}

void record_sensor::raise_user_notification(const std::string& str)
{
    notification noti(RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR, 0, RS2_LOG_SEVERITY_ERROR, str);
    rs2_notification rs2_noti(&noti);
    if(m_user_notification_callback) m_user_notification_callback->on_notification(&rs2_noti);
}

void librealsense::record_sensor::record_snapshot(rs2_extension extension_type, const std::shared_ptr<extension_snapshot>& snapshot)
{
    m_device_record_snapshot_handler(extension_type, snapshot);
}

std::vector<platform::stream_profile> record_sensor::convert_profiles(const std::vector<stream_profile>& profiles)
{
    std::vector<platform::stream_profile> platform_profiles;
    //TODO: Implement
    return platform_profiles;
}
