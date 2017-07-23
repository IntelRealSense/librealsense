// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <cmath>
#include "playback_device.h"
#include "core/motion.h"


/**
*
*  Initial state is "Stopped"
*
*  Internal members meaning:
*
*               m_is_started  |     True     |      False
*  m_is_paused                |              |
*  ---------------------------|--------------|---------------
*      True                   |    Paused    |   Stopped
*      False                  |    Playing   |   Stopped
*
*
*  State Changes:
*
*      Playing ---->  start()   set m_is_started to True  ----> Do nothing
*      Playing ---->  stop()    set m_is_started to False ----> Stopped
*      Playing ---->  pause()   set m_is_paused  to True  ----> Paused
*      Playing ---->  resume()  set m_is_paused  to False ----> Do nothing
*
*      Paused  ---->  start()   set m_is_started to True  ----> Do nothing
*      Paused  ---->  stop()    set m_is_started to False ----> Stopped
*      Paused  ---->  pause()   set m_is_paused  to True  ----> Do nothing
*      Paused  ---->  resume()  set m_is_paused  to False ----> Playing
*
*      Stopped ---->  start()   set m_is_started to True  ----> Paused/Playing (depends on m_is_paused)
*      Stopped ---->  stop()    set m_is_started to False ----> Do nothing
*      Stopped ---->  pause()   set m_is_paused  to True  ----> Do nothing
*      Stopped ---->  resume()  set m_is_paused  to False ----> Do nothing
*
*/

/*******************************************************************************
 *TODO: Revise                  Playback Device                                *
 *                              ***************                                *
 *                                                                             *
 * playback device is an implementation of device interface which reads from a *
 * file to simulate a real device.                                             * 
 *                                                                             *
 * playback device holds playback sensors which simulate real sensors.         *
 *                                                                             *
 * when creating the playback device, it will read all of the device           *
 * description (device_snapshot) from the file to map itself and its sensors   *
 * in matters of functionality and data provided.                              *
 * when creating each sensor, the device will create a sensor from the         *
 * sensor's description inside the device_snapshot.                            *
 * In addition, each sensor will be given a "view" of the file which will allow*
 * the sensor to read only data that is relevant to it (e.g - first sensor will*
 * be given a view of the file that shows only the depth and ir frames stored  *
 * in the file). all views will be synchronized at the device level to provide *
 * chronologic order of the data.                                              *
 * to allow sensor's snapshot to be updated even when they are not streaming,  *
 * another view                                                                *
 * <TODO: complete>                                                            *
 *******************************************************************************/

playback_device::playback_device(std::shared_ptr<device_serializer::reader> serializer) :
    m_is_started(false), 
    m_is_paused(false), 
    m_sample_rate(1), 
    m_real_time(false),
    m_prev_timestamp(0),
    m_read_thread([]() {return std::make_shared<dispatcher>(std::numeric_limits<unsigned int>::max()); })
{
    if (serializer == nullptr)
    {
        throw invalid_value_exception("null serializer");
    }

    //serializer->reset();  
    m_reader = serializer;
    (*m_read_thread)->start();
    //Read header and build device from recorded device snapshot
    m_device_description = m_reader->query_device_description();
    //TODO: add support for file info
    //Create playback sensor that simulate the recorded sensors
    m_sensors = create_playback_sensors(m_device_description); 
}
std::map<uint32_t, std::shared_ptr<playback_sensor>> playback_device::create_playback_sensors(const device_snapshot& device_description)
{
    uint32_t sensor_id = 0;
    std::map<uint32_t, std::shared_ptr<playback_sensor>> sensors;
    for (auto sensor_snapshot : device_description.get_sensors_snapshots())
    {
        //Each sensor will know its capabilities from the sensor_snapshot
        auto sensor = std::make_shared<playback_sensor>(*this, sensor_snapshot, sensor_id);
 
        sensor->started += [this](uint32_t id, frame_callback_ptr user_callback) -> void
        {
            (*m_read_thread)->invoke([this, id, user_callback](dispatcher::cancellable_timer c)
            {
                auto it = m_active_sensors.find(id);
                if (it == m_active_sensors.end())
                {
                    m_active_sensors[id] = m_sensors[id];

                    if (m_active_sensors.size() == 1) //On the first sensor that starts, start the reading thread
                    {
                        start();
                        //if ( == false)
                        //{
                        //    //TODO: notify user and close thread
                        //    //s->stop();
                        //}
                    }
                }
            });
        };

        sensor->stopped += [this](uint32_t id, bool invoke_required) -> void
        {
            //stopped could be called by the user (when calling sensor.stop(), main thread) or from the reader_thread when reaching eof (which means invoke is not required)
            auto action = [this, id]()
            {
                auto it = m_active_sensors.find(id);
                if (it != m_active_sensors.end())
                {
                    m_active_sensors.erase(it);
                    if (m_active_sensors.size() == 0)
                    {
                        stop();
                    }
                }
            };
            if (invoke_required)
            {
                (*m_read_thread)->invoke([action](dispatcher::cancellable_timer c) { action(); });
            }
            else
            {
                action();
            }
        };

        sensor->opened += [this](int32_t id, const std::vector<stream_profile>& requested_profiles) -> void
        {
            (*m_read_thread)->invoke([this, id, requested_profiles](dispatcher::cancellable_timer c)
            {
                set_filter(id, requested_profiles);
            });
        };

        sensor->closed += [this](uint32_t id) -> void
        {
            (*m_read_thread)->invoke([this, id](dispatcher::cancellable_timer c)
            {
                set_filter(id, {});
            });
        };
        
        sensors[sensor_id++] = sensor;
    }
    return sensors;
}
playback_device::~playback_device()
{
    (*m_read_thread)->invoke([this](dispatcher::cancellable_timer c)
    {
        for (auto&& sensor : m_active_sensors)
        {
            if (sensor.second != nullptr)
                sensor.second->stop(); //TODO: make sure this works with this dispatcher
        }
    });
    (*m_read_thread)->flush();
    (*m_read_thread)->stop();
}
sensor_interface& playback_device::get_sensor(size_t i) 
{
    return *m_sensors.at(i);
}
size_t playback_device::get_sensors_count() const 
{
    return m_sensors.size();
}
const std::string& playback_device::get_info(rs2_camera_info info) const 
{
    return std::dynamic_pointer_cast<librealsense::info_interface>(m_device_description.get_device_extensions_snapshots().get_snapshots()[RS2_EXTENSION_INFO ])->get_info(info);
}
bool playback_device::supports_info(rs2_camera_info info) const 
{
    auto info_extension = m_device_description.get_device_extensions_snapshots().get_snapshots().at(RS2_EXTENSION_INFO );
    auto info_api = std::dynamic_pointer_cast<librealsense::info_interface>(info_extension);
    if(info_api == nullptr)
    {
        throw invalid_value_exception("Failed to get info interface");
    }
    return info_api->supports_info(info);
}
const sensor_interface& playback_device::get_sensor(size_t i) const
{
    auto sensor = m_sensors.at(static_cast<uint32_t>(i));
    return *std::dynamic_pointer_cast<sensor_interface>(sensor);
}
void playback_device::hardware_reset() 
{
    //Nothing to see here folks
}
rs2_extrinsics playback_device::get_extrinsics(size_t from, rs2_stream from_stream, size_t to, rs2_stream to_stream) const 
{
    throw not_implemented_exception(__FUNCTION__);
    //std::dynamic_pointer_cast<librealsense::info_interface>(m_device_description.get_device_extensions_snapshots().get_snapshots()[RS2_EXTENSION_EXTRINSICS ])->supports_info(info);
}

bool playback_device::extend_to(rs2_extension extension_type, void** ext) 
{
    std::shared_ptr<extension_snapshot> e = m_device_description.get_device_extensions_snapshots().find(extension_type);
    if (e == nullptr)
    {
        return false;
    }
    switch (extension_type)
    {
    case RS2_EXTENSION_UNKNOWN: return false;
    case RS2_EXTENSION_DEBUG : return try_extend<debug_interface>(e, ext);
    case RS2_EXTENSION_INFO : return try_extend<info_interface>(e, ext);
    case RS2_EXTENSION_MOTION : return try_extend<motion_sensor_interface>(e, ext);;
    case RS2_EXTENSION_OPTIONS : return try_extend<options_interface>(e, ext);;
    case RS2_EXTENSION_VIDEO : return try_extend<video_sensor_interface>(e, ext);;
    case RS2_EXTENSION_ROI : return try_extend<roi_sensor_interface>(e, ext);;
    case RS2_EXTENSION_VIDEO_FRAME : return try_extend<video_frame>(e, ext);
    //TODO: add: case RS2_EXTENSION_MOTION_FRAME : return try_extend<motion_frame>(e, ext);
    case RS2_EXTENSION_COUNT :
        //[[fallthrough]];
    default: 
        LOG_WARNING("Unsupported extension type: " << extension_type);
        return false;
    }
}

std::shared_ptr<matcher> playback_device::create_matcher(rs2_stream stream) const
{
    return nullptr; //TOOD: WTD?
}

void playback_device::set_frame_rate(double rate)
{
    if(rate < 0)
    {
        throw invalid_value_exception(to_string() << "Failed to set frame rate to " << std::to_string(rate) << ", value is less than 0");
    }
    m_sample_rate = rate;
}

void playback_device::seek_to_time(std::chrono::nanoseconds time)
{
    (*m_read_thread)->invoke([this, time](dispatcher::cancellable_timer t)
    {
        m_reader->seek_to_time(time);
        m_base_timestamp = 0;
    });
    (*m_read_thread)->flush();
}

rs2_playback_status playback_device::get_current_status() const
{
    rs2_playback_status current_status = RS2_PLAYBACK_STATUS_STOPPED;
    (*m_read_thread)->invoke([this,&current_status](dispatcher::cancellable_timer t)
    {
        return m_is_started
               ? (m_is_paused
                  ? RS2_PLAYBACK_STATUS_PAUSED
                  : RS2_PLAYBACK_STATUS_PLAYING)
               : RS2_PLAYBACK_STATUS_STOPPED;
    });
    (*m_read_thread)->flush();
    return current_status;
}

uint64_t playback_device::get_duration() const
{
    auto nanos = m_reader->query_duration();
    auto unanos = std::chrono::duration_cast<file_format::file_types::nanoseconds>(nanos);
    return unanos.count();
}
void playback_device::pause()
{
    /*
        Playing ---->  pause()   set m_is_paused  to True  ----> Paused
        Paused  ---->  pause()   set m_is_paused  to True  ----> Do nothing
        Stopped ---->  pause()   set m_is_paused  to True  ----> Do nothing
    */
    (*m_read_thread)->invoke([this](dispatcher::cancellable_timer t)
    {
       if (m_is_paused)
           return;

       m_is_paused = true;

       if(m_is_started)
       {
           //Wait for any remaining sensor callbacks to return
           for (auto sensor : m_sensors)
           {
               sensor.second->flush_pending_frames();
           }
           playback_status_changed(RS2_PLAYBACK_STATUS_PAUSED);
       }
    });
    (*m_read_thread)->flush();
}

void playback_device::resume()
{
    (*m_read_thread)->invoke([this](dispatcher::cancellable_timer t)
    {
        if (m_is_paused == false)
           return;

        m_is_paused = false;

        try_looping();
    });
    (*m_read_thread)->flush();
}

void playback_device::set_real_time(bool real_time)
{
    m_real_time = real_time;
}

bool playback_device::is_real_time() const 
{
    return m_real_time;
}

void playback_device::update_time_base(uint64_t base_timestamp)
{
    m_base_sys_time = std::chrono::high_resolution_clock::now();
    m_base_timestamp = base_timestamp;
}

int64_t playback_device::calc_sleep_time(const uint64_t& timestamp) const
{
    //The time to sleep returned here equals to the difference between the file recording time
    // and the playback time.
    auto now = std::chrono::high_resolution_clock::now();
    auto play_time = std::chrono::duration_cast<std::chrono::microseconds>(now - m_base_sys_time).count();
    int64_t time_diff = timestamp - m_base_timestamp;
    if (time_diff < 0)
    {
        return 0;
    }
    auto recorded_time = std::llround(static_cast<double>(time_diff) / m_sample_rate);
    int64_t sleep_time = (recorded_time - play_time);
    return sleep_time;
}

void playback_device::start()
{
    //Start reading from the file
    //Start is called only in from Stopped state
    /*
    Playing ---->  start()   set m_is_started to True  ----> Do nothing
    Paused  ---->  start()   set m_is_started to True  ----> Do nothing
    Stopped ---->  start()   set m_is_started to True  ----> Paused/Playing (depends on m_is_paused)
    */
    if (m_is_started)
        return ; //nothing to do

    m_is_started = true;
    m_base_timestamp = 0;
    try_looping();
    //return m_read_thread->invoke<bool>([this]()
    //{
    //    if (m_is_started)
    //        return true; //nothing to do

    //    m_is_started = true;
    //    m_base_timestamp = 0;

    //    try_looping();
    //    return true;
    //});
}

void playback_device::stop()
{
    if (m_is_started == false)
        return; //nothing to do


    m_is_started = false;
    for (auto sensor : m_sensors)
    {
        //TODO: sensor.second->flush_frame_callbacks();
    }
    m_reader->reset();
    playback_status_changed(RS2_PLAYBACK_STATUS_STOPPED);
}

template <typename T>
void playback_device::do_loop(T action)
{
    (*m_read_thread)->invoke([this, action](dispatcher::cancellable_timer c)
    {
        action();
        if (m_is_started == true && m_is_paused == false)
        {
            do_loop(action);
        }
    });
}
void playback_device::try_looping()
{
    //try_looping is called from start() or resume()
    if (m_is_started && m_is_paused == false)
    {
        //Notify subscribers that playback status changed
        if (m_is_paused)
        {
            playback_status_changed(RS2_PLAYBACK_STATUS_PAUSED);
        }
        else
        {
            playback_status_changed(RS2_PLAYBACK_STATUS_PLAYING);
        }
    }
    auto read_action = [this]() 
    {
        //Read next data from the serializer, on success: 'obj' will be a valid object that came from
        // sensor number 'sensor_index' with a timestamp equal to 'timestamp'
        uint32_t sensor_index;
        //TODO: change timestamp type to file_type::nanoseconds
        std::chrono::nanoseconds timestamp = std::chrono::nanoseconds::max();
        frame_holder frame;
        bool is_valid_read = true;
        try
        {
            auto retval = m_reader->read(timestamp, sensor_index, frame);
            if (retval ==file_format::status_file_read_failed)
            {
                throw io_exception("Failed to read next sample from file");
            }
            if (retval ==file_format::status_file_eof)
            {
                //End frame reader
                //TODO close frame reader and notify user
                is_valid_read = false;
            }
            is_valid_read = (retval ==file_format::status_no_error);
        }
        catch (const std::exception& e)
        {
            std::cerr << "Failed to read next frame: " << e.what() << std::endl;
            is_valid_read = false;
        }
        if(is_valid_read == false || timestamp == std::chrono::nanoseconds::max())
        {
            //Go over the sensors and stop them
            size_t active_sensors_count = m_active_sensors.size();
            for (size_t i = 0; i<active_sensors_count; i++)
            {
                if (m_active_sensors.size() == 0)
                    break;

                //NOTE: calling stop will remove the sensor from m_active_sensors
                m_active_sensors[i]->stop(false);
            }
            //After all sensors were stopped stop() is called and flags m_is_started as false
            assert(m_is_started == false);
            return; //Should stop the loop
        }
        m_prev_timestamp = timestamp;
        auto timestamp_micros = std::chrono::duration_cast<std::chrono::microseconds>(timestamp);
        //Objects with timestamp of 0 are non streams.
        if (m_base_timestamp == 0)
        {
            //As long as m_base_timestamp is 0, update it to object's timestamp.
            //Once a streaming object arrive, the base will change from 0
            
            update_time_base(timestamp_micros.count());
        }

        //Calculate the duration for the reader to sleep (i.e wait for next frame)
        auto sleep_time = calc_sleep_time(timestamp_micros.count());
        if (sleep_time > 0)
        {
            if (m_sample_rate > 0)
            {
                std::this_thread::sleep_for(std::chrono::microseconds(sleep_time));
            }
        }

        
        if (sensor_index >= m_sensors.size())
        {
            LOG_ERROR("Unexpected sensor index while playing file, sensor index = " << sensor_index);
            return;
        }
        //Pass the object to the

        m_sensors[sensor_index]->handle_frame(std::move(frame), m_real_time);
        
    };
    do_loop(read_action);
}

void playback_device::set_filter(int32_t id, const std::vector<stream_profile>& requested_profiles)
{
    (*m_read_thread)->invoke([this, id, requested_profiles](dispatcher::cancellable_timer c)
    {
        m_reader->set_filter(id, requested_profiles);
    });
}

const std::string& playback_device::get_file_name() const
{
    return m_reader->get_file_name();
}

uint64_t playback_device::get_position() const
{
    return m_prev_timestamp.count();
}
