// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "playback_sensor.h"
#include "core/motion.h"
#include <map>
#include "types.h"

playback_sensor::playback_sensor(const device_interface& parent_device, const sensor_snapshot& sensor_description, uint32_t sensor_id):
    m_sensor_description(sensor_description),
    m_sensor_id(sensor_id),
    m_is_started(false),
    m_user_notification_callback(nullptr, [](rs2_notifications_callback* n) {}),
    m_parent_device(parent_device)
{
}
playback_sensor::~playback_sensor()
{
}

std::vector<stream_profile> playback_sensor::get_principal_requests() 
{
    return m_sensor_description.get_streamig_profiles();
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
    for (auto&& profile : requests)
    {
        m_dispatchers.emplace(std::make_pair(profile.stream, std::make_shared<dispatcher>(1))); //TODO: what size the queue should be?
        m_dispatchers[profile.stream]->start();
    }
    
    opened(m_sensor_id, requests);
}

void playback_sensor::close()
{
    m_dispatchers.clear();
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
    auto snapshot = m_sensor_description.get_sensor_extensions_snapshots().find(RS2_EXTENSION_TYPE_OPTIONS);
    if (snapshot == nullptr)
        return false;
    auto option = std::dynamic_pointer_cast<librealsense::options_interface>(snapshot);
    if (option == nullptr)
        return false;
    return option->supports_option(id);
}

void playback_sensor::register_notifications_callback(notifications_callback_ptr callback)
{
    m_user_notification_callback = std::move(callback);
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
void playback_sensor::stop(bool invoke_required)
{
    if (m_is_started == true)
    {
        stopped(m_sensor_id, invoke_required);
        m_is_started = false;
    }
}
void playback_sensor::stop()
{
    stop(true);
}

bool playback_sensor::is_streaming() const
{
    return m_is_started;
}
bool playback_sensor::extend_to(rs2_extension extension_type, void** ext)
{
    std::shared_ptr<extension_snapshot> e = m_sensor_description.get_sensor_extensions_snapshots().find(extension_type);
    if(e == nullptr)
    {
        return false;
    }
    switch (extension_type)
    {
    case RS2_EXTENSION_TYPE_UNKNOWN: return false;
    case RS2_EXTENSION_TYPE_DEBUG: return try_extend<debug_interface>(e, ext);
    case RS2_EXTENSION_TYPE_INFO: return try_extend<info_interface>(e, ext);
    case RS2_EXTENSION_TYPE_MOTION: return try_extend<motion_sensor_interface>(e, ext);;
    case RS2_EXTENSION_TYPE_OPTIONS: return try_extend<options_interface>(e, ext);;
    case RS2_EXTENSION_TYPE_VIDEO: return try_extend<video_sensor_interface>(e, ext);;
    case RS2_EXTENSION_TYPE_ROI: return try_extend<roi_sensor_interface>(e, ext);;
    case RS2_EXTENSION_TYPE_VIDEO_FRAME: return try_extend<video_frame>(e, ext);
 //TODO: RS2_EXTENSION_TYPE_MOTION_FRAME: return try_extend<motion_frame>(e, ext);
    case RS2_EXTENSION_TYPE_COUNT:
        //[[fallthrough]];
    default:
        LOG_WARNING("Unsupported extension type: " << extension_type);
        assert(0);
        return false;
    }
}

const device_interface& playback_sensor::get_device()
{
    return m_parent_device;
}

rs2_extrinsics playback_sensor::get_extrinsics_to(rs2_stream from, const sensor_interface& other, rs2_stream to) const
{
    throw not_implemented_exception(__FUNCTION__);
}

const std::vector<platform::stream_profile>& playback_sensor::get_curr_configurations() const
{
    throw not_implemented_exception(__FUNCTION__);
}

void playback_sensor::handle_frame(frame_holder frame, bool is_real_time)
{
    if(m_is_started)
    {
        auto stream_type = frame.frame->get_stream_type();
		//TODO: remove this once filter is implemented (which will only read streams that were 'open'ed 
    	if(m_dispatchers.find(stream_type) == m_dispatchers.end())
		{
			return;
		}
        //TODO: Ziv, remove usage of shared_ptr when frame_holder is cpoyable
        auto pf = std::make_shared<frame_holder>(std::move(frame));
        m_dispatchers.at(stream_type)->invoke([this, pf](dispatcher::cancellable_timer t)
        {
            frame_interface* pframe = nullptr;
            std::swap((*pf).frame, pframe);
            m_user_callback->on_frame((rs2_frame*)pframe);
        });
    }
}
