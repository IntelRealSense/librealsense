// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "playback_sensor.h"

playback_sensor::playback_sensor(const sensor_snapshot& sensor_description, uint32_t sensor_id):
    m_sensor_description(sensor_description),
    m_sensor_id(sensor_id),
    m_is_started(false),
    m_user_notification_callback(nullptr, [](rs2_notifications_callback* n) {})
//TODO: initialize rest of members
{
    //TODO: make sure sensor_description has info ,options and streaming
}
playback_sensor::~playback_sensor()
{

}

std::vector<stream_profile> playback_sensor::get_principal_requests() 
{
    throw not_implemented_exception(__FUNCTION__);
}

void playback_sensor::open(const std::vector<stream_profile>& requests) 
{
    auto available_profiles = m_sensor_description.get_streamig_profiles();
    for (auto&& r : requests)
    {
        if(std::find(std::begin(available_profiles), std::end(available_profiles), r) == std::end(available_profiles))
        {
            throw std::runtime_error("Failed to open sensor, requested profile is not available");
        }
    }
    m_dispatchers[requests[0].stream]->get()->start();
    opened(m_sensor_id, requests);
}

void playback_sensor::close()
{
    closed(m_sensor_id);
}

option& playback_sensor::get_option(rs2_option id)
{
    return std::dynamic_pointer_cast<librealsense::options_interface>(m_sensor_description.get_sensor_extensions_snapshots().get_snapshots()[RS2_EXTENSION_TYPE_OPTIONS])->get_option(id);
}

const option& playback_sensor::get_option(rs2_option id) const
{
    return std::dynamic_pointer_cast<librealsense::options_interface>(m_sensor_description.get_sensor_extensions_snapshots().get_snapshots()[RS2_EXTENSION_TYPE_OPTIONS])->get_option(id);
}

const std::string& playback_sensor::get_info(rs2_camera_info info) const
{
    return std::dynamic_pointer_cast<librealsense::info_interface>(m_sensor_description.get_sensor_extensions_snapshots().get_snapshots()[RS2_EXTENSION_TYPE_INFO])->get_info(info);
}

bool playback_sensor::supports_info(rs2_camera_info info) const
{
    return std::dynamic_pointer_cast<librealsense::info_interface>(m_sensor_description.get_sensor_extensions_snapshots().get_snapshots()[RS2_EXTENSION_TYPE_INFO])->supports_info(info);
}

bool playback_sensor::supports_option(rs2_option id) const
{
    return std::dynamic_pointer_cast<librealsense::options_interface>(m_sensor_description.get_sensor_extensions_snapshots().get_snapshots()[RS2_EXTENSION_TYPE_OPTIONS])->supports_option(id);
}

void playback_sensor::register_notifications_callback(notifications_callback_ptr callback)
{
    throw not_implemented_exception(__FUNCTION__);
}

void playback_sensor::start(frame_callback_ptr callback)
{
    if (m_is_started == false)
    {
        started(m_sensor_id, callback);
        m_is_started = true;
        m_user_callback = callback;
    }
}

void playback_sensor::stop()
{
    if (m_is_started == true)
    {
        stopped(m_sensor_id);
        m_is_started = false;
    }
}

bool playback_sensor::is_streaming() const
{
    return m_is_started;
}
bool playback_sensor::extend_to(rs2_extension_type extension_type, void** ext)
{
    throw not_implemented_exception(__FUNCTION__);
    //return false;
}

void playback_sensor::handle_frame(frame_holder frame, bool is_real_time)
{
    if(m_is_started)
    {
        auto pf = std::make_shared<frame_holder>(std::move(frame));
        m_dispatchers[frame.frame->get()->get_stream_type()]->get()->invoke([this, pf](dispatcher::cancellable_timer t)
        {
            m_user_callback->on_frame(std::move(*pf));
        });
    }
}
