// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "playback_sensor.h"
#include "core/motion.h"
#include "api.h"
#include "software-device.h"
#include <map>
#include "types.h"
#include "context.h"
#include "ds5/ds5-options.h"
#include "media/ros/ros_reader.h"

using namespace librealsense;

std::string profile_to_string(std::shared_ptr<stream_profile_interface> s)
{
    std::ostringstream os;
    if (s != nullptr)
    {
        os << s->get_unique_id() << ", " <<
            s->get_format() << ", " <<
            s->get_stream_type() << "_" <<
            s->get_stream_index() << " @ " <<
            s->get_framerate();
    }
    return os.str();
}

playback_sensor::playback_sensor(const device_interface& parent_device, const device_serializer::sensor_snapshot& sensor_description):
    m_is_started(false),
    m_sensor_description(sensor_description),
    m_sensor_id(sensor_description.get_sensor_index()),
    m_parent_device(parent_device)
{
    register_sensor_streams(m_sensor_description.get_stream_profiles());
    register_sensor_infos(m_sensor_description);
    register_sensor_options(m_sensor_description);
    LOG_DEBUG("Created Playback Sensor " << m_sensor_id);
}
playback_sensor::~playback_sensor()
{
}

stream_profiles playback_sensor::get_stream_profiles() const
{
    return m_available_profiles;
}

void playback_sensor::open(const stream_profiles& requests)
{
    //Playback can only play the streams that were recorded.
    //Go over the requested profiles and see if they are available
    LOG_DEBUG("Open Sensor " << m_sensor_id);

    for (auto&& r : requests)
    {
        if (std::find_if(std::begin(m_available_profiles),
            std::end(m_available_profiles),
            [r](const std::shared_ptr<stream_profile_interface>& s) { return r->get_unique_id() == s->get_unique_id(); }) == std::end(m_available_profiles))
        {
            throw std::runtime_error(to_string() << "Failed to open sensor, requested profile: " << profile_to_string(r) << " is not available");
        }
    }
    std::vector<device_serializer::stream_identifier> opened_streams;
    //For each stream, create a dedicated dispatching thread
    for (auto&& profile : requests)
    {
        m_dispatchers.emplace(std::make_pair(profile->get_unique_id(), std::make_shared<dispatcher>(10))); //TODO: what size the queue should be?
        m_dispatchers[profile->get_unique_id()]->start();
        device_serializer::stream_identifier f{ get_device_index(), m_sensor_id, profile->get_stream_type(), static_cast<uint32_t>(profile->get_stream_index()) };
        opened_streams.push_back(f);
    }
    m_active_streams = requests;
    opened(opened_streams);
}

void playback_sensor::close()
{
    LOG_DEBUG("Close sensor " << m_sensor_id);
    std::vector<device_serializer::stream_identifier> closed_streams;
    for (auto dispatcher : m_dispatchers)
    {
        dispatcher.second->flush();
        for (auto available_profile : m_available_profiles)
        {
            if (available_profile->get_unique_id() == dispatcher.first)
            {
                closed_streams.push_back({ get_device_index(), m_sensor_id, available_profile->get_stream_type(), static_cast<uint32_t>(available_profile->get_stream_index()) });
            }
        }
    }
    m_dispatchers.clear();
    m_active_streams.clear();
    closed(closed_streams);
}

void playback_sensor::register_notifications_callback(notifications_callback_ptr callback)
{
    LOG_DEBUG("register_notifications_callback for sensor " << m_sensor_id);
    _notifications_processor.set_callback(std::move(callback));
}

notifications_callback_ptr playback_sensor::get_notifications_callback() const
{
    return _notifications_processor.get_callback();
}


void playback_sensor::start(frame_callback_ptr callback)
{
    LOG_DEBUG("Start sensor " << m_sensor_id);
    if (m_is_started == false)
    {
        started(m_sensor_id, callback);
        m_is_started = true;
        m_user_callback = callback;
    }
}
void playback_sensor::stop(bool invoke_required)
{
    LOG_DEBUG("Stop sensor " << m_sensor_id);

    if (m_is_started == true)
    {
        stopped(m_sensor_id, invoke_required);
        m_is_started = false;
        for (auto dispatcher : m_dispatchers)
        {
            dispatcher.second->stop();
        }
        m_user_callback.reset();
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
    return playback_device::try_extend_snapshot(e, extension_type, ext);
}

const device_interface& playback_sensor::get_device()
{
    return m_parent_device;
}

void playback_sensor::handle_frame(frame_holder frame, bool is_real_time)
{
    if(frame == nullptr)
    {
        throw invalid_value_exception("null frame passed to handle_frame");
    }
    if(m_is_started)
    {
        frame->get_owner()->set_sensor(shared_from_this());
        auto type = frame->get_stream()->get_stream_type();
        auto index = static_cast<uint32_t>(frame->get_stream()->get_stream_index());
        frame->set_stream(m_streams[std::make_pair(type, index)]);
        frame->set_sensor(shared_from_this());
        auto stream_id = frame.frame->get_stream()->get_unique_id();
        //TODO: Ziv, remove usage of shared_ptr when frame_holder is cpoyable
        auto pf = std::make_shared<frame_holder>(std::move(frame));
        m_dispatchers.at(stream_id)->invoke([this, pf](dispatcher::cancellable_timer t)
        {
            frame_interface* pframe = nullptr;
            std::swap((*pf).frame, pframe);
            m_user_callback->on_frame((rs2_frame*)pframe);
        });
        if(is_real_time)
        {
            m_dispatchers.at(stream_id)->flush();
        }
    }
}
void playback_sensor::update_option(rs2_option id, std::shared_ptr<option> option)
{
    register_option(id, option);
}
void playback_sensor::flush_pending_frames()
{
    for (auto&& dispatcher : m_dispatchers)
    {
        dispatcher.second->flush();
    }
}

void playback_sensor::register_sensor_streams(const stream_profiles& profiles)
{
    for (auto profile : profiles)
    {
        profile->set_unique_id(environment::get_instance().generate_stream_id());
        m_available_profiles.push_back(profile);
        m_streams[std::make_pair(profile->get_stream_type(), static_cast<uint32_t>(profile->get_stream_index()))] = profile;
        LOG_DEBUG("Added new stream: " << profile_to_string(profile));
    }
}

void playback_sensor::register_sensor_infos(const device_serializer::sensor_snapshot& sensor_snapshot)
{
    auto info_snapshot = sensor_snapshot.get_sensor_extensions_snapshots().find(RS2_EXTENSION_INFO);
    if (info_snapshot == nullptr)
    {
        LOG_WARNING("Recorded file does not contain sensor information");
        return;
    }
    auto info_api = As<info_interface>(info_snapshot);
    if (info_api == nullptr)
    {
        throw invalid_value_exception("Failed to get info interface from sensor snapshots");
    }

    for (int i = 0; i < RS2_CAMERA_INFO_COUNT; ++i)
    {
        rs2_camera_info info = static_cast<rs2_camera_info>(i);
        if (info_api->supports_info(info))
        {
            const std::string& str = info_api->get_info(info);
            register_info(info, str);
            LOG_DEBUG("Registered " << info << " for sensor " << m_sensor_id << " with value: " << str);
        }
    }
}

void playback_sensor::register_sensor_options(const device_serializer::sensor_snapshot& sensor_snapshot)
{
    auto options_snapshot = sensor_snapshot.get_sensor_extensions_snapshots().find(RS2_EXTENSION_OPTIONS);
    if (options_snapshot == nullptr)
    {
        LOG_WARNING("Recorded file does not contain sensor options");
        return;
    }
    auto options_api = As<options_interface>(options_snapshot);
    if (options_api == nullptr)
    {
        throw invalid_value_exception("Failed to get options interface from sensor snapshots");
    }

    for (int i = 0; i < static_cast<int>(RS2_OPTION_COUNT); i++)
    {
        auto option_id = static_cast<rs2_option>(i);
        try
        {
            if (options_api->supports_option(option_id))
            {
                auto&& option = options_api->get_option(option_id);
                float value = option.query();
                register_option(option_id, std::make_shared<const_value_option>(option.get_description(), option.query()));
                LOG_DEBUG("Registered " << rs2_option_to_string(option_id) << " for sensor " << m_sensor_id << " with value: " << option.query());
            }
        }
        catch (std::exception& e)
        {
            LOG_WARNING("Failed to register option " << option_id << ". Exception: " << e.what());
        }
    }
}

void playback_sensor::update(const device_serializer::sensor_snapshot& sensor_snapshot)
{
    register_sensor_options(sensor_snapshot);
}

frame_callback_ptr playback_sensor::get_frames_callback() const
{
    return m_user_callback;
}
void playback_sensor::set_frames_callback(frame_callback_ptr callback)
{
    m_user_callback = callback;
}
stream_profiles playback_sensor::get_active_streams() const
{
    return m_active_streams;
}

int playback_sensor::register_before_streaming_changes_callback(std::function<void(bool)> callback)
{
    throw librealsense::not_implemented_exception("playback_sensor::register_before_streaming_changes_callback");

}

void playback_sensor::unregister_before_start_callback(int token)
{
    throw librealsense::not_implemented_exception("playback_sensor::unregister_before_start_callback");
}

void playback_sensor::raise_notification(const notification& n)
{
    _notifications_processor.raise_notification(n);
}

rs2_extension playback_sensor::get_sensor_type()
{
    if (VALIDATE_INTERFACE_NO_THROW(this, librealsense::depth_sensor)) return RS2_EXTENSION_DEPTH_SENSOR;
    else if (VALIDATE_INTERFACE_NO_THROW(this, librealsense::depth_stereo_sensor)) return RS2_EXTENSION_DEPTH_STEREO_SENSOR;
    else if (VALIDATE_INTERFACE_NO_THROW(this, librealsense::video_sensor_interface)) return RS2_EXTENSION_VIDEO;
    else if (VALIDATE_INTERFACE_NO_THROW(this, librealsense::software_sensor)) return RS2_EXTENSION_SOFTWARE_SENSOR;
    else return RS2_EXTENSION_UNKNOWN;
}
