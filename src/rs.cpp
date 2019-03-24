// License: Apache 2.0 See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <functional>   // For function

#include "api.h"
#include "context.h"
#include "device.h"
#include "algo.h"
#include "core/debug.h"
#include "core/motion.h"
#include "core/extension.h"
#include "media/record/record_device.h"
#include <media/ros/ros_writer.h>
#include <media/ros/ros_reader.h>
#include "core/advanced_mode.h"
#include "source.h"
#include "core/processing.h"
#include "proc/synthetic-stream.h"
#include "proc/processing-blocks-factory.h"
#include "proc/colorizer.h"
#include "proc/pointcloud.h"
#include "proc/threshold.h"
#include "proc/disparity-transform.h"
#include "proc/syncer-processing-block.h"
#include "proc/decimation-filter.h"
#include "proc/spatial-filter.h"
#include "proc/zero-order.h"
#include "proc/hole-filling-filter.h"
#include "proc/yuy2rgb.h"
#include "proc/rates-printer.h"
#include "media/playback/playback_device.h"
#include "stream.h"
#include "../include/librealsense2/h/rs_types.h"
#include "pipeline/pipeline.h"
#include "environment.h"
#include "proc/temporal-filter.h"
#include "software-device.h"

////////////////////////
// API implementation //
////////////////////////

using namespace librealsense;

struct rs2_stream_profile_list
{
    std::vector<std::shared_ptr<stream_profile_interface>> list;
};

struct rs2_processing_block_list
{
    processing_blocks list;
};

struct rs2_sensor : public rs2_options
{
    rs2_sensor(rs2_device parent,
        librealsense::sensor_interface* sensor,
        size_t index)
        : rs2_options((librealsense::options_interface*)sensor),
        parent(parent), sensor(sensor), index(index)
    {}

    rs2_device parent;
    librealsense::sensor_interface* sensor;
    size_t index;

    rs2_sensor& operator=(const rs2_sensor&) = delete;
    rs2_sensor(const rs2_sensor&) = delete;
};


struct rs2_context
{
    ~rs2_context() { ctx->stop(); }
    std::shared_ptr<librealsense::context> ctx;
};

struct rs2_device_hub
{
    std::shared_ptr<librealsense::device_hub> hub;
};

struct rs2_pipeline
{
    std::shared_ptr<librealsense::pipeline::pipeline> pipeline;
};

struct rs2_config
{
    std::shared_ptr<librealsense::pipeline::config> config;
};

struct rs2_pipeline_profile
{
    std::shared_ptr<librealsense::pipeline::profile> profile;
};

struct rs2_frame_queue
{
    explicit rs2_frame_queue(int cap)
        : queue(cap)
    {
    }

    single_consumer_frame_queue<librealsense::frame_holder> queue;
};

struct rs2_sensor_list
{
    rs2_device dev;
};

struct rs2_error
{
    std::string message;
    std::string function;
    std::string args;
    rs2_exception_type exception_type;
};

rs2_error * rs2_create_error(const char* what, const char* name, const char* args, rs2_exception_type type)
{
    return new rs2_error{ what, name, args, type };
}

void notifications_processor::raise_notification(const notification n)
{
    _dispatcher.invoke([this, n](dispatcher::cancellable_timer ct)
    {
        std::lock_guard<std::mutex> lock(_callback_mutex);
        rs2_notification noti(&n);
        if (_callback)_callback->on_notification(&noti);
    });
}

rs2_context* rs2_create_context(int api_version, rs2_error** error) BEGIN_API_CALL
{
    verify_version_compatibility(api_version);

    return new rs2_context{ std::make_shared<librealsense::context>(librealsense::backend_type::standard) };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, api_version)

void rs2_delete_context(rs2_context* context) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(context);
    delete context;
}
NOEXCEPT_RETURN(, context)

rs2_device_hub* rs2_create_device_hub(const rs2_context* context, rs2_error** error) BEGIN_API_CALL
{
    return new rs2_device_hub{ std::make_shared<librealsense::device_hub>(context->ctx) };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, context)

void rs2_delete_device_hub(const rs2_device_hub* hub) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(hub);
    delete hub;
}
NOEXCEPT_RETURN(, hub)

rs2_device* rs2_device_hub_wait_for_device(const rs2_device_hub* hub, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(hub);
    auto dev = hub->hub->wait_for_device();
    return new rs2_device{ hub->hub->get_context(), std::make_shared<readonly_device_info>(dev), dev };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, hub)

int rs2_device_hub_is_device_connected(const rs2_device_hub* hub, const rs2_device* device, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(hub);
    VALIDATE_NOT_NULL(device);
    auto res = hub->hub->is_connected(*device->device);
    return res ? 1 : 0;
}
HANDLE_EXCEPTIONS_AND_RETURN(0, hub, device)

rs2_device_list* rs2_query_devices(const rs2_context* context, rs2_error** error)
{
    return rs2_query_devices_ex(context, RS2_PRODUCT_LINE_ANY_INTEL, error);
}

rs2_device_list* rs2_query_devices_ex(const rs2_context* context, int product_mask, rs2_error** error)
{
    VALIDATE_NOT_NULL(context);

    std::vector<rs2_device_info> results;
    for (auto&& dev_info : context->ctx->query_devices(product_mask))
    {
        try
        {
            rs2_device_info d{ context->ctx, dev_info };
            results.push_back(d);
        }
        catch (...)
        {
            LOG_WARNING("Could not open device!");
        }
    }

    return new rs2_device_list{ context->ctx, results };
}

rs2_sensor_list* rs2_query_sensors(const rs2_device* device, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(device);

    std::vector<rs2_device_info> results;
    try
    {
        auto dev = device->device;
        for (unsigned int i = 0; i < dev->get_sensors_count(); i++)
        {
            rs2_device_info d{ device->ctx, device->info };
            results.push_back(d);
        }
    }
    catch (...)
    {
        LOG_WARNING("Could not open device!");
    }

    return new rs2_sensor_list{ *device };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, device)

int rs2_get_device_count(const rs2_device_list* list, rs2_error** error) BEGIN_API_CALL
{
    if (list == nullptr)
        return 0;
    return static_cast<int>(list->list.size());
}
HANDLE_EXCEPTIONS_AND_RETURN(0, list)

int rs2_get_sensors_count(const rs2_sensor_list* list, rs2_error** error) BEGIN_API_CALL
{
    if (list == nullptr)
        return 0;
    return static_cast<int>(list->dev.device->get_sensors_count());
}
HANDLE_EXCEPTIONS_AND_RETURN(0, list)

void rs2_delete_device_list(rs2_device_list* list) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(list);
    delete list;
}
NOEXCEPT_RETURN(, list)

void rs2_delete_sensor_list(rs2_sensor_list* list) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(list);
    delete list;
}
NOEXCEPT_RETURN(, list)

rs2_device* rs2_create_device(const rs2_device_list* info_list, int index, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(info_list);
    VALIDATE_RANGE(index, 0, (int)info_list->list.size() - 1);

    return new rs2_device{ info_list->ctx,
                          info_list->list[index].info,
                          info_list->list[index].info->create_device()
    };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, info_list, index)

void rs2_delete_device(rs2_device* device) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(device);
    delete device;
}
NOEXCEPT_RETURN(, device)

rs2_sensor* rs2_create_sensor(const rs2_sensor_list* list, int index, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(list);
    VALIDATE_RANGE(index, 0, (int)list->dev.device->get_sensors_count() - 1);

    return new rs2_sensor{
            list->dev,
            &list->dev.device->get_sensor(index),
            (size_t)index
    };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, list, index)

void rs2_delete_sensor(rs2_sensor* device) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(device);
    delete device;
}
NOEXCEPT_RETURN(, device)

rs2_stream_profile_list* rs2_get_stream_profiles(rs2_sensor* sensor, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(sensor);
    return new rs2_stream_profile_list{ sensor->sensor->get_stream_profiles() };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, sensor)

const rs2_stream_profile* rs2_get_stream_profile(const rs2_stream_profile_list* list, int index, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(list);
    VALIDATE_RANGE(index, 0, (int)list->list.size() - 1);

    return list->list[index]->get_c_wrapper();
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, list, index)

int rs2_get_stream_profiles_count(const rs2_stream_profile_list* list, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(list);
    return static_cast<int>(list->list.size());
}
HANDLE_EXCEPTIONS_AND_RETURN(0, list)

void rs2_delete_stream_profiles_list(rs2_stream_profile_list* list) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(list);
    delete list;
}
NOEXCEPT_RETURN(, list)

void rs2_get_video_stream_intrinsics(const rs2_stream_profile* from, rs2_intrinsics* intr, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(from);
    VALIDATE_NOT_NULL(intr);

    auto vid = VALIDATE_INTERFACE(from->profile, librealsense::video_stream_profile_interface);

    *intr = vid->get_intrinsics();
}
HANDLE_EXCEPTIONS_AND_RETURN(, from, intr)

void rs2_get_motion_intrinsics(const rs2_stream_profile* mode, rs2_motion_device_intrinsic * intrinsics, rs2_error ** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(mode);
    VALIDATE_NOT_NULL(intrinsics);

    auto motion = VALIDATE_INTERFACE(mode->profile, librealsense::motion_stream_profile_interface);
    *intrinsics = motion->get_intrinsics();
}
HANDLE_EXCEPTIONS_AND_RETURN(, mode, intrinsics)


void rs2_get_video_stream_resolution(const rs2_stream_profile* from, int* width, int* height, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(from);

    auto vid = VALIDATE_INTERFACE(from->profile, librealsense::video_stream_profile_interface);

    if (width)  *width = vid->get_width();
    if (height) *height = vid->get_height();
}
HANDLE_EXCEPTIONS_AND_RETURN(, from, width, height)

int rs2_is_stream_profile_default(const rs2_stream_profile* profile, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(profile);
    return profile->profile->get_tag() & profile_tag::PROFILE_TAG_DEFAULT ? 1 : 0;
}
HANDLE_EXCEPTIONS_AND_RETURN(0, profile)

void rs2_get_stream_profile_data(const rs2_stream_profile* mode, rs2_stream* stream, rs2_format* format, int* index, int* unique_id, int* framerate, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(mode);
    VALIDATE_NOT_NULL(stream);
    VALIDATE_NOT_NULL(format);
    VALIDATE_NOT_NULL(index);
    VALIDATE_NOT_NULL(unique_id);

    *framerate = mode->profile->get_framerate();
    *format = mode->profile->get_format();
    *index = mode->profile->get_stream_index();
    *stream = mode->profile->get_stream_type();
    *unique_id = mode->profile->get_unique_id();
}
HANDLE_EXCEPTIONS_AND_RETURN(, mode, stream, format, index, framerate)

void rs2_set_stream_profile_data(rs2_stream_profile* mode, rs2_stream stream, int index, rs2_format format, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(mode);
    VALIDATE_ENUM(stream);
    VALIDATE_ENUM(format);

    mode->profile->set_format(format);
    mode->profile->set_stream_type(stream);
    mode->profile->set_stream_index(index);
}
HANDLE_EXCEPTIONS_AND_RETURN(, mode, stream, format)

void rs2_delete_stream_profile(rs2_stream_profile* p) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(p);
    delete p;
}
NOEXCEPT_RETURN(, p)

rs2_stream_profile* rs2_clone_stream_profile(const rs2_stream_profile* mode, rs2_stream stream, int stream_idx, rs2_format fmt, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(mode);
    VALIDATE_ENUM(stream);
    VALIDATE_ENUM(fmt);

    auto sp = mode->profile->clone();
    sp->set_stream_type(stream);
    sp->set_stream_index(stream_idx);
    sp->set_format(fmt);

    return new rs2_stream_profile{ sp.get(), sp };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, mode, stream, stream_idx, fmt)

const rs2_raw_data_buffer* rs2_send_and_receive_raw_data(rs2_device* device, void* raw_data_to_send, unsigned size_of_raw_data_to_send, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(device);

    auto debug_interface = VALIDATE_INTERFACE(device->device, librealsense::debug_interface);

    auto raw_data_buffer = static_cast<uint8_t*>(raw_data_to_send);
    std::vector<uint8_t> buffer_to_send(raw_data_buffer, raw_data_buffer + size_of_raw_data_to_send);
    auto ret_data = debug_interface->send_receive_raw_data(buffer_to_send);
    return new rs2_raw_data_buffer{ ret_data };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, device)

const unsigned char* rs2_get_raw_data(const rs2_raw_data_buffer* buffer, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(buffer);
    return buffer->buffer.data();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, buffer)

int rs2_get_raw_data_size(const rs2_raw_data_buffer* buffer, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(buffer);
    return static_cast<int>(buffer->buffer.size());
}
HANDLE_EXCEPTIONS_AND_RETURN(0, buffer)

void rs2_delete_raw_data(const rs2_raw_data_buffer* buffer) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(buffer);
    delete buffer;
}
NOEXCEPT_RETURN(, buffer)

void rs2_open(rs2_sensor* sensor, const rs2_stream_profile* profile, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(sensor);
    VALIDATE_NOT_NULL(profile);

    std::vector<std::shared_ptr<stream_profile_interface>> request;
    request.push_back(std::dynamic_pointer_cast<stream_profile_interface>(profile->profile->shared_from_this()));
    sensor->sensor->open(request);
}
HANDLE_EXCEPTIONS_AND_RETURN(, sensor, profile)

void rs2_open_multiple(rs2_sensor* sensor,
    const rs2_stream_profile** profiles, int count, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(sensor);
    VALIDATE_NOT_NULL(profiles);

    std::vector<std::shared_ptr<stream_profile_interface>> request;
    for (auto i = 0; i < count; i++)
    {
        request.push_back(std::dynamic_pointer_cast<stream_profile_interface>(profiles[i]->profile->shared_from_this()));
    }
    sensor->sensor->open(request);
}
HANDLE_EXCEPTIONS_AND_RETURN(, sensor, profiles, count)

void rs2_close(const rs2_sensor* sensor, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(sensor);
    sensor->sensor->close();
}
HANDLE_EXCEPTIONS_AND_RETURN(, sensor)

int rs2_is_option_read_only(const rs2_options* options, rs2_option option, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(options);
    return options->options->get_option(option).is_read_only();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, options, option)

float rs2_get_option(const rs2_options* options, rs2_option option, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(options);
    VALIDATE_OPTION(options, option);
    return options->options->get_option(option).query();
}
HANDLE_EXCEPTIONS_AND_RETURN(0.0f, options, option)

void rs2_set_option(const rs2_options* options, rs2_option option, float value, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(options);
    VALIDATE_OPTION(options, option);
    options->options->get_option(option).set(value);
}
HANDLE_EXCEPTIONS_AND_RETURN(, options, option, value)

rs2_options_list* rs2_get_options_list(const rs2_options* options, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(options);
    return new rs2_options_list{ options->options->get_supported_options() };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, options)

const char* rs2_get_option_name(const rs2_options* options,rs2_option option, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(options);
    return options->options->get_option_name(option);
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, option, options)

int rs2_get_options_list_size(const rs2_options_list* options, rs2_error** error)BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(options);
    return int(options->list.size());
}
HANDLE_EXCEPTIONS_AND_RETURN(0, options)

rs2_option rs2_get_option_from_list(const rs2_options_list* options, int i, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(options);
    return options->list[i];
}
HANDLE_EXCEPTIONS_AND_RETURN(RS2_OPTION_COUNT, options)

void rs2_delete_options_list(rs2_options_list* list) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(list);
    delete list;
}
NOEXCEPT_RETURN(, list)

int rs2_supports_option(const rs2_options* options, rs2_option option, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(options);
    return options->options->supports_option(option);
}
HANDLE_EXCEPTIONS_AND_RETURN(0, options, option)

void rs2_get_option_range(const rs2_options* options, rs2_option option,
    float* min, float* max, float* step, float* def, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(options);
    VALIDATE_OPTION(options, option);
    VALIDATE_NOT_NULL(min);
    VALIDATE_NOT_NULL(max);
    VALIDATE_NOT_NULL(step);
    VALIDATE_NOT_NULL(def);
    auto range = options->options->get_option(option).get_range();
    *min = range.min;
    *max = range.max;
    *def = range.def;
    *step = range.step;
}
HANDLE_EXCEPTIONS_AND_RETURN(, options, option, min, max, step, def)

const char* rs2_get_device_info(const rs2_device* dev, rs2_camera_info info, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(dev);
    VALIDATE_ENUM(info);
    if (dev->device->supports_info(info))
    {
        return dev->device->get_info(info).c_str();
    }
    throw librealsense::invalid_value_exception(librealsense::to_string() << "info " << rs2_camera_info_to_string(info) << " not supported by the device!");
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, dev, info)

int rs2_supports_device_info(const rs2_device* dev, rs2_camera_info info, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(dev);
    VALIDATE_ENUM(info);
    return dev->device->supports_info(info);
}
HANDLE_EXCEPTIONS_AND_RETURN(false, dev, info)

const char* rs2_get_sensor_info(const rs2_sensor* sensor, rs2_camera_info info, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(sensor);
    VALIDATE_ENUM(info);
    if (sensor->sensor->supports_info(info))
    {
        return sensor->sensor->get_info(info).c_str();
    }
    throw librealsense::invalid_value_exception(librealsense::to_string() << "info " << rs2_camera_info_to_string(info) << " not supported by the sensor!");
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, sensor, info)

int rs2_supports_sensor_info(const rs2_sensor* sensor, rs2_camera_info info, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(sensor);
    VALIDATE_ENUM(info);
    return sensor->sensor->supports_info(info);
}
HANDLE_EXCEPTIONS_AND_RETURN(false, sensor, info)

void rs2_start(const rs2_sensor* sensor, rs2_frame_callback_ptr on_frame, void* user, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(sensor);
    VALIDATE_NOT_NULL(on_frame);
    librealsense::frame_callback_ptr callback(
        new librealsense::frame_callback(on_frame, user));
    sensor->sensor->start(move(callback));
}
HANDLE_EXCEPTIONS_AND_RETURN(, sensor, on_frame, user)

void rs2_start_queue(const rs2_sensor* sensor, rs2_frame_queue* queue, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(sensor);
    VALIDATE_NOT_NULL(queue);
    librealsense::frame_callback_ptr callback(
        new librealsense::frame_callback(rs2_enqueue_frame, queue));
    sensor->sensor->start(move(callback));
}
HANDLE_EXCEPTIONS_AND_RETURN(, sensor, queue)

void rs2_set_notifications_callback(const rs2_sensor* sensor, rs2_notification_callback_ptr on_notification, void* user, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(sensor);
    VALIDATE_NOT_NULL(on_notification);
    librealsense::notifications_callback_ptr callback(
        new librealsense::notifications_callback(on_notification, user),
        [](rs2_notifications_callback* p) { delete p; });
    sensor->sensor->register_notifications_callback(std::move(callback));
}
HANDLE_EXCEPTIONS_AND_RETURN(, sensor, on_notification, user)

void rs2_set_devices_changed_callback(const rs2_context* context, rs2_devices_changed_callback_ptr callback, void* user, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(context);
    VALIDATE_NOT_NULL(callback);
    librealsense::devices_changed_callback_ptr cb(
        new librealsense::devices_changed_callback(callback, user),
        [](rs2_devices_changed_callback* p) { delete p; });
    context->ctx->set_devices_changed_callback(std::move(cb));
}
HANDLE_EXCEPTIONS_AND_RETURN(, context, callback, user)

void rs2_start_cpp(const rs2_sensor* sensor, rs2_frame_callback* callback, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(sensor);
    VALIDATE_NOT_NULL(callback);
    sensor->sensor->start({ callback, [](rs2_frame_callback* p) { p->release(); } });
}
HANDLE_EXCEPTIONS_AND_RETURN(, sensor, callback)

void rs2_set_notifications_callback_cpp(const rs2_sensor* sensor, rs2_notifications_callback* callback, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(sensor);
    VALIDATE_NOT_NULL(callback);
    sensor->sensor->register_notifications_callback({ callback, [](rs2_notifications_callback* p) { p->release(); } });
}
HANDLE_EXCEPTIONS_AND_RETURN(, sensor, callback)

void rs2_set_devices_changed_callback_cpp(rs2_context* context, rs2_devices_changed_callback* callback, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(context);
    VALIDATE_NOT_NULL(callback);
    context->ctx->set_devices_changed_callback({ callback, [](rs2_devices_changed_callback* p) { p->release(); } });
}
HANDLE_EXCEPTIONS_AND_RETURN(, context, callback)

void rs2_stop(const rs2_sensor* sensor, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(sensor);
    sensor->sensor->stop();
}
HANDLE_EXCEPTIONS_AND_RETURN(, sensor)

int rs2_supports_frame_metadata(const rs2_frame* frame, rs2_frame_metadata_value frame_metadata, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(frame);
    VALIDATE_ENUM(frame_metadata);
    return ((frame_interface*)frame)->supports_frame_metadata(frame_metadata);
}
HANDLE_EXCEPTIONS_AND_RETURN(0, frame, frame_metadata)

rs2_metadata_type rs2_get_frame_metadata(const rs2_frame* frame, rs2_frame_metadata_value frame_metadata, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(frame);
    VALIDATE_ENUM(frame_metadata);
    return ((frame_interface*)frame)->get_frame_metadata(frame_metadata);
}
HANDLE_EXCEPTIONS_AND_RETURN(0, frame, frame_metadata)

const char* rs2_get_notification_description(rs2_notification* notification, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(notification);
    return notification->_notification->description.c_str();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, notification)

rs2_time_t rs2_get_notification_timestamp(rs2_notification* notification, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(notification);
    return notification->_notification->timestamp;
}
HANDLE_EXCEPTIONS_AND_RETURN(0, notification)

rs2_log_severity rs2_get_notification_severity(rs2_notification* notification, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(notification);
    return (rs2_log_severity)notification->_notification->severity;
}
HANDLE_EXCEPTIONS_AND_RETURN(RS2_LOG_SEVERITY_NONE, notification)

rs2_notification_category rs2_get_notification_category(rs2_notification* notification, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(notification);
    return (rs2_notification_category)notification->_notification->category;
}
HANDLE_EXCEPTIONS_AND_RETURN(RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR, notification)

const char* rs2_get_notification_serialized_data(rs2_notification* notification, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(notification);
    return notification->_notification->serialized_data.c_str();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, notification)


int rs2_device_list_contains(const rs2_device_list* info_list, const rs2_device* device, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(info_list);
    VALIDATE_NOT_NULL(device);

    for (auto info : info_list->list)
    {
        // TODO: This is incapable of detecting playback devices
        // Need to extend, if playback, compare filename or something
        if (device->info && device->info->get_device_data() == info.info->get_device_data())
        {
            return 1;
        }
    }
    return 0;
}
HANDLE_EXCEPTIONS_AND_RETURN(false, info_list, device)

rs2_time_t rs2_get_frame_timestamp(const rs2_frame* frame_ref, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(frame_ref);
    return ((frame_interface*)frame_ref)->get_frame_timestamp();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, frame_ref)

rs2_timestamp_domain rs2_get_frame_timestamp_domain(const rs2_frame* frame_ref, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(frame_ref);
    return ((frame_interface*)frame_ref)->get_frame_timestamp_domain();
}
HANDLE_EXCEPTIONS_AND_RETURN(RS2_TIMESTAMP_DOMAIN_COUNT, frame_ref)

const void* rs2_get_frame_data(const rs2_frame* frame_ref, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(frame_ref);
    return ((frame_interface*)frame_ref)->get_frame_data();
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, frame_ref)

int rs2_get_frame_width(const rs2_frame* frame_ref, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(frame_ref);
    auto vf = VALIDATE_INTERFACE(((frame_interface*)frame_ref), librealsense::video_frame);
    return vf->get_width();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, frame_ref)

int rs2_get_frame_height(const rs2_frame* frame_ref, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(frame_ref);
    auto vf = VALIDATE_INTERFACE(((frame_interface*)frame_ref), librealsense::video_frame);
    return vf->get_height();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, frame_ref)

int rs2_get_frame_stride_in_bytes(const rs2_frame* frame_ref, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(frame_ref);
    auto vf = VALIDATE_INTERFACE(((frame_interface*)frame_ref), librealsense::video_frame);
    return vf->get_stride();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, frame_ref)

const rs2_stream_profile* rs2_get_frame_stream_profile(const rs2_frame* frame_ref, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(frame_ref);
    return ((frame_interface*)frame_ref)->get_stream()->get_c_wrapper();
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, frame_ref)

int rs2_get_frame_bits_per_pixel(const rs2_frame* frame_ref, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(frame_ref);
    auto vf = VALIDATE_INTERFACE(((frame_interface*)frame_ref), librealsense::video_frame);
    return vf->get_bpp();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, frame_ref)

unsigned long long rs2_get_frame_number(const rs2_frame* frame, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(frame);
    return ((frame_interface*)frame)->get_frame_number();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, frame)

void rs2_release_frame(rs2_frame* frame) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(frame);
    ((frame_interface*)frame)->release();
}
NOEXCEPT_RETURN(, frame)

void rs2_keep_frame(rs2_frame* frame) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(frame);
    ((frame_interface*)frame)->keep();
}
NOEXCEPT_RETURN(, frame)

const char* rs2_get_option_description(const rs2_options* options, rs2_option option, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(options);
    VALIDATE_OPTION(options, option);
    return options->options->get_option(option).get_description();
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, options, option)

void rs2_frame_add_ref(rs2_frame* frame, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(frame);
    ((frame_interface*)frame)->acquire();
}
HANDLE_EXCEPTIONS_AND_RETURN(, frame)

const char* rs2_get_option_value_description(const rs2_options* options, rs2_option option, float value, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(options);
    VALIDATE_OPTION(options, option);
    return options->options->get_option(option).get_value_description(value);
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, options, option, value)

rs2_frame_queue* rs2_create_frame_queue(int capacity, rs2_error** error) BEGIN_API_CALL
{
    return new rs2_frame_queue(capacity);
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, capacity)

void rs2_delete_frame_queue(rs2_frame_queue* queue) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(queue);
    delete queue;
}
NOEXCEPT_RETURN(, queue)

rs2_frame* rs2_wait_for_frame(rs2_frame_queue* queue, unsigned int timeout_ms, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(queue);
    librealsense::frame_holder fh;
    if (!queue->queue.dequeue(&fh, timeout_ms))
    {
        throw std::runtime_error("Frame did not arrive in time!");
    }

    frame_interface* result = nullptr;
    std::swap(result, fh.frame);
    return (rs2_frame*)result;
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, queue)

int rs2_poll_for_frame(rs2_frame_queue* queue, rs2_frame** output_frame, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(queue);
    VALIDATE_NOT_NULL(output_frame);
    librealsense::frame_holder fh;
    if (queue->queue.try_dequeue(&fh))
    {
        frame_interface* result = nullptr;
        std::swap(result, fh.frame);
        *output_frame = (rs2_frame*)result;
        return true;
    }

    return false;
}
HANDLE_EXCEPTIONS_AND_RETURN(0, queue, output_frame)

int rs2_try_wait_for_frame(rs2_frame_queue* queue, unsigned int timeout_ms, rs2_frame** output_frame, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(queue);
    VALIDATE_NOT_NULL(output_frame);
    librealsense::frame_holder fh;
    if (!queue->queue.dequeue(&fh, timeout_ms))
    {
        return false;
    }

    frame_interface* result = nullptr;
    std::swap(result, fh.frame);
    *output_frame = (rs2_frame*)result;
    return true;
}
HANDLE_EXCEPTIONS_AND_RETURN(0, queue, output_frame)

void rs2_enqueue_frame(rs2_frame* frame, void* queue) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(frame);
    VALIDATE_NOT_NULL(queue);
    auto q = reinterpret_cast<rs2_frame_queue*>(queue);
    librealsense::frame_holder fh;
    fh.frame = (frame_interface*)frame;
    q->queue.enqueue(std::move(fh));
}
NOEXCEPT_RETURN(, frame, queue)

void rs2_flush_queue(rs2_frame_queue* queue, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(queue);
    queue->queue.clear();
}
HANDLE_EXCEPTIONS_AND_RETURN(, queue)

void rs2_get_extrinsics(const rs2_stream_profile* from,
    const rs2_stream_profile* to,
    rs2_extrinsics* extrin, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(from);
    VALIDATE_NOT_NULL(to);
    VALIDATE_NOT_NULL(extrin);

    if (!environment::get_instance().get_extrinsics_graph().try_fetch_extrinsics(*from->profile, *to->profile, extrin))
    {
        throw not_implemented_exception("Requested extrinsics are not available!");
    }
}
HANDLE_EXCEPTIONS_AND_RETURN(, from, to, extrin)

void rs2_register_extrinsics(const rs2_stream_profile* from,
    const rs2_stream_profile* to,
    rs2_extrinsics extrin, rs2_error** error)BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(from);
    VALIDATE_NOT_NULL(to);

    environment::get_instance().get_extrinsics_graph().register_extrinsics(*from->profile, *to->profile, extrin);
}
HANDLE_EXCEPTIONS_AND_RETURN(, from, to)

void rs2_hardware_reset(const rs2_device* device, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(device);
    device->device->hardware_reset();
}
HANDLE_EXCEPTIONS_AND_RETURN(, device)

// Verify  and provide API version encoded as integer value
int rs2_get_api_version(rs2_error** error) BEGIN_API_CALL
{
    // Each component type is within [0-99] range
    VALIDATE_RANGE(RS2_API_MAJOR_VERSION, 0, 99);
    VALIDATE_RANGE(RS2_API_MINOR_VERSION, 0, 99);
    VALIDATE_RANGE(RS2_API_PATCH_VERSION, 0, 99);
    return RS2_API_VERSION;
}
HANDLE_EXCEPTIONS_AND_RETURN(0, RS2_API_MAJOR_VERSION, RS2_API_MINOR_VERSION, RS2_API_PATCH_VERSION)

rs2_context* rs2_create_recording_context(int api_version, const char* filename, const char* section, rs2_recording_mode mode, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(filename);
    VALIDATE_NOT_NULL(section);
    verify_version_compatibility(api_version);

    return new rs2_context{ std::make_shared<librealsense::context>(librealsense::backend_type::record, filename, section, mode) };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, api_version, filename, section, mode)

rs2_context* rs2_create_mock_context_versioned(int api_version, const char* filename, const char* section, const char* min_api_version, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(filename);
    VALIDATE_NOT_NULL(section);
    verify_version_compatibility(api_version);

    return new rs2_context{ std::make_shared<librealsense::context>(librealsense::backend_type::playback, filename, section, RS2_RECORDING_MODE_COUNT, std::string(min_api_version)) };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, api_version, filename, section)

rs2_context* rs2_create_mock_context(int api_version, const char* filename, const char* section, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(filename);
    VALIDATE_NOT_NULL(section);
    verify_version_compatibility(api_version);

    return new rs2_context{ std::make_shared<librealsense::context>(librealsense::backend_type::playback, filename, section, RS2_RECORDING_MODE_COUNT) };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, api_version, filename, section)

void rs2_set_region_of_interest(const rs2_sensor* sensor, int min_x, int min_y, int max_x, int max_y, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(sensor);

    VALIDATE_LE(min_x, max_x);
    VALIDATE_LE(min_y, max_y);
    VALIDATE_LE(0, min_x);
    VALIDATE_LE(0, min_y);

    auto roi = VALIDATE_INTERFACE(sensor->sensor, librealsense::roi_sensor_interface);
    roi->get_roi_method().set({ min_x, min_y, max_x, max_y });
}
HANDLE_EXCEPTIONS_AND_RETURN(, sensor, min_x, min_y, max_x, max_y)

void rs2_get_region_of_interest(const rs2_sensor* sensor, int* min_x, int* min_y, int* max_x, int* max_y, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(sensor);
    VALIDATE_NOT_NULL(min_x);
    VALIDATE_NOT_NULL(min_y);
    VALIDATE_NOT_NULL(max_x);
    VALIDATE_NOT_NULL(max_y);

    auto roi = VALIDATE_INTERFACE(sensor->sensor, librealsense::roi_sensor_interface);
    auto rect = roi->get_roi_method().get();

    *min_x = rect.min_x;
    *min_y = rect.min_y;
    *max_x = rect.max_x;
    *max_y = rect.max_y;
}
HANDLE_EXCEPTIONS_AND_RETURN(, sensor, min_x, min_y, max_x, max_y)

void rs2_free_error(rs2_error* error) { if (error) delete error; }
const char* rs2_get_failed_function(const rs2_error* error) { return error ? error->function.c_str() : nullptr; }
const char* rs2_get_failed_args(const rs2_error* error) { return error ? error->args.c_str() : nullptr; }
const char* rs2_get_error_message(const rs2_error* error) { return error ? error->message.c_str() : nullptr; }
rs2_exception_type rs2_get_librealsense_exception_type(const rs2_error* error) { return error ? error->exception_type : RS2_EXCEPTION_TYPE_UNKNOWN; }

const char* rs2_stream_to_string(rs2_stream stream)                                       { return librealsense::get_string(stream);       }
const char* rs2_format_to_string(rs2_format format)                                       { return librealsense::get_string(format);       }
const char* rs2_distortion_to_string(rs2_distortion distortion)                           { return librealsense::get_string(distortion);   }
const char* rs2_option_to_string(rs2_option option)                                       { return librealsense::get_string(option);       }
const char* rs2_camera_info_to_string(rs2_camera_info info)                               { return librealsense::get_string(info);         }
const char* rs2_timestamp_domain_to_string(rs2_timestamp_domain info)                     { return librealsense::get_string(info);         }
const char* rs2_notification_category_to_string(rs2_notification_category category)       { return librealsense::get_string(category);     }
const char* rs2_sr300_visual_preset_to_string(rs2_sr300_visual_preset preset)             { return librealsense::get_string(preset);       }
const char* rs2_log_severity_to_string(rs2_log_severity severity)                         { return librealsense::get_string(severity);     }
const char* rs2_exception_type_to_string(rs2_exception_type type)                         { return librealsense::get_string(type);         }
const char* rs2_playback_status_to_string(rs2_playback_status status)                     { return librealsense::get_string(status);       }
const char* rs2_extension_type_to_string(rs2_extension type)                              { return librealsense::get_string(type);         }
const char* rs2_frame_metadata_to_string(rs2_frame_metadata_value metadata)               { return librealsense::get_string(metadata);     }
const char* rs2_extension_to_string(rs2_extension type)                                   { return rs2_extension_type_to_string(type);     }
const char* rs2_frame_metadata_value_to_string(rs2_frame_metadata_value metadata)         { return rs2_frame_metadata_to_string(metadata); }


void rs2_log_to_console(rs2_log_severity min_severity, rs2_error** error) BEGIN_API_CALL
{
    librealsense::log_to_console(min_severity);
}
HANDLE_EXCEPTIONS_AND_RETURN(, min_severity)

void rs2_log_to_file(rs2_log_severity min_severity, const char* file_path, rs2_error** error) BEGIN_API_CALL
{
    librealsense::log_to_file(min_severity, file_path);
}
HANDLE_EXCEPTIONS_AND_RETURN(, min_severity, file_path)

int rs2_is_sensor_extendable_to(const rs2_sensor* sensor, rs2_extension extension_type, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(sensor);
    VALIDATE_ENUM(extension_type);
    switch (extension_type)
    {
    case RS2_EXTENSION_DEBUG               : return VALIDATE_INTERFACE_NO_THROW(sensor->sensor, librealsense::debug_interface)        != nullptr;
    case RS2_EXTENSION_INFO                : return VALIDATE_INTERFACE_NO_THROW(sensor->sensor, librealsense::info_interface)         != nullptr;
    case RS2_EXTENSION_OPTIONS             : return VALIDATE_INTERFACE_NO_THROW(sensor->sensor, librealsense::options_interface)      != nullptr;
    case RS2_EXTENSION_VIDEO               : return VALIDATE_INTERFACE_NO_THROW(sensor->sensor, librealsense::video_sensor_interface) != nullptr;
    case RS2_EXTENSION_ROI                 : return VALIDATE_INTERFACE_NO_THROW(sensor->sensor, librealsense::roi_sensor_interface)   != nullptr;
    case RS2_EXTENSION_DEPTH_SENSOR        : return VALIDATE_INTERFACE_NO_THROW(sensor->sensor, librealsense::depth_sensor)           != nullptr;
    case RS2_EXTENSION_DEPTH_STEREO_SENSOR : return VALIDATE_INTERFACE_NO_THROW(sensor->sensor, librealsense::depth_stereo_sensor)    != nullptr;
    case RS2_EXTENSION_SOFTWARE_SENSOR     : return VALIDATE_INTERFACE_NO_THROW(sensor->sensor, librealsense::software_sensor)        != nullptr;
    case RS2_EXTENSION_POSE_SENSOR         : return VALIDATE_INTERFACE_NO_THROW(sensor->sensor, librealsense::pose_sensor_interface)  != nullptr;
    case RS2_EXTENSION_WHEEL_ODOMETER      : return VALIDATE_INTERFACE_NO_THROW(sensor->sensor, librealsense::wheel_odometry_interface)!= nullptr;

    default:
        return false;
    }
}
HANDLE_EXCEPTIONS_AND_RETURN(0, sensor, extension_type)

int rs2_is_device_extendable_to(const rs2_device* dev, rs2_extension extension, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(dev);
    VALIDATE_ENUM(extension);
    switch (extension)
    {
        case RS2_EXTENSION_DEBUG                 : return VALIDATE_INTERFACE_NO_THROW(dev->device, librealsense::debug_interface)             != nullptr;
        case RS2_EXTENSION_INFO                  : return VALIDATE_INTERFACE_NO_THROW(dev->device, librealsense::info_interface)              != nullptr;
        case RS2_EXTENSION_OPTIONS               : return VALIDATE_INTERFACE_NO_THROW(dev->device, librealsense::options_interface)           != nullptr;
        case RS2_EXTENSION_VIDEO                 : return VALIDATE_INTERFACE_NO_THROW(dev->device, librealsense::video_sensor_interface)      != nullptr;
        case RS2_EXTENSION_ROI                   : return VALIDATE_INTERFACE_NO_THROW(dev->device, librealsense::roi_sensor_interface)        != nullptr;
        case RS2_EXTENSION_DEPTH_SENSOR          : return VALIDATE_INTERFACE_NO_THROW(dev->device, librealsense::depth_sensor)                != nullptr;
        case RS2_EXTENSION_DEPTH_STEREO_SENSOR   : return VALIDATE_INTERFACE_NO_THROW(dev->device, librealsense::depth_stereo_sensor)         != nullptr;
        case RS2_EXTENSION_ADVANCED_MODE         : return VALIDATE_INTERFACE_NO_THROW(dev->device, librealsense::ds5_advanced_mode_interface) != nullptr;
        case RS2_EXTENSION_RECORD                : return VALIDATE_INTERFACE_NO_THROW(dev->device, librealsense::record_device)               != nullptr;
        case RS2_EXTENSION_PLAYBACK              : return VALIDATE_INTERFACE_NO_THROW(dev->device, librealsense::playback_device)             != nullptr;
        case RS2_EXTENSION_TM2                   : return VALIDATE_INTERFACE_NO_THROW(dev->device, librealsense::tm2_extensions)              != nullptr;
        default:
            return false;
    }
}
HANDLE_EXCEPTIONS_AND_RETURN(0, dev, extension)


int rs2_is_frame_extendable_to(const rs2_frame* f, rs2_extension extension_type, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(f);
    VALIDATE_ENUM(extension_type);
    switch (extension_type)
    {
    case RS2_EXTENSION_VIDEO_FRAME     : return VALIDATE_INTERFACE_NO_THROW((frame_interface*)f, librealsense::video_frame)     != nullptr;
    case RS2_EXTENSION_COMPOSITE_FRAME : return VALIDATE_INTERFACE_NO_THROW((frame_interface*)f, librealsense::composite_frame) != nullptr;
    case RS2_EXTENSION_POINTS          : return VALIDATE_INTERFACE_NO_THROW((frame_interface*)f, librealsense::points)          != nullptr;
    case RS2_EXTENSION_DEPTH_FRAME     : return VALIDATE_INTERFACE_NO_THROW((frame_interface*)f, librealsense::depth_frame)     != nullptr;
    case RS2_EXTENSION_DISPARITY_FRAME : return VALIDATE_INTERFACE_NO_THROW((frame_interface*)f, librealsense::disparity_frame) != nullptr;
    case RS2_EXTENSION_MOTION_FRAME    : return VALIDATE_INTERFACE_NO_THROW((frame_interface*)f, librealsense::motion_frame)    != nullptr;
    case RS2_EXTENSION_POSE_FRAME      : return VALIDATE_INTERFACE_NO_THROW((frame_interface*)f, librealsense::pose_frame)      != nullptr;

    default:
        return false;
    }
}
HANDLE_EXCEPTIONS_AND_RETURN(0, f, extension_type)

int rs2_is_processing_block_extendable_to(const rs2_processing_block* f, rs2_extension extension_type, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(f);
    VALIDATE_ENUM(extension_type);
    switch (extension_type)
    {
    case RS2_EXTENSION_DECIMATION_FILTER: return VALIDATE_INTERFACE_NO_THROW((processing_block_interface*)(f->block.get()), librealsense::decimation_filter) != nullptr;
    case RS2_EXTENSION_THRESHOLD_FILTER: return VALIDATE_INTERFACE_NO_THROW((processing_block_interface*)(f->block.get()), librealsense::threshold) != nullptr;
    case RS2_EXTENSION_DISPARITY_FILTER: return VALIDATE_INTERFACE_NO_THROW((processing_block_interface*)(f->block.get()), librealsense::disparity_transform) != nullptr;
    case RS2_EXTENSION_SPATIAL_FILTER: return VALIDATE_INTERFACE_NO_THROW((processing_block_interface*)(f->block.get()), librealsense::spatial_filter) != nullptr;
    case RS2_EXTENSION_TEMPORAL_FILTER: return VALIDATE_INTERFACE_NO_THROW((processing_block_interface*)(f->block.get()), librealsense::temporal_filter) != nullptr;
    case RS2_EXTENSION_HOLE_FILLING_FILTER: return VALIDATE_INTERFACE_NO_THROW((processing_block_interface*)(f->block.get()), librealsense::hole_filling_filter) != nullptr;
    case RS2_EXTENSION_ZERO_ORDER_FILTER: return VALIDATE_INTERFACE_NO_THROW((processing_block_interface*)(f->block.get()), librealsense::zero_order) != nullptr;
  
    default:
        return false;
    }
}
HANDLE_EXCEPTIONS_AND_RETURN(0, f, extension_type)

int rs2_stream_profile_is(const rs2_stream_profile* f, rs2_extension extension_type, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(f);
    VALIDATE_ENUM(extension_type);
    switch (extension_type)
    {
    case RS2_EXTENSION_VIDEO_PROFILE    : return VALIDATE_INTERFACE_NO_THROW(f->profile, librealsense::video_stream_profile_interface)  != nullptr;
    case RS2_EXTENSION_MOTION_PROFILE   : return VALIDATE_INTERFACE_NO_THROW(f->profile, librealsense::motion_stream_profile_interface) != nullptr;
    case RS2_EXTENSION_POSE_PROFILE     : return VALIDATE_INTERFACE_NO_THROW(f->profile, librealsense::pose_stream_profile_interface)   != nullptr;
    default:
        return false;
    }
}
HANDLE_EXCEPTIONS_AND_RETURN(0, f, extension_type)

rs2_device* rs2_context_add_device(rs2_context* ctx, const char* file, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(ctx);
    VALIDATE_NOT_NULL(file);

    return new rs2_device{ ctx->ctx, nullptr, ctx->ctx->add_device(file) };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, ctx, file)

void rs2_context_add_software_device(rs2_context* ctx, rs2_device* dev, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(ctx);
    VALIDATE_NOT_NULL(dev);
    auto software_dev = VALIDATE_INTERFACE(dev->device, librealsense::software_device);
    
    ctx->ctx->add_software_device(software_dev->get_info());
}
HANDLE_EXCEPTIONS_AND_RETURN(, ctx, dev)

void rs2_context_remove_device(rs2_context* ctx, const char* file, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(ctx);
    VALIDATE_NOT_NULL(file);
    ctx->ctx->remove_device(file);
}
HANDLE_EXCEPTIONS_AND_RETURN(, ctx, file)

void rs2_context_unload_tracking_module(rs2_context* ctx, rs2_error** error) BEGIN_API_CALL
{
#if WITH_TRACKING
    VALIDATE_NOT_NULL(ctx);
    ctx->ctx->unload_tracking_module();
#endif
}
HANDLE_EXCEPTIONS_AND_RETURN(, ctx)

const char* rs2_playback_device_get_file_path(const rs2_device* device, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(device);
    auto playback = VALIDATE_INTERFACE(device->device, librealsense::playback_device);
    return playback->get_file_name().c_str();
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, device)

unsigned long long int rs2_playback_get_duration(const rs2_device* device, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(device);
    auto playback = VALIDATE_INTERFACE(device->device, librealsense::playback_device);
    return playback->get_duration();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, device)

void rs2_playback_seek(const rs2_device* device, long long int time, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_LE(0, time);
    auto playback = VALIDATE_INTERFACE(device->device, librealsense::playback_device);
    playback->seek_to_time(std::chrono::nanoseconds(time));
}
HANDLE_EXCEPTIONS_AND_RETURN(, device)

unsigned long long int rs2_playback_get_position(const rs2_device* device, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(device);
    auto playback = VALIDATE_INTERFACE(device->device, librealsense::playback_device);
    return playback->get_position();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, device)

void rs2_playback_device_resume(const rs2_device* device, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(device);
    auto playback = VALIDATE_INTERFACE(device->device, librealsense::playback_device);
    playback->resume();
}
HANDLE_EXCEPTIONS_AND_RETURN(, device)

void rs2_playback_device_pause(const rs2_device* device, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(device);
    auto playback = VALIDATE_INTERFACE(device->device, librealsense::playback_device);
    return playback->pause();
}
HANDLE_EXCEPTIONS_AND_RETURN(, device)

void rs2_playback_device_set_real_time(const rs2_device* device, int real_time, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(device);
    auto playback = VALIDATE_INTERFACE(device->device, librealsense::playback_device);
    playback->set_real_time(real_time == 0 ? false : true);
}
HANDLE_EXCEPTIONS_AND_RETURN(, device)

int rs2_playback_device_is_real_time(const rs2_device* device, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(device);
    auto playback = VALIDATE_INTERFACE(device->device, librealsense::playback_device);
    return playback->is_real_time() ? 1 : 0;
}
HANDLE_EXCEPTIONS_AND_RETURN(0, device)

void rs2_playback_device_set_status_changed_callback(const rs2_device* device, rs2_playback_status_changed_callback* callback, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_NOT_NULL(callback);
    auto playback = VALIDATE_INTERFACE(device->device, librealsense::playback_device);
    auto cb = std::shared_ptr<rs2_playback_status_changed_callback>(callback, [](rs2_playback_status_changed_callback* p) { if (p) p->release(); });
    playback->playback_status_changed += [cb](rs2_playback_status status) { cb->on_playback_status_changed(status); };
}
HANDLE_EXCEPTIONS_AND_RETURN(, device, callback)


rs2_playback_status rs2_playback_device_get_current_status(const rs2_device* device, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(device);
    auto playback = VALIDATE_INTERFACE(device->device, librealsense::playback_device);
    return playback->get_current_status();
}
HANDLE_EXCEPTIONS_AND_RETURN(RS2_PLAYBACK_STATUS_UNKNOWN, device)

void rs2_playback_device_set_playback_speed(const rs2_device* device, float speed, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(device);
    auto playback = VALIDATE_INTERFACE(device->device, librealsense::playback_device);
    playback->set_frame_rate(speed);
}
HANDLE_EXCEPTIONS_AND_RETURN(, device)

void rs2_playback_device_stop(const rs2_device* device, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(device);
    auto playback = VALIDATE_INTERFACE(device->device, librealsense::playback_device);
    return playback->stop();
}
HANDLE_EXCEPTIONS_AND_RETURN(, device)

rs2_device* rs2_create_record_device(const rs2_device* device, const char* file, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_NOT_NULL(device->device);
    VALIDATE_NOT_NULL(file);

    return rs2_create_record_device_ex(device, file, device->device->compress_while_record(), error);
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, device, device->device, file)

rs2_device* rs2_create_record_device_ex(const rs2_device* device, const char* file, int compression_enabled, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_NOT_NULL(file);

    return new rs2_device({
        device->ctx,
        device->info,
        std::make_shared<record_device>(device->device, std::make_shared<ros_writer>(file, compression_enabled))
        });
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, device, file)

void rs2_record_device_pause(const rs2_device* device, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(device);
    auto record_device = VALIDATE_INTERFACE(device->device, librealsense::record_device);
    record_device->pause_recording();
}
HANDLE_EXCEPTIONS_AND_RETURN(, device)

void rs2_record_device_resume(const rs2_device* device, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(device);
    auto record_device = VALIDATE_INTERFACE(device->device, librealsense::record_device);
    record_device->resume_recording();
}
HANDLE_EXCEPTIONS_AND_RETURN(, device)

const char* rs2_record_device_filename(const rs2_device* device, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(device);
    auto record_device = VALIDATE_INTERFACE(device->device, librealsense::record_device);
    return record_device->get_filename().c_str();
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, device)


rs2_frame* rs2_allocate_synthetic_video_frame(rs2_source* source, const rs2_stream_profile* new_stream, rs2_frame* original,
    int new_bpp, int new_width, int new_height, int new_stride, rs2_extension frame_type, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(source);
    VALIDATE_NOT_NULL(original);
    VALIDATE_NOT_NULL(new_stream);

    auto recovered_profile = std::dynamic_pointer_cast<stream_profile_interface>(new_stream->profile->shared_from_this());

    return (rs2_frame*)source->source->allocate_video_frame(recovered_profile,
        (frame_interface*)original, new_bpp, new_width, new_height, new_stride, frame_type);
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, source, new_stream, original, new_bpp, new_width, new_height, new_stride, frame_type)

rs2_frame* rs2_allocate_points(rs2_source* source, const rs2_stream_profile* new_stream, rs2_frame* original, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(source);
    VALIDATE_NOT_NULL(original);
    VALIDATE_NOT_NULL(new_stream);

    auto recovered_profile = std::dynamic_pointer_cast<stream_profile_interface>(new_stream->profile->shared_from_this());

    return (rs2_frame*)source->source->allocate_points(recovered_profile,
        (frame_interface*)original);
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, source, new_stream, original)

void rs2_synthetic_frame_ready(rs2_source* source, rs2_frame* frame, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(frame);

    librealsense::frame_holder holder((frame_interface*)frame);
    VALIDATE_NOT_NULL(source);

    source->source->frame_ready(std::move(holder));
}
HANDLE_EXCEPTIONS_AND_RETURN(, source, frame)

rs2_pipeline* rs2_create_pipeline(rs2_context* ctx, rs2_error ** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(ctx);

    auto pipe = std::make_shared<pipeline::pipeline>(ctx->ctx);

    return new rs2_pipeline{ pipe };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, ctx)

void rs2_pipeline_stop(rs2_pipeline* pipe, rs2_error ** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(pipe);

    pipe->pipeline->stop();
}
HANDLE_EXCEPTIONS_AND_RETURN(, pipe)

rs2_frame* rs2_pipeline_wait_for_frames(rs2_pipeline* pipe, unsigned int timeout_ms, rs2_error ** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(pipe);

    auto f = pipe->pipeline->wait_for_frames(timeout_ms);
    auto frame = f.frame;
    f.frame = nullptr;
    return (rs2_frame*)(frame);
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, pipe)

int rs2_pipeline_poll_for_frames(rs2_pipeline * pipe, rs2_frame** output_frame, rs2_error ** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(pipe);
    VALIDATE_NOT_NULL(output_frame);

    librealsense::frame_holder fh;
    if (pipe->pipeline->poll_for_frames(&fh))
    {
        frame_interface* result = nullptr;
        std::swap(result, fh.frame);
        *output_frame = (rs2_frame*)result;
        return true;
    }
    return false;
}
HANDLE_EXCEPTIONS_AND_RETURN(0, pipe, output_frame)

int rs2_pipeline_try_wait_for_frames(rs2_pipeline* pipe, rs2_frame** output_frame, unsigned int timeout_ms, rs2_error ** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(pipe);
    VALIDATE_NOT_NULL(output_frame);

    librealsense::frame_holder fh;
    if (pipe->pipeline->try_wait_for_frames(&fh, timeout_ms))
    {
        frame_interface* result = nullptr;
        std::swap(result, fh.frame);
        *output_frame = (rs2_frame*)result;
        return true;
    }
    return false;
}
HANDLE_EXCEPTIONS_AND_RETURN(0, pipe, output_frame)

void rs2_delete_pipeline(rs2_pipeline* pipe) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(pipe);

    delete pipe;
}
NOEXCEPT_RETURN(, pipe)

rs2_pipeline_profile* rs2_pipeline_start(rs2_pipeline* pipe, rs2_error ** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(pipe);
    return new rs2_pipeline_profile{ pipe->pipeline->start(std::make_shared<pipeline::config>()) };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, pipe)

rs2_pipeline_profile* rs2_pipeline_start_with_config(rs2_pipeline* pipe, rs2_config* config, rs2_error ** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(pipe);
    VALIDATE_NOT_NULL(config);
    return new rs2_pipeline_profile{ pipe->pipeline->start(config->config) };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, pipe, config)

rs2_pipeline_profile* rs2_pipeline_start_with_callback(rs2_pipeline* pipe, rs2_frame_callback_ptr on_frame, void* user, rs2_error ** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(pipe);
    librealsense::frame_callback_ptr callback(new librealsense::frame_callback(on_frame, user), [](rs2_frame_callback* p) { p->release(); });
    return new rs2_pipeline_profile{ pipe->pipeline->start(std::make_shared<pipeline::config>(), move(callback)) };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, pipe, on_frame, user)

rs2_pipeline_profile* rs2_pipeline_start_with_config_and_callback(rs2_pipeline* pipe, rs2_config* config, rs2_frame_callback_ptr on_frame, void* user, rs2_error ** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(pipe);
    VALIDATE_NOT_NULL(config);
    librealsense::frame_callback_ptr callback(new librealsense::frame_callback(on_frame, user), [](rs2_frame_callback* p) { p->release(); });
    return new rs2_pipeline_profile{ pipe->pipeline->start(config->config, callback) };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, pipe, config, on_frame, user)

rs2_pipeline_profile* rs2_pipeline_start_with_callback_cpp(rs2_pipeline* pipe, rs2_frame_callback* callback, rs2_error ** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(pipe);
    VALIDATE_NOT_NULL(callback);

    return new rs2_pipeline_profile{ pipe->pipeline->start(std::make_shared<pipeline::config>(),
        { callback, [](rs2_frame_callback* p) { p->release(); } }) };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, pipe, callback)

rs2_pipeline_profile* rs2_pipeline_start_with_config_and_callback_cpp(rs2_pipeline* pipe, rs2_config* config, rs2_frame_callback* callback, rs2_error ** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(pipe);
    VALIDATE_NOT_NULL(config);
    VALIDATE_NOT_NULL(callback);

    return new rs2_pipeline_profile{ pipe->pipeline->start(config->config,
        { callback, [](rs2_frame_callback* p) { p->release(); } }) };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, pipe, config, callback)

rs2_pipeline_profile* rs2_pipeline_get_active_profile(rs2_pipeline* pipe, rs2_error ** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(pipe);

    return new rs2_pipeline_profile{ pipe->pipeline->get_active_profile() };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, pipe)

rs2_device* rs2_pipeline_profile_get_device(rs2_pipeline_profile* profile, rs2_error ** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(profile);

    auto dev = profile->profile->get_device();
    auto dev_info = std::make_shared<librealsense::readonly_device_info>(dev);
    return new rs2_device{ dev->get_context(), dev_info , dev };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, profile)

rs2_stream_profile_list* rs2_pipeline_profile_get_streams(rs2_pipeline_profile* profile, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(profile);
    return new rs2_stream_profile_list{ profile->profile->get_active_streams() };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, profile)

void rs2_delete_pipeline_profile(rs2_pipeline_profile* profile) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(profile);

    delete profile;
}
NOEXCEPT_RETURN(, profile)

//config
rs2_config* rs2_create_config(rs2_error** error) BEGIN_API_CALL
{
    return new rs2_config{ std::make_shared<pipeline::config>() };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, 0)

void rs2_delete_config(rs2_config* config) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(config);

    delete config;
}
NOEXCEPT_RETURN(, config)

void rs2_config_enable_stream(rs2_config* config,
    rs2_stream stream,
    int index,
    int width,
    int height,
    rs2_format format,
    int framerate,
    rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(config);
    config->config->enable_stream(stream, index, width, height, format, framerate);
}
HANDLE_EXCEPTIONS_AND_RETURN(, config, stream, index, width, height, format, framerate)

void rs2_config_enable_all_stream(rs2_config* config, rs2_error ** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(config);
    config->config->enable_all_stream();
}
HANDLE_EXCEPTIONS_AND_RETURN(, config)

void rs2_config_enable_device(rs2_config* config, const char* serial, rs2_error ** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(config);
    VALIDATE_NOT_NULL(serial);

    config->config->enable_device(serial);

}
HANDLE_EXCEPTIONS_AND_RETURN(, config, serial)

void rs2_config_enable_device_from_file_repeat_option(rs2_config* config, const char* file, int repeat_playback, rs2_error ** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(config);
    VALIDATE_NOT_NULL(file);

    config->config->enable_device_from_file(file, repeat_playback);
}
HANDLE_EXCEPTIONS_AND_RETURN(, config, file)

void rs2_config_enable_device_from_file(rs2_config* config, const char* file, rs2_error ** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(config);
    VALIDATE_NOT_NULL(file);

    config->config->enable_device_from_file(file, true);
}
HANDLE_EXCEPTIONS_AND_RETURN(, config, file)

void rs2_config_enable_record_to_file(rs2_config* config, const char* file, rs2_error ** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(config);
    VALIDATE_NOT_NULL(file);

    config->config->enable_record_to_file(file);
}
HANDLE_EXCEPTIONS_AND_RETURN(, config, file)

void rs2_config_disable_stream(rs2_config* config, rs2_stream stream, rs2_error ** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(config);
    config->config->disable_stream(stream);
}
HANDLE_EXCEPTIONS_AND_RETURN(, config, stream)

void rs2_config_disable_indexed_stream(rs2_config* config, rs2_stream stream, int index, rs2_error ** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(config);
    config->config->disable_stream(stream, index);
}
HANDLE_EXCEPTIONS_AND_RETURN(, config, stream, index)

void rs2_config_disable_all_streams(rs2_config* config, rs2_error ** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(config);
    config->config->disable_all_streams();
}
HANDLE_EXCEPTIONS_AND_RETURN(, config)

rs2_pipeline_profile* rs2_config_resolve(rs2_config* config, rs2_pipeline* pipe, rs2_error ** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(config);
    VALIDATE_NOT_NULL(pipe);
    return new rs2_pipeline_profile{ config->config->resolve(pipe->pipeline) };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, config, pipe)

int rs2_config_can_resolve(rs2_config* config, rs2_pipeline* pipe, rs2_error ** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(config);
    VALIDATE_NOT_NULL(pipe);
    return config->config->can_resolve(pipe->pipeline) ? 1 : 0;
}
HANDLE_EXCEPTIONS_AND_RETURN(0, config, pipe)

rs2_processing_block* rs2_create_processing_block(rs2_frame_processor_callback* proc, rs2_error** error) BEGIN_API_CALL
{
    auto block = std::make_shared<librealsense::processing_block>("Custom processing block");
    block->set_processing_callback({ proc, [](rs2_frame_processor_callback* p) { p->release(); } });

    return new rs2_processing_block{ block };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, proc)

rs2_processing_block* rs2_create_processing_block_fptr(rs2_frame_processor_callback_ptr proc, void * context, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(proc);

    auto block = std::make_shared<librealsense::processing_block>("Custom processing block");

    block->set_processing_callback({
        new librealsense::internal_frame_processor_fptr_callback(proc, context),
        [](rs2_frame_processor_callback* p) { } });

    return new rs2_processing_block{ block };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, proc, context)

int rs2_processing_block_register_simple_option(rs2_processing_block* block, rs2_option option_id, float min, float max, float step, float def, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(block);
    VALIDATE_LE(min, max);
    VALIDATE_RANGE(def, min, max);
    VALIDATE_LE(0, step);

    if (block->options->supports_option(option_id)) return false;
    std::shared_ptr<option> opt = std::make_shared<float_option>(option_range{ min, max, step, def });
    // TODO: am I supposed to use the extensions API here?
    auto options = dynamic_cast<options_container*>(block->options);
    options->register_option(option_id, opt);
    return true;
}
HANDLE_EXCEPTIONS_AND_RETURN(false, block, option_id, min, max, step, def)

rs2_processing_block* rs2_create_sync_processing_block(rs2_error** error) BEGIN_API_CALL
{
    auto block = std::make_shared<librealsense::syncer_process_unit>();

    return new rs2_processing_block{ block };
}
NOARGS_HANDLE_EXCEPTIONS_AND_RETURN(nullptr)

void rs2_start_processing(rs2_processing_block* block, rs2_frame_callback* on_frame, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(block);

    block->block->set_output_callback({ on_frame, [](rs2_frame_callback* p) { p->release(); } });
}
HANDLE_EXCEPTIONS_AND_RETURN(, block, on_frame)

void rs2_start_processing_fptr(rs2_processing_block* block, rs2_frame_callback_ptr on_frame, void* user, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(block);
    VALIDATE_NOT_NULL(on_frame);

    block->block->set_output_callback({ new frame_callback(on_frame, user), [](rs2_frame_callback* p) { } });
}
HANDLE_EXCEPTIONS_AND_RETURN(, block, on_frame, user)

void rs2_start_processing_queue(rs2_processing_block* block, rs2_frame_queue* queue, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(block);
    VALIDATE_NOT_NULL(queue);
    librealsense::frame_callback_ptr callback(
        new librealsense::frame_callback(rs2_enqueue_frame, queue));
    block->block->set_output_callback(move(callback));
}
HANDLE_EXCEPTIONS_AND_RETURN(, block, queue)

void rs2_process_frame(rs2_processing_block* block, rs2_frame* frame, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(block);
    VALIDATE_NOT_NULL(frame);

    block->block->invoke(frame_holder((frame_interface*)frame));
}
HANDLE_EXCEPTIONS_AND_RETURN(, block, frame)

void rs2_delete_processing_block(rs2_processing_block* block) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(block);

    delete block;
}
NOEXCEPT_RETURN(, block)

rs2_frame* rs2_extract_frame(rs2_frame* composite, int index, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(composite);

    auto cf = VALIDATE_INTERFACE((frame_interface*)composite, librealsense::composite_frame);

    VALIDATE_RANGE(index, 0, (int)cf->get_embedded_frames_count() - 1);
    auto res = cf->get_frame(index);
    res->acquire();
    return (rs2_frame*)res;
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, composite)

rs2_frame* rs2_allocate_composite_frame(rs2_source* source, rs2_frame** frames, int count, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(source)
        VALIDATE_NOT_NULL(frames)
        VALIDATE_RANGE(count, 1, 128);

    std::vector<frame_holder> holders(count);
    for (int i = 0; i < count; i++)
    {
        holders[i] = std::move(frame_holder((frame_interface*)frames[i]));
    }
    auto res = source->source->allocate_composite_frame(std::move(holders));

    return (rs2_frame*)res;
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, frames, count)

int rs2_embedded_frames_count(rs2_frame* composite, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(composite)

    auto cf = VALIDATE_INTERFACE((frame_interface*)composite, librealsense::composite_frame);

    return static_cast<int>(cf->get_embedded_frames_count());
}
HANDLE_EXCEPTIONS_AND_RETURN(0, composite)

rs2_vertex* rs2_get_frame_vertices(const rs2_frame* frame, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(frame);
    auto points = VALIDATE_INTERFACE((frame_interface*)frame, librealsense::points);
    return (rs2_vertex*)points->get_vertices();
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, frame)

void rs2_export_to_ply(const rs2_frame* frame, const char* fname, rs2_frame* texture, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(frame);
    VALIDATE_NOT_NULL(fname);
    auto points = VALIDATE_INTERFACE((frame_interface*)frame, librealsense::points);
    points->export_to_ply(fname, (frame_interface*)texture);
}
HANDLE_EXCEPTIONS_AND_RETURN(, frame, fname)

rs2_pixel* rs2_get_frame_texture_coordinates(const rs2_frame* frame, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(frame);
    auto points = VALIDATE_INTERFACE((frame_interface*)frame, librealsense::points);
    return (rs2_pixel*)points->get_texture_coordinates();
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, frame)

int rs2_get_frame_points_count(const rs2_frame* frame, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(frame);
    auto points = VALIDATE_INTERFACE((frame_interface*)frame, librealsense::points);
    return static_cast<int>(points->get_vertex_count());
}
HANDLE_EXCEPTIONS_AND_RETURN(0, frame)

rs2_processing_block* rs2_create_pointcloud(rs2_error** error) BEGIN_API_CALL
{
    return new rs2_processing_block { pointcloud::create() };
}
NOARGS_HANDLE_EXCEPTIONS_AND_RETURN(nullptr)

rs2_processing_block* rs2_create_yuy_decoder(rs2_error** error) BEGIN_API_CALL
{
    return new rs2_processing_block { std::make_shared<yuy2rgb>() };
}
NOARGS_HANDLE_EXCEPTIONS_AND_RETURN(nullptr)

rs2_processing_block* rs2_create_threshold(rs2_error** error) BEGIN_API_CALL
{
    return new rs2_processing_block { std::make_shared<threshold>() };
}
NOARGS_HANDLE_EXCEPTIONS_AND_RETURN(nullptr)

rs2_processing_block* rs2_create_align(rs2_stream align_to, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_ENUM(align_to);

    auto block = create_align(align_to);

    return new rs2_processing_block{ block };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, align_to)

rs2_processing_block* rs2_create_colorizer(rs2_error** error) BEGIN_API_CALL
{
    auto block = std::make_shared<librealsense::colorizer>();

    auto res = new rs2_processing_block{ block };

    auto res2 = (rs2_options*)res;

    return res;
}
NOARGS_HANDLE_EXCEPTIONS_AND_RETURN(nullptr)


rs2_processing_block* rs2_create_decimation_filter_block(rs2_error** error) BEGIN_API_CALL
{
    auto block = std::make_shared<librealsense::decimation_filter>();

    return new rs2_processing_block{ block };
}
NOARGS_HANDLE_EXCEPTIONS_AND_RETURN(nullptr)

rs2_processing_block* rs2_create_temporal_filter_block(rs2_error** error) BEGIN_API_CALL
{
    auto block = std::make_shared<librealsense::temporal_filter>();

    return new rs2_processing_block{ block };
}
NOARGS_HANDLE_EXCEPTIONS_AND_RETURN(nullptr)

rs2_processing_block* rs2_create_spatial_filter_block(rs2_error** error) BEGIN_API_CALL
{
    auto block = std::make_shared<librealsense::spatial_filter>();

    return new rs2_processing_block{ block };
}
NOARGS_HANDLE_EXCEPTIONS_AND_RETURN(nullptr)

rs2_processing_block* rs2_create_disparity_transform_block(unsigned char transform_to_disparity, rs2_error** error) BEGIN_API_CALL
{
    auto block = std::make_shared<librealsense::disparity_transform>(transform_to_disparity > 0);

    return new rs2_processing_block{ block };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, transform_to_disparity)

rs2_processing_block* rs2_create_hole_filling_filter_block(rs2_error** error) BEGIN_API_CALL
{
    auto block = std::make_shared<librealsense::hole_filling_filter>();

    return new rs2_processing_block{ block };
}
NOARGS_HANDLE_EXCEPTIONS_AND_RETURN(nullptr)

rs2_processing_block* rs2_create_rates_printer_block(rs2_error** error) BEGIN_API_CALL
{
    auto block = std::make_shared<librealsense::rates_printer>();

    return new rs2_processing_block{ block };
}
NOARGS_HANDLE_EXCEPTIONS_AND_RETURN(nullptr)

rs2_processing_block* rs2_create_zero_order_invalidation_block(rs2_error** error) BEGIN_API_CALL
{
    auto block = std::make_shared<librealsense::zero_order>();

    return new rs2_processing_block{ block };
}
NOARGS_HANDLE_EXCEPTIONS_AND_RETURN(nullptr)

float rs2_get_depth_scale(rs2_sensor* sensor, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(sensor);
    auto ds = VALIDATE_INTERFACE(sensor->sensor, librealsense::depth_sensor);
    return ds->get_depth_scale();
}
HANDLE_EXCEPTIONS_AND_RETURN(0.f, sensor)

float rs2_get_stereo_baseline(rs2_sensor* sensor, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(sensor);
    auto ds = VALIDATE_INTERFACE(sensor->sensor, librealsense::depth_stereo_sensor);
    return ds->get_stereo_baseline_mm();
}
HANDLE_EXCEPTIONS_AND_RETURN(0.f, sensor)

rs2_device* rs2_create_device_from_sensor(const rs2_sensor* sensor, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(sensor);
    return new rs2_device(sensor->parent);
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, sensor)

float rs2_depth_frame_get_distance(const rs2_frame* frame_ref, int x, int y, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(frame_ref);
    auto df = VALIDATE_INTERFACE(((frame_interface*)frame_ref), librealsense::depth_frame);
    return df->get_distance(x, y);
}
HANDLE_EXCEPTIONS_AND_RETURN(0, frame_ref, x, y)

float rs2_depth_stereo_frame_get_baseline(const rs2_frame* frame_ref, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(frame_ref);
    auto df = VALIDATE_INTERFACE(((frame_interface*)frame_ref), librealsense::disparity_frame);
    return df->get_stereo_baseline();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, frame_ref)

void rs2_pose_frame_get_pose_data(const rs2_frame* frame, rs2_pose* pose, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(frame);
    VALIDATE_NOT_NULL(pose);

    auto pf = VALIDATE_INTERFACE((frame_interface*)frame, librealsense::pose_frame);

    const float3 t = pf->get_translation();
    pose->translation = { t.x, t.y, t.z };

    const float3 v = pf->get_velocity();
    pose->velocity = { v.x, v.y, v.z };

    const float3 a = pf->get_acceleration();
    pose->acceleration = { a.x, a.y, a.z };

    const float4 r = pf->get_rotation();
    pose->rotation = { r.x, r.y, r.z, r.w };

    const float3 av = pf->get_angular_velocity();
    pose->angular_velocity = { av.x, av.y, av.z };

    const float3 aa = pf->get_angular_acceleration();
    pose->angular_acceleration = { aa.x, aa.y, aa.z };

    pose->tracker_confidence = pf->get_tracker_confidence();
    pose->mapper_confidence = pf->get_mapper_confidence();
}
HANDLE_EXCEPTIONS_AND_RETURN(, frame, pose)

rs2_time_t rs2_get_time(rs2_error** error) BEGIN_API_CALL
{
    return environment::get_instance().get_time_service()->get_time();
}
NOARGS_HANDLE_EXCEPTIONS_AND_RETURN(0)

rs2_device* rs2_create_software_device(rs2_error** error) BEGIN_API_CALL
{
    auto dev = std::make_shared<software_device>();
    return new rs2_device{ dev->get_context(), std::make_shared<readonly_device_info>(dev), dev };
}
NOARGS_HANDLE_EXCEPTIONS_AND_RETURN(0)

void rs2_software_device_create_matcher(rs2_device* dev, rs2_matchers m, rs2_error** error)BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(dev);
    auto df = VALIDATE_INTERFACE(dev->device, librealsense::software_device);
    df->set_matcher_type(m);
}
HANDLE_EXCEPTIONS_AND_RETURN(, dev, m)

rs2_sensor* rs2_software_device_add_sensor(rs2_device* dev, const char* sensor_name, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(dev);
    auto df = VALIDATE_INTERFACE(dev->device, librealsense::software_device);

    return new rs2_sensor(
        *dev,
        &df->add_software_sensor(sensor_name),
        0);
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, dev, sensor_name)

void rs2_software_sensor_on_video_frame(rs2_sensor* sensor, rs2_software_video_frame frame, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(sensor);
    auto bs = VALIDATE_INTERFACE(sensor->sensor, librealsense::software_sensor);
    return bs->on_video_frame(frame);
}
HANDLE_EXCEPTIONS_AND_RETURN(, sensor, frame.pixels)

void rs2_software_sensor_on_motion_frame(rs2_sensor* sensor, rs2_software_motion_frame frame, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(sensor);
    auto bs = VALIDATE_INTERFACE(sensor->sensor, librealsense::software_sensor);
    return bs->on_motion_frame(frame);
}
HANDLE_EXCEPTIONS_AND_RETURN(, sensor, frame.data)

void rs2_software_sensor_on_pose_frame(rs2_sensor* sensor, rs2_software_pose_frame frame, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(sensor);
    auto bs = VALIDATE_INTERFACE(sensor->sensor, librealsense::software_sensor);
    return bs->on_pose_frame(frame);
}
HANDLE_EXCEPTIONS_AND_RETURN(, sensor, frame.data)

void rs2_software_sensor_set_metadata(rs2_sensor* sensor, rs2_frame_metadata_value key, rs2_metadata_type value, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(sensor);
    auto bs = VALIDATE_INTERFACE(sensor->sensor, librealsense::software_sensor);
    return bs->set_metadata(key, value);
}
HANDLE_EXCEPTIONS_AND_RETURN(, sensor, key, value)

rs2_stream_profile* rs2_software_sensor_add_video_stream(rs2_sensor* sensor, rs2_video_stream video_stream, rs2_error** error) BEGIN_API_CALL
{
    auto bs = VALIDATE_INTERFACE(sensor->sensor, librealsense::software_sensor);
    return bs->add_video_stream(video_stream)->get_c_wrapper();
}
HANDLE_EXCEPTIONS_AND_RETURN(0,sensor, video_stream.type, video_stream.index, video_stream.fmt, video_stream.width, video_stream.height, video_stream.uid)

rs2_stream_profile* rs2_software_sensor_add_motion_stream(rs2_sensor* sensor, rs2_motion_stream motion_stream, rs2_error** error) BEGIN_API_CALL
{
    auto bs = VALIDATE_INTERFACE(sensor->sensor, librealsense::software_sensor);
    return bs->add_motion_stream(motion_stream)->get_c_wrapper();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, sensor, motion_stream.type, motion_stream.index, motion_stream.fmt, motion_stream.uid)

rs2_stream_profile* rs2_software_sensor_add_pose_stream(rs2_sensor* sensor, rs2_pose_stream pose_stream, rs2_error** error) BEGIN_API_CALL
{
    auto bs = VALIDATE_INTERFACE(sensor->sensor, librealsense::software_sensor);
    return bs->add_pose_stream(pose_stream)->get_c_wrapper();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, sensor, pose_stream.type, pose_stream.index, pose_stream.fmt, pose_stream.uid)

void rs2_software_sensor_add_read_only_option(rs2_sensor* sensor, rs2_option option, float val, rs2_error** error) BEGIN_API_CALL
{
    auto bs = VALIDATE_INTERFACE(sensor->sensor, librealsense::software_sensor);
    return bs->add_read_only_option(option, val);
}
HANDLE_EXCEPTIONS_AND_RETURN(, sensor, option, val)

void rs2_software_sensor_update_read_only_option(rs2_sensor* sensor, rs2_option option, float val, rs2_error** error) BEGIN_API_CALL
{
    auto bs = VALIDATE_INTERFACE(sensor->sensor, librealsense::software_sensor);
    return bs->update_read_only_option(option, val);
}
HANDLE_EXCEPTIONS_AND_RETURN(, sensor, option, val)

void rs2_log(rs2_log_severity severity, const char * message, rs2_error ** error) BEGIN_API_CALL
{
    VALIDATE_ENUM(severity);
    VALIDATE_NOT_NULL(message);
    switch (severity)
    {
    case RS2_LOG_SEVERITY_DEBUG:
        LOG_DEBUG(message);
        break;
    case RS2_LOG_SEVERITY_INFO:
        LOG_INFO(message);
        break;
    case RS2_LOG_SEVERITY_WARN:
        LOG_WARNING(message);
        break;
    case RS2_LOG_SEVERITY_ERROR:
        LOG_ERROR(message);
        break;
    case RS2_LOG_SEVERITY_FATAL:
        LOG_FATAL(message);
        break;
    case RS2_LOG_SEVERITY_NONE:
        break;
    default:
        LOG_INFO(message);
    }
}
HANDLE_EXCEPTIONS_AND_RETURN(, severity, message)

void rs2_loopback_enable(const rs2_device* device, const char* from_file, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_NOT_NULL(from_file);

    auto loopback = VALIDATE_INTERFACE(device->device, librealsense::tm2_extensions);
    loopback->enable_loopback(from_file);

}
HANDLE_EXCEPTIONS_AND_RETURN(, device, from_file)

void rs2_loopback_disable(const rs2_device* device, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(device);

    auto loopback = VALIDATE_INTERFACE(device->device, librealsense::tm2_extensions);
    loopback->disable_loopback();
}
HANDLE_EXCEPTIONS_AND_RETURN(, device)

int rs2_loopback_is_enabled(const rs2_device* device, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(device);

    auto loopback = VALIDATE_INTERFACE(device->device, librealsense::tm2_extensions);
    return loopback->is_enabled() ? 1 : 0;
}
HANDLE_EXCEPTIONS_AND_RETURN(0, device)

void rs2_connect_tm2_controller(const rs2_device* device, const unsigned char* mac, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_NOT_NULL(mac);

    auto tm2 = VALIDATE_INTERFACE(device->device, librealsense::tm2_extensions);
    tm2->connect_controller({ mac[0], mac[1], mac[2], mac[3], mac[4], mac[5] });
}
HANDLE_EXCEPTIONS_AND_RETURN(, device)

void rs2_disconnect_tm2_controller(const rs2_device* device, int id, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(device);

    auto tm2 = VALIDATE_INTERFACE(device->device, librealsense::tm2_extensions);
    tm2->disconnect_controller(id);
}
HANDLE_EXCEPTIONS_AND_RETURN(, device)

rs2_processing_block_list* rs2_get_recommended_processing_blocks(rs2_sensor* sensor, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(sensor);                            
    return new rs2_processing_block_list{ sensor->sensor->get_recommended_processing_blocks() };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, sensor)

rs2_processing_block* rs2_get_processing_block(const rs2_processing_block_list* list, int index, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(list);
    VALIDATE_RANGE(index, 0, (int)list->list.size() - 1);

    return new rs2_processing_block(list->list[index]);
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, list, index)

int rs2_get_recommended_processing_blocks_count(const rs2_processing_block_list* list, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(list);
    return static_cast<int>(list->list.size());
}
HANDLE_EXCEPTIONS_AND_RETURN(0, list)

void rs2_delete_recommended_processing_blocks(rs2_processing_block_list* list) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(list);
    delete list;
}
NOEXCEPT_RETURN(, list)

const char* rs2_get_processing_block_info(const rs2_processing_block* block, rs2_camera_info info, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(block);
    VALIDATE_ENUM(info);
    if (block->block->supports_info(info))
    {
        return block->block->get_info(info).c_str();
    }
    throw librealsense::invalid_value_exception(librealsense::to_string() << "Info " << rs2_camera_info_to_string(info) << " not supported by processing block!");
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, block, info)

int rs2_supports_processing_block_info(const rs2_processing_block* block, rs2_camera_info info, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(block);
    VALIDATE_ENUM(info);
    return block->block->supports_info(info);
}
HANDLE_EXCEPTIONS_AND_RETURN(false, block, info)

int rs2_import_localization_map(const rs2_sensor* sensor, const unsigned char* lmap_blob, unsigned int blob_size, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(sensor);
    VALIDATE_NOT_NULL(lmap_blob);
    VALIDATE_RANGE(blob_size,1, std::numeric_limits<uint32_t>::max());      // Set by FW

    auto pose_snr = VALIDATE_INTERFACE(sensor->sensor, librealsense::pose_sensor_interface);

    std::vector<uint8_t> buffer_to_send(lmap_blob, lmap_blob + blob_size);
    int ret = pose_snr->import_relocalization_map(buffer_to_send);
    if (!ret)
        throw librealsense::invalid_value_exception(librealsense::to_string() << "import localization failed, map size " << blob_size);
    return ret;
}
HANDLE_EXCEPTIONS_AND_RETURN(0, sensor, lmap_blob, blob_size)

const rs2_raw_data_buffer* rs2_export_localization_map(const rs2_sensor* sensor, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(sensor);

    auto pose_snr = VALIDATE_INTERFACE(sensor->sensor, librealsense::pose_sensor_interface);
    std::vector<uint8_t> recv_buffer;
    if (pose_snr->export_relocalization_map(recv_buffer))
        return new rs2_raw_data_buffer{ recv_buffer };
    return nullptr;
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, sensor)

int rs2_set_static_node(const rs2_sensor* sensor, const char* guid, const rs2_vector pos, const rs2_quaternion orient, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(sensor);
    VALIDATE_NOT_NULL(guid);
    auto pose_snr = VALIDATE_INTERFACE(sensor->sensor, librealsense::pose_sensor_interface);
    std::string s_guid(guid);
    VALIDATE_RANGE(s_guid.size(), 1, 127);      // T2xx spec

    return int(pose_snr->set_static_node(s_guid, { pos.x, pos.y, pos.z }, { orient.x, orient.y, orient.z, orient.w }));
}
HANDLE_EXCEPTIONS_AND_RETURN(0, sensor, guid, pos, orient)

int rs2_get_static_node(const rs2_sensor* sensor, const char* guid, rs2_vector *pos, rs2_quaternion *orient, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(sensor);
    VALIDATE_NOT_NULL(guid);
    VALIDATE_NOT_NULL(pos);
    VALIDATE_NOT_NULL(orient);
    auto pose_snr = VALIDATE_INTERFACE(sensor->sensor, librealsense::pose_sensor_interface);
    std::string s_guid(guid);
    VALIDATE_RANGE(s_guid.size(), 1, 127);      // T2xx spec

    float3 t_pos{};
    float4 t_or {};
    int ret = 0;
    if (ret = pose_snr->get_static_node(s_guid, t_pos, t_or))
    {
        pos->x = t_pos.x;
        pos->y = t_pos.y;
        pos->z = t_pos.z;
        orient->x = t_or.x;
        orient->y = t_or.y;
        orient->z = t_or.z;
        orient->w = t_or.w;
    }
    return ret;
}
HANDLE_EXCEPTIONS_AND_RETURN(0, sensor, guid, pos, orient)

int rs2_load_wheel_odometry_config(const rs2_sensor* sensor, const unsigned char* odometry_blob, unsigned int blob_size, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(sensor);
    VALIDATE_NOT_NULL(odometry_blob);
    VALIDATE_RANGE(blob_size, 1, std::numeric_limits<uint32_t>::max());

    auto wo_snr = VALIDATE_INTERFACE(sensor->sensor, librealsense::wheel_odometry_interface);

    std::vector<uint8_t> buffer_to_send(odometry_blob, odometry_blob + blob_size);
    int ret = wo_snr->load_wheel_odometery_config(buffer_to_send);
    if (!ret)
        throw librealsense::wrong_api_call_sequence_exception(librealsense::to_string() << "Load wheel odometry config failed, file size " << blob_size);
    return ret;
}
HANDLE_EXCEPTIONS_AND_RETURN(0, sensor, odometry_blob, blob_size)

int rs2_send_wheel_odometry(const rs2_sensor* sensor, char wo_sensor_id, unsigned int frame_num,
                            const rs2_vector translational_velocity, rs2_error** error) BEGIN_API_CALL
{
    VALIDATE_NOT_NULL(sensor);
    auto wo_snr = VALIDATE_INTERFACE(sensor->sensor, librealsense::wheel_odometry_interface);

    return wo_snr->send_wheel_odometry(wo_sensor_id, frame_num, { translational_velocity.x, translational_velocity.y, translational_velocity.z });
}
HANDLE_EXCEPTIONS_AND_RETURN(0, sensor, wo_sensor_id, frame_num, translational_velocity)
