// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "record_sensor.h"
#include "api.h"
#include "stream.h"

librealsense::record_sensor::record_sensor(const device_interface& device,
                                            sensor_interface& sensor) :
    m_sensor(sensor),
    m_user_notification_callback(nullptr, [](rs2_notifications_callback* n) {}),
    m_is_recording(false),
    m_is_pause(false),
    m_parent_device(device)
{
    //override_notification_callback
    m_user_notification_callback = sensor.get_notifications_callback();
    register_notifications_callback(m_user_notification_callback);
    LOG_DEBUG("Created record_sensor");
}

librealsense::record_sensor::~record_sensor()
{
    for (auto id : m_recording_options)
    {
        auto& option = m_sensor.get_option(id);
        option.enable_recording([](const librealsense::option& snapshot) {});
    }
    if(m_user_notification_callback)
        m_sensor.register_notifications_callback(m_user_notification_callback);
}

stream_profiles record_sensor::get_stream_profiles() const
{
    return m_sensor.get_stream_profiles();
}

void librealsense::record_sensor::open(const stream_profiles& requests)
{
    m_sensor.open(requests);

    m_is_recording = true;
    for (auto request : requests)
    {
        std::shared_ptr<stream_profile_interface> snapshot;
        request->create_snapshot(snapshot);
        //TODO: handle non video profiles
        rs2_extension extension_type;
        if (Is<librealsense::video_stream_profile_interface>(request))
            extension_type = RS2_EXTENSION_VIDEO_PROFILE;
        else if (Is<librealsense::motion_stream_profile_interface>(request))
            extension_type = RS2_EXTENSION_MOTION_PROFILE;
        else if (Is<librealsense::pose_stream_profile_interface>(request))
            extension_type = RS2_EXTENSION_POSE_PROFILE;
     
        on_extension_change(extension_type, std::dynamic_pointer_cast<extension_snapshot>(snapshot));
    }
}

void librealsense::record_sensor::close()
{
    m_sensor.close();
    disable_recording();
}

librealsense::option& librealsense::record_sensor::get_option(rs2_option id)
{
    auto& option = m_sensor.get_option(id);
    if (m_recording_options.find(id) == m_recording_options.end())
    {
        option.enable_recording([this, id](const librealsense::option& option) {
            options_container options;
            std::shared_ptr<librealsense::option> option_snapshot;
            option.create_snapshot(option_snapshot);
            options.register_option(id, option_snapshot);
            record_snapshot<options_interface>(RS2_EXTENSION_OPTIONS, options);
        });
        m_recording_options.insert(id);
    }
    return option;
}
const librealsense::option& librealsense::record_sensor::get_option(rs2_option id) const
{
    return const_cast<librealsense::option&>(const_cast<librealsense::record_sensor*>(this)->get_option(id));
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
    auto from_live_sensor = notifications_callback_ptr(new notification_callback([&](rs2_notification* n)
    {
        if (m_is_recording)
        {
            on_notification(*(n->_notification));
        }
        if (m_user_notification_callback)
        {
            m_user_notification_callback->on_notification(n);
        }
    }), [](rs2_notifications_callback* p) { p->release(); });
    m_sensor.register_notifications_callback(std::move(from_live_sensor));
}

notifications_callback_ptr librealsense::record_sensor::get_notifications_callback() const
{
    return m_sensor.get_notifications_callback();
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

template <rs2_extension E, typename P>
bool librealsense::record_sensor::extend_to_aux(P* p, void** ext)
{
    using EXT_TYPE = typename ExtensionToType<E>::type;
    auto ptr = As<EXT_TYPE>(p);
    if (!ptr)
        return false;


    if (auto recordable = As<librealsense::recordable<EXT_TYPE>>(p))
    {
        recordable->enable_recording([this](const EXT_TYPE& ext) {
            record_snapshot(E, ext);
        });
    }

    *ext = ptr;
    return true;
}

bool librealsense::record_sensor::extend_to(rs2_extension extension_type, void** ext)
{
    /**************************************************************************************
     A record sensor wraps the live sensor, and should have the same functionalities.
     To do that, the record sensor implements the extendable_interface and when the user tries to
     treat the sensor as some extension, this function is called, and we return a pointer to the 
     live sensor's extension. If that extension is a recordable one, we also enable_recording for it.
    **************************************************************************************/
    
    switch (extension_type)
    {
    case RS2_EXTENSION_OPTIONS: // [[fallthrough]]
    case RS2_EXTENSION_INFO:    // [[fallthrough]]
        *ext = this;
        return true;
    case RS2_EXTENSION_DEPTH_SENSOR    : return extend_to_aux<RS2_EXTENSION_DEPTH_SENSOR   >(&m_sensor, ext);
    //Other extensions are not expected to be extensions of a sensor
    default:
        LOG_WARNING("Extensions type is unhandled: " << extension_type);
        return false;
    }
}

const device_interface& record_sensor::get_device()
{
    return m_parent_device;
}

template <typename T>
void librealsense::record_sensor::record_snapshot(rs2_extension extension_type, const T& ext)
{
    std::shared_ptr<T> snapshot;
    ext.create_snapshot(snapshot);
    auto ext_snapshot = As<extension_snapshot>(snapshot);
    if(m_is_recording)
    {
        //Send to recording thread
        on_extension_change(extension_type, ext_snapshot);
    }
}

void record_sensor::stop_with_error(const std::string& error_msg)
{
    disable_recording();
    if (m_user_notification_callback)
    {
        std::string msg = to_string() << "Stopping recording for sensor (streaming will continue). (Error: " << error_msg << ")";
        notification noti(RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR, 0, RS2_LOG_SEVERITY_ERROR, msg);
        rs2_notification rs2_noti(&noti);
        m_user_notification_callback->on_notification(&rs2_noti);
    }
}

void record_sensor::disable_recording()
{
    m_is_recording = false;
}

void record_sensor::record_frame(frame_holder frame)
{
    if(m_is_recording)
    {
        //Send to recording thread
        on_frame(std::move(frame));
    }
}