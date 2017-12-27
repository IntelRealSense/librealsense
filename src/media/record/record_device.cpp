// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <core/debug.h>
#include <core/motion.h>
#include <core/advanced_mode.h>
#include "record_device.h"

using namespace device_serializer;

librealsense::record_device::record_device(std::shared_ptr<librealsense::device_interface> device,
                                      std::shared_ptr<librealsense::device_serializer::writer> serializer):
    m_write_thread([](){return std::make_shared<dispatcher>(std::numeric_limits<unsigned int>::max());}),
    m_is_recording(true),
    m_record_pause_time(0)
{
    if (device == nullptr)
    {
        throw invalid_value_exception("device is null");
    }

    if (serializer == nullptr)
    {
        throw invalid_value_exception("serializer is null");
    }

    m_device = device;
    m_ros_writer = serializer;
    m_sensors = create_record_sensors(m_device);
    (*m_write_thread)->start();
    LOG_DEBUG("Created record_device");
}

std::vector<std::shared_ptr<record_sensor>> record_device::create_record_sensors(std::shared_ptr<device_interface> device)
{
    std::vector<std::shared_ptr<record_sensor>> record_sensors;
    for (size_t sensor_index = 0; sensor_index < device->get_sensors_count(); sensor_index++)
    {
        auto& live_sensor = device->get_sensor(sensor_index);
        auto sensor_frame_handler = [this, sensor_index](frame_holder f, std::function<void(std::string const&)> on_error)
        {
            write_data(sensor_index, std::move(f), on_error);
        };

        auto sensor_snapshot_changes_handler = [this, sensor_index](rs2_extension ext, const std::shared_ptr<extension_snapshot>& snapshot, std::function<void(std::string const&)> on_error)
        {
            write_sensor_extension_snapshot(sensor_index, ext, snapshot, on_error);
        };

        auto recording_sensor = std::make_shared<record_sensor>(*this, live_sensor, sensor_frame_handler, sensor_snapshot_changes_handler);
        record_sensors.emplace_back(recording_sensor);
    }
    return record_sensors;
}

librealsense::record_device::~record_device()
{
    if((*m_write_thread)->flush() == false)
    {
        LOG_ERROR("Error - timeout waiting for flush, possible deadlock detected");
    }
    (*m_write_thread)->stop();
}

std::shared_ptr<context> librealsense::record_device::get_context() const
{
    return m_device->get_context();
}

librealsense::sensor_interface& librealsense::record_device::get_sensor(size_t i)
{
    return *(m_sensors.at(i));
}

size_t librealsense::record_device::get_sensors_count() const
{
    return m_sensors.size();
}

void librealsense::record_device::write_header()
{

    auto device_extensions_md = get_extensions_snapshots(m_device.get());
    LOG_DEBUG("Created device snapshot with " << device_extensions_md.get_snapshots().size() << " snapshots");

    std::vector<sensor_snapshot> sensors_snapshot;
    for (size_t j = 0; j < m_device->get_sensors_count(); ++j)
    {
        auto& sensor = m_device->get_sensor(j);
        auto sensor_extensions_snapshots = get_extensions_snapshots(&sensor);
        sensors_snapshot.emplace_back(static_cast<uint32_t>(j), sensor_extensions_snapshots);
        LOG_DEBUG("Created sensor " << j << " snapshot with " << device_extensions_md.get_snapshots().size() << " snapshots");
    }

    m_ros_writer->write_device_description({ device_extensions_md, sensors_snapshot, {/*extrinsics are written by ros_writer*/} });
}

//Returns the time relative to beginning of the recording
std::chrono::nanoseconds librealsense::record_device::get_capture_time() const
{
    if (m_capture_time_base.time_since_epoch() == std::chrono::nanoseconds::zero())
    {
        return std::chrono::nanoseconds::zero();
    }
    auto now = std::chrono::high_resolution_clock::now();
    return (now - m_capture_time_base) - m_record_pause_time;
}

void librealsense::record_device::write_data(size_t sensor_index, librealsense::frame_holder frame, std::function<void(std::string const&)> on_error)
{
    //write_data is called from the sensors, when the live sensor raises a frame

    LOG_DEBUG("write frame " << (frame ? std::to_string(frame.frame->get_frame_number()) : "") <<  " from sensor " << sensor_index);

    std::call_once(m_first_call_flag, [this]()
    {
        initialize_recording();
    });

    //TODO: restore: uint64_t data_size = frame.frame->get_frame_data_size();
    uint64_t cached_data_size = m_cached_data_size; //TODO: restore: (+ data_size)
    if (cached_data_size > MAX_CACHED_DATA_SIZE)
    {
        LOG_WARNING("Recorder reached maximum cache size, frame dropped");
        on_error("Recorder reached maximum cache size, frame dropped");
        return;
    }

    m_cached_data_size = cached_data_size;
    auto capture_time = get_capture_time();
    //TODO: remove usage of shared pointer when frame_holder is copyable
    auto frame_holder_ptr = std::make_shared<frame_holder>();
    *frame_holder_ptr = std::move(frame);
    (*m_write_thread)->invoke([this, frame_holder_ptr, sensor_index, capture_time/*, data_size*/, on_error](dispatcher::cancellable_timer t) {
        if (m_is_recording == false)
        {
            return; //Recording is paused
        }
        std::call_once(m_first_frame_flag, [&]()
        {
            try
            {
                write_header();
            }
            catch (const std::exception& e)
            {
                LOG_ERROR("Failed to write header. " << e.what());
                on_error(to_string() << "Failed to write header. " << e.what());
            }
        });

        try
        {
            const uint32_t device_index = 0;
            auto stream_type = frame_holder_ptr->frame->get_stream()->get_stream_type();
            auto stream_index = static_cast<uint32_t>(frame_holder_ptr->frame->get_stream()->get_stream_index());
            m_ros_writer->write_frame({ device_index, static_cast<uint32_t>(sensor_index), stream_type, stream_index }, capture_time, std::move(*frame_holder_ptr));
            //TODO: restore: std::lock_guard<std::mutex> locker(m_mutex);  m_cached_data_size -= data_size;
        }
        catch(std::exception& e)
        {
            on_error(to_string() << "Failed to write frame. " << e.what());
        }
    });
}

const std::string& librealsense::record_device::get_info(rs2_camera_info info) const
{
    //info has no setter, it does not change - nothing to record
    return m_device->get_info(info);
}

bool librealsense::record_device::supports_info(rs2_camera_info info) const
{
    //info has no setter, it does not change - nothing to record
    return m_device->supports_info(info);
}

const librealsense::sensor_interface& librealsense::record_device::get_sensor(size_t i) const
{
    return *m_sensors.at(i);
}
void librealsense::record_device::hardware_reset()
{
    m_device->hardware_reset();
}

template <typename T, typename Ext>
void librealsense::record_device::try_add_snapshot(T* extendable, snapshot_collection& snapshots)
{
    auto api = dynamic_cast<recordable<Ext>*>(extendable);

    if (api != nullptr)
    {
        std::shared_ptr<Ext> p;
        try
        {
            api->create_snapshot(p);
            auto snapshot = std::dynamic_pointer_cast<extension_snapshot>(p);
            if (snapshot != nullptr)
            {
                snapshots[TypeToExtension<Ext>::value] = snapshot;
                LOG_INFO("Added snapshot of type: " << TypeToExtension<Ext>::to_string());
            }
            else
            {
                LOG_ERROR("Failed to downcast snapshot of type " << TypeToExtension<Ext>::to_string());
            }
        }
        catch (const std::exception& e)
        {
            LOG_ERROR("Failed to add snapshot of type " << TypeToExtension<Ext>::to_string() << ". Exception: " << e.what());
        }
    }
}

/**
 * Go over the extendable instance and find all extensions
 * @tparam T
 * @param extendable
 * @return
 */
template<typename T>
snapshot_collection librealsense::record_device::get_extensions_snapshots(T* extendable)
{
    //No support for extensions with more than a single type - i.e every extension has exactly one type in rs2_extension
    snapshot_collection snapshots;
    for (int i = 0; i < static_cast<int>(RS2_EXTENSION_COUNT ); ++i)
    {
        rs2_extension ext = static_cast<rs2_extension>(i);
        switch (ext)
        {
            case RS2_EXTENSION_DEBUG           : try_add_snapshot<T, ExtensionToType<RS2_EXTENSION_DEBUG          >::type>(extendable, snapshots);
            case RS2_EXTENSION_INFO            : try_add_snapshot<T, ExtensionToType<RS2_EXTENSION_INFO           >::type>(extendable, snapshots);
            case RS2_EXTENSION_OPTIONS         : try_add_snapshot<T, ExtensionToType<RS2_EXTENSION_OPTIONS        >::type>(extendable, snapshots);
            //case RS2_EXTENSION_MOTION          : try_add_snapshot<T, ExtensionToType<RS2_EXTENSION_MOTION         >::type>(extendable, snapshots);
            //case RS2_EXTENSION_VIDEO           : try_add_snapshot<T, ExtensionToType<RS2_EXTENSION_VIDEO          >::type>(extendable, snapshots);
            //case RS2_EXTENSION_ROI             : try_add_snapshot<T, ExtensionToType<RS2_EXTENSION_ROI            >::type>(extendable, snapshots);
            case RS2_EXTENSION_DEPTH_SENSOR    : try_add_snapshot<T, ExtensionToType<RS2_EXTENSION_DEPTH_SENSOR   >::type>(extendable, snapshots);
            case RS2_EXTENSION_DEPTH_STEREO_SENSOR: try_add_snapshot<T, ExtensionToType<RS2_EXTENSION_DEPTH_STEREO_SENSOR   >::type>(extendable, snapshots);
            //case RS2_EXTENSION_ADVANCED_MODE   : try_add_snapshot<T, ExtensionToType<RS2_EXTENSION_ADVANCED_MODE  >::type>(extendable, snapshots);
            case RS2_EXTENSION_VIDEO_FRAME     : break;
            case RS2_EXTENSION_MOTION_FRAME    : break;
            case RS2_EXTENSION_COMPOSITE_FRAME : break;
            case RS2_EXTENSION_POINTS          : break;
            case RS2_EXTENSION_RECORD          : break;
            case RS2_EXTENSION_PLAYBACK        : break;
            case RS2_EXTENSION_COUNT           : break;
            case RS2_EXTENSION_UNKNOWN         : break;
            default:
                LOG_WARNING("Extensions type is unhandled: " << ext);
        }
    }
    return snapshots;
}

template <typename T>
void librealsense::record_device::write_device_extension_changes(const T& ext)
{
    std::shared_ptr<T> snapshot;
    ext.create_snapshot(snapshot);
    auto ext_snapshot = As<extension_snapshot>(snapshot);
    if (!ext_snapshot)
    {
        assert(0);
        return;
    }
    auto capture_time = get_capture_time();
    (*m_write_thread)->invoke([this, capture_time, ext_snapshot](dispatcher::cancellable_timer t)
    {
        try
        {
            const uint32_t device_index = 0;
            m_ros_writer->write_snapshot(device_index, capture_time, TypeToExtension<T>::value, ext_snapshot);
        }
        catch (const std::exception& e)
        {
            LOG_ERROR(e.what());
        }
    });
}

void record_device::write_sensor_extension_snapshot(size_t sensor_index,
    rs2_extension ext,
    const std::shared_ptr<extension_snapshot>& snapshot,
    std::function<void(std::string const&)> on_error)
{
    auto capture_time = get_capture_time();
    (*m_write_thread)->invoke([this, sensor_index, capture_time, ext, snapshot, on_error](dispatcher::cancellable_timer t)
    {
        try
        {
            const uint32_t device_index = 0;
            m_ros_writer->write_snapshot({ device_index, static_cast<uint32_t>(sensor_index) }, capture_time, ext, snapshot);
        }
        catch (const std::exception& e)
        {
            on_error(e.what());
        }
    });
}

template <rs2_extension E, typename P>
bool librealsense::record_device::extend_to_aux(std::shared_ptr<P> p, void** ext)
{
    using EXT_TYPE = typename ExtensionToType<E>::type;
    auto ptr = As<EXT_TYPE>(p);
    if (!ptr)
        return false;

    if (auto recordable = As<librealsense::recordable<EXT_TYPE>>(p))
    {
        recordable->enable_recording([this](const EXT_TYPE& ext) {
            write_device_extension_changes(ext);
        });
    }

    *ext = &ptr;
    return true;
}

bool librealsense::record_device::extend_to(rs2_extension extension_type, void** ext)
{
    /**************************************************************************************
     A record device wraps the live device, and should have the same functionalities.
     To do that, the record device implements the extendable_interface and when the user tries to
     treat the device as some extension, this function is called, and we return a pointer to the
     live device's extension. If that extension is a recordable one, we also enable_recording for it.
    **************************************************************************************/

    switch (extension_type)
    {
    case RS2_EXTENSION_INFO:    // [[fallthrough]]
    case RS2_EXTENSION_RECORD:
        *ext = this;
        return true;
    case RS2_EXTENSION_OPTIONS         : return extend_to_aux<RS2_EXTENSION_OPTIONS        >(m_device, ext);
    case RS2_EXTENSION_DEBUG           : return extend_to_aux<RS2_EXTENSION_DEBUG          >(m_device, ext);
    //TODO: Add once implements recordable: case RS2_EXTENSION_MOTION          : return extend_to_aux<RS2_EXTENSION_MOTION         >(m_device, ext);
    //case RS2_EXTENSION_VIDEO           : return extend_to_aux<RS2_EXTENSION_VIDEO          >(m_device, ext);
    //case RS2_EXTENSION_ROI             : return extend_to_aux<RS2_EXTENSION_ROI            >(m_device, ext);
    //case RS2_EXTENSION_DEPTH_SENSOR    : return extend_to_aux<RS2_EXTENSION_DEPTH_SENSOR   >(m_device, ext);
    //case RS2_EXTENSION_VIDEO_FRAME     : return extend_to_aux<RS2_EXTENSION_VIDEO_FRAME    >(m_device, ext);
    //TODO: Add once interface created: case RS2_EXTENSION_MOTION_FRAME    : return extend_to_aux<RS2_EXTENSION_MOTION_FRAME   >(m_device, ext);
    //case RS2_EXTENSION_COMPOSITE_FRAME : return extend_to_aux<RS2_EXTENSION_COMPOSITE_FRAME>(m_device, ext);
    //case RS2_EXTENSION_POINTS          : return extend_to_aux<RS2_EXTENSION_POINTS         >(m_device, ext);
    //case RS2_EXTENSION_DEPTH_FRAME     : return extend_to_aux<RS2_EXTENSION_DEPTH_FRAME    >(m_device, ext);
    //case RS2_EXTENSION_ADVANCED_MODE   : return extend_to_aux<RS2_EXTENSION_ADVANCED_MODE  >(m_device, ext);
    //case RS2_EXTENSION_VIDEO_PROFILE   : return extend_to_aux<RS2_EXTENSION_VIDEO_PROFILE  >(m_device, ext);
    //case RS2_EXTENSION_PLAYBACK        : return extend_to_aux<RS2_EXTENSION_PLAYBACK       >(m_device, ext);
    default:
        LOG_WARNING("Extensions type is unhandled: " << extension_type);
        return false;
    }
}

void librealsense::record_device::pause_recording()
{
    LOG_INFO("Record Pause called");

    (*m_write_thread)->invoke([this](dispatcher::cancellable_timer c)
    {
        LOG_DEBUG("Record pause invoked");

        if (m_is_recording == false)
            return;

        //unregister_callbacks();
        m_time_of_pause = std::chrono::high_resolution_clock::now();
        m_is_recording = false;
        LOG_DEBUG("Time of pause: " << m_time_of_pause.time_since_epoch().count());
    });
    (*m_write_thread)->flush();
    LOG_INFO("Record paused");
}
void librealsense::record_device::resume_recording()
{
    LOG_INFO("Record resume called");
    (*m_write_thread)->invoke([this](dispatcher::cancellable_timer c)
    {
        LOG_DEBUG("Record resume invoked");
        if (m_is_recording)
            return;

        m_record_pause_time += (std::chrono::high_resolution_clock::now() - m_time_of_pause);
        //register_callbacks();
        m_is_recording = true;
        LOG_DEBUG("Total pause time: " << m_record_pause_time.count());
        LOG_INFO("Record resumed");
    });
}
platform::backend_device_group record_device::get_device_data() const
{
    return m_device->get_device_data();
}
std::shared_ptr<matcher> record_device::create_matcher(const frame_holder& frame) const
{
    return m_device->create_matcher(frame);
}

void record_device::initialize_recording()
{
    //Expected to be called once when recording to file actually starts
    m_capture_time_base = std::chrono::high_resolution_clock::now();
    m_cached_data_size = 0;
}
void record_device::stop_gracefully(to_string error_msg)
{
    for (auto&& sensor : m_sensors)
    {
        sensor->stop();
        sensor->close();
    }
}

std::pair<uint32_t, rs2_extrinsics> record_device::get_extrinsics(const stream_interface& stream) const
{
    return m_device->get_extrinsics(stream);
}

bool record_device::is_valid() const
{
    return m_device->is_valid();
}
