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
    m_parent_device(device),
    m_is_sensor_hooked(false),
    m_before_start_callback_token(-1)
{
    enable_sensor_options_recording();
    m_before_start_callback_token = m_sensor.register_before_streaming_changes_callback([this](bool streaming)
    {
        if (streaming)
        {
            enable_sensor_hooks();
        }
        else
        {
            disable_sensor_hooks();
        }
    });

    if (m_sensor.is_streaming())
    {
        //In case the sensor is already streaming, 
        // we will not get the above callback (before start) so we hook it now
        enable_sensor_hooks();
    }
    LOG_DEBUG("Created record_sensor");
}

librealsense::record_sensor::~record_sensor()
{
    m_sensor.unregister_before_start_callback(m_before_start_callback_token);
    disable_sensor_options_recording();
    disable_sensor_hooks();
    m_is_recording = false;
    LOG_DEBUG("Destructed record_sensor");
}

stream_profiles record_sensor::get_stream_profiles() const
{
    return m_sensor.get_stream_profiles();
}

void librealsense::record_sensor::open(const stream_profiles& requests)
{
    m_sensor.open(requests);
}

void librealsense::record_sensor::close()
{
    m_sensor.close();
}

librealsense::option& librealsense::record_sensor::get_option(rs2_option id)
{
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
    //TODO: Wrap notification callback (copy from future)
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
    m_sensor.start(callback);
}
void librealsense::record_sensor::stop()
{
    m_sensor.stop();
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
        recordable->enable_recording([this](const EXT_TYPE& ext1) {
            record_snapshot<EXT_TYPE>(E, ext1);
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

    //case RS2_EXTENSION_DEBUG           : return extend_to_aux<RS2_EXTENSION_DEBUG          >(&m_sensor, ext);
    //TODO: Add once implements recordable: case RS2_EXTENSION_MOTION          : return extend_to_aux<RS2_EXTENSION_MOTION         >(m_device, ext);
    //case RS2_EXTENSION_MOTION          : return extend_to_aux<RS2_EXTENSION_MOTION         >(&m_sensor, ext);
    //case RS2_EXTENSION_VIDEO           : return extend_to_aux<RS2_EXTENSION_VIDEO          >(&m_sensor, ext);
    //case RS2_EXTENSION_ROI             : return extend_to_aux<RS2_EXTENSION_ROI            >(&m_sensor, ext);
    case RS2_EXTENSION_DEPTH_SENSOR    : return extend_to_aux<RS2_EXTENSION_DEPTH_SENSOR   >(&m_sensor, ext);
    case RS2_EXTENSION_DEPTH_STEREO_SENSOR: return extend_to_aux<RS2_EXTENSION_DEPTH_STEREO_SENSOR   >(&m_sensor, ext);
    //case RS2_EXTENSION_VIDEO_FRAME     : return extend_to_aux<RS2_EXTENSION_VIDEO_FRAME    >(&m_sensor, ext);
    //case RS2_EXTENSION_MOTION_FRAME    : return extend_to_aux<RS2_EXTENSION_MOTION_FRAME   >(&m_sensor, ext);
    //case RS2_EXTENSION_COMPOSITE_FRAME : return extend_to_aux<RS2_EXTENSION_COMPOSITE_FRAME>(&m_sensor, ext);
    //case RS2_EXTENSION_POINTS          : return extend_to_aux<RS2_EXTENSION_POINTS         >(&m_sensor, ext);
    //case RS2_EXTENSION_DEPTH_FRAME     : return extend_to_aux<RS2_EXTENSION_DEPTH_FRAME    >(&m_sensor, ext);
    //case RS2_EXTENSION_ADVANCED_MODE   : return extend_to_aux<RS2_EXTENSION_ADVANCED_MODE  >(&m_sensor, ext);
    //case RS2_EXTENSION_VIDEO_PROFILE   : return extend_to_aux<RS2_EXTENSION_VIDEO_PROFILE  >(&m_sensor, ext);
    //case RS2_EXTENSION_PLAYBACK        : return extend_to_aux<RS2_EXTENSION_PLAYBACK       >(&m_sensor, ext);
    //case RS2_EXTENSION_RECORD          : return extend_to_aux<RS2_EXTENSION_PLAYBACK       >(&m_sensor, ext);
    default:
        LOG_WARNING("Extensions type is unhandled: " << extension_type);
        return false;
    }
}

const device_interface& record_sensor::get_device()
{
    return m_parent_device;
}

frame_callback_ptr record_sensor::get_frames_callback() const
{
    return m_frame_callback;
}

void record_sensor::set_frames_callback(frame_callback_ptr callback)
{
    m_frame_callback = callback;
}

stream_profiles record_sensor::get_active_streams() const
{
    return m_sensor.get_active_streams();
}

int record_sensor::register_before_streaming_changes_callback(std::function<void(bool)> callback)
{
    throw librealsense::not_implemented_exception("playback_sensor::register_before_streaming_changes_callback");

}

void record_sensor::unregister_before_start_callback(int token)
{
    throw librealsense::not_implemented_exception("playback_sensor::unregister_before_start_callback");
}

void record_sensor::raise_user_notification(const std::string& str)
{
    notification noti(RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR, 0, RS2_LOG_SEVERITY_ERROR, str);
    rs2_notification rs2_noti(&noti);
    if(m_user_notification_callback) m_user_notification_callback->on_notification(&rs2_noti);
}

template <typename T>
void librealsense::record_sensor::record_snapshot(rs2_extension extension_type, const recordable<T>& ext)
{
    std::shared_ptr<T> snapshot;
    ext.create_snapshot(snapshot);
    auto ext_snapshot = As<extension_snapshot>(snapshot);
    if(m_is_recording)
    {    
        //Send to recording thread
        m_device_record_snapshot_handler(extension_type,
                                        ext_snapshot,
                                        [this](const std::string& err)
                                        {
                                            stop_with_error(err);
                                        });
    }
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

void record_sensor::enable_sensor_hooks()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_is_sensor_hooked)
        return;
    hook_sensor_callbacks();
    wrap_streams();
    m_is_sensor_hooked = true;
}
void record_sensor::disable_sensor_hooks()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_is_sensor_hooked)
        return;
    unhook_sensor_callbacks();
    m_is_sensor_hooked = false;
}
void record_sensor::hook_sensor_callbacks()
{
    //TODO: wrap_notification_callback (copy from future)
    m_original_callback = m_sensor.get_frames_callback();
    if (m_original_callback)
    {
        m_frame_callback = wrap_frame_callback(m_original_callback);
        m_sensor.set_frames_callback(m_frame_callback);
        m_is_recording = true;
    }
}
frame_callback_ptr librealsense::record_sensor::wrap_frame_callback(frame_callback_ptr callback)
{
    auto record_cb = [this, callback](frame_holder frame)
    {
        record_frame(frame.clone());

        //Raise to user callback
        frame_interface* ref = nullptr;
        std::swap(frame.frame, ref);
        callback->on_frame((rs2_frame*)ref);
    };

    return std::make_shared<frame_holder_callback>(record_cb);
}
void record_sensor::unhook_sensor_callbacks()
{
    if (m_original_callback)
    {
        m_sensor.set_frames_callback(m_original_callback);
        m_original_callback.reset();
    }
}
void record_sensor::enable_sensor_options_recording()
{
    for (int i = 0; i < static_cast<int>(RS2_OPTION_COUNT); i++)
    {
        rs2_option id = static_cast<rs2_option>(i);
        if (!m_sensor.supports_option(id))
        {
            continue;
        }

        if (m_recording_options.find(id) != m_recording_options.end())
        {
            continue;
        }

        try
        {
            auto& opt = m_sensor.get_option(id);
            opt.enable_recording([this, id](const librealsense::option& option) {
                options_container options;
                std::shared_ptr<librealsense::option> option_snapshot;
                option.create_snapshot(option_snapshot);
                options.register_option(id, option_snapshot);
                record_snapshot<options_interface>(RS2_EXTENSION_OPTIONS, options);
            });
            m_recording_options.insert(id);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to enable recording for option " << get_string(id));
        }
    }
}
void record_sensor::disable_sensor_options_recording()
{
    for (auto id : m_recording_options)
    {
        auto& option = m_sensor.get_option(id);
        option.enable_recording([](const librealsense::option& snapshot) {});
    }
}
void record_sensor::wrap_streams()
{
    auto streams = m_sensor.get_active_streams();
    for (auto stream : streams)
    {
        auto id = stream->get_unique_id();
        if (m_recorded_streams_ids.count(id) == 0)
        {
            std::shared_ptr<stream_profile_interface> snapshot;
            stream->create_snapshot(snapshot);
            //TODO: handle non video profiles
           m_device_record_snapshot_handler(RS2_EXTENSION_VIDEO_PROFILE, std::dynamic_pointer_cast<extension_snapshot>(snapshot), [this](const std::string& err) { stop_with_error(err); });
           m_recorded_streams_ids.insert(id);
        }
    }
}

