// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <functional>   // For function
#include <climits>

#include "context.h"
#include "device.h"
#include "archive.h"
#include "concurrency.h"
#include "algo.h"
#include "types.h"
#include "sync.h"

////////////////////////
// API implementation //
////////////////////////

struct rs2_error
{
    std::string message;
    const char * function;
    std::string args;
    rs2_exception_type exception_type;
};

struct rs2_raw_data_buffer
{
    std::vector<uint8_t> buffer;
};

struct rs2_stream_profile_list
{
    std::vector<rsimpl2::stream_profile> list;
};

struct rs2_device
{
    std::shared_ptr<rsimpl2::context> ctx;
    std::shared_ptr<rsimpl2::device_info> info;
    std::shared_ptr<rsimpl2::device> device;
    unsigned int subdevice;
};

struct rs2_device_info
{
    std::shared_ptr<rsimpl2::context> ctx;
    std::shared_ptr<rsimpl2::device_info> info;
    unsigned int subdevice;
};

struct rs2_context
{
    std::shared_ptr<rsimpl2::context> ctx;
};

struct rs2_device_list
{
    std::shared_ptr<rsimpl2::context> ctx;
    std::vector<rs2_device_info> list;
};

struct rs2_notification
{
    rs2_notification(const rsimpl2::notification* notification)
        :_notification(notification){}

    const rsimpl2::notification* _notification;
};

struct rs2_syncer
{
    std::shared_ptr<rsimpl2::sync_interface> syncer;
};

struct frame_holder
{
    rs2_frame* frame;

    ~frame_holder()
    {
        if (frame) rs2_release_frame(frame);
    }

    frame_holder(const frame_holder&) = delete;
    frame_holder(frame_holder&& other)
        : frame(other.frame)
    {
        other.frame = nullptr;
    }

    frame_holder() : frame(nullptr) {}

    frame_holder& operator=(const frame_holder&) = delete;
    frame_holder& operator=(frame_holder&& other)
    {
        frame = other.frame;
        other.frame = nullptr;
        return *this;
    }
};

struct rs2_frame_queue
{
    explicit rs2_frame_queue(int cap)
        : queue(cap)
    {
    }

    single_consumer_queue<rsimpl2::frame_holder> queue;
};


// This facility allows for translation of exceptions to rs2_error structs at the API boundary
namespace rsimpl2
{
    template<class T> void stream_args(std::ostream & out, const char * names, const T & last) { out << names << ':' << last; }
    template<class T, class... U> void stream_args(std::ostream & out, const char * names, const T & first, const U &... rest)
    {
        while(*names && *names != ',') out << *names++;
        out << ':' << first << ", ";
        while(*names && (*names == ',' || isspace(*names))) ++names;
        stream_args(out, names, rest...);
    }

    static void translate_exception(const char * name, std::string args, rs2_error ** error)
    {
        try { throw; }
        catch (const librealsense_exception& e) { if (error) *error = new rs2_error{ e.what(), name, move(args), e.get_exception_type() }; }
        catch (const std::exception& e) { if (error) *error = new rs2_error {e.what(), name, move(args)}; }
        catch (...) { if (error) *error = new rs2_error {"unknown error", name, move(args)}; }
    }


    void notifications_proccessor::raise_notification(const notification n)
    {
        _dispatcher.invoke([this, n](dispatcher::cancellable_timer ct)
        {
            std::lock_guard<std::mutex> lock(_callback_mutex);
            rs2_notification noti(&n);
            if (_callback)_callback->on_notification(&noti);
            else
            {
#ifdef DEBUG

#endif // !DEBUG

            }
        });
    }

}

#define NOEXCEPT_RETURN(R, ...) catch(...) { std::ostringstream ss; rsimpl2::stream_args(ss, #__VA_ARGS__, __VA_ARGS__); rs2_error* e; rsimpl2::translate_exception(__FUNCTION__, ss.str(), &e); LOG_WARNING(rs2_get_error_message(e)); rs2_free_error(e); return R; }
#define HANDLE_EXCEPTIONS_AND_RETURN(R, ...) catch(...) { std::ostringstream ss; rsimpl2::stream_args(ss, #__VA_ARGS__, __VA_ARGS__); rsimpl2::translate_exception(__FUNCTION__, ss.str(), error); return R; }
#define VALIDATE_NOT_NULL(ARG) if(!(ARG)) throw std::runtime_error("null pointer passed for argument \"" #ARG "\"");
#define VALIDATE_ENUM(ARG) if(!rsimpl2::is_valid(ARG)) { std::ostringstream ss; ss << "invalid enum value for argument \"" #ARG "\""; throw rsimpl2::invalid_value_exception(ss.str()); }
#define VALIDATE_RANGE(ARG, MIN, MAX) if((ARG) < (MIN) || (ARG) > (MAX)) { std::ostringstream ss; ss << "out of range value for argument \"" #ARG "\""; throw rsimpl2::invalid_value_exception(ss.str()); }
#define VALIDATE_LE(ARG, MAX) if((ARG) > (MAX)) { std::ostringstream ss; ss << "out of range value for argument \"" #ARG "\""; throw std::runtime_error(ss.str()); }
#define VALIDATE_NATIVE_STREAM(ARG) VALIDATE_ENUM(ARG); if(ARG >= RS2_STREAM_NATIVE_COUNT) { std::ostringstream ss; ss << "argument \"" #ARG "\" must be a native stream"; throw rsimpl2::wrong_value_exception(ss.str()); }

int major(int version)
{
    return version / 10000;
}
int minor(int version)
{
    return (version % 10000) / 100;
}
int patch(int version)
{
    return (version % 100);
}

std::string api_version_to_string(int version)
{
    if (major(version) == 0) return rsimpl2::to_string() << version;
    return rsimpl2::to_string() << major(version) << "." << minor(version) << "." << patch(version);
}

void report_version_mismatch(int runtime, int compiletime)
{
    throw rsimpl2::invalid_value_exception(rsimpl2::to_string() << "API version mismatch: librealsense.so was compiled with API version "
        << api_version_to_string(runtime) << " but the application was compiled with "
        << api_version_to_string(compiletime) << "! Make sure correct version of the library is installed (make install)");
}

void verify_version_compatibility(int api_version)
{
    rs2_error* error = nullptr;
    auto runtime_api_version = rs2_get_api_version(&error);
    if (error)
        throw rsimpl2::invalid_value_exception(rs2_get_error_message(error));

    if ((runtime_api_version < 10) || (api_version < 10))
    {
        // when dealing with version < 1.0.0 that were still using single number for API version, require exact match
        if (api_version != runtime_api_version)
            report_version_mismatch(runtime_api_version, api_version);
    }
    else if ((major(runtime_api_version) == 1 && minor(runtime_api_version) <= 9)
        || (major(api_version) == 1 && minor(api_version) <= 9))
    {
        // when dealing with version < 1.10.0, API breaking changes are still possible without minor version change, require exact match
        if (api_version != runtime_api_version)
            report_version_mismatch(runtime_api_version, api_version);
    }
    else
    {
        // starting with 1.10.0, versions with same patch are compatible
        if ((major(api_version) != major(runtime_api_version))
            || (minor(api_version) != minor(runtime_api_version)))
            report_version_mismatch(runtime_api_version, api_version);
    }
}

rs2_context * rs2_create_context(int api_version, rs2_error ** error) try
{
    verify_version_compatibility(api_version);

    return new rs2_context{ std::make_shared<rsimpl2::context>(rsimpl2::backend_type::standard) };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, api_version)

void rs2_delete_context(rs2_context * context) try
{
    VALIDATE_NOT_NULL(context);
    delete context;
}
NOEXCEPT_RETURN(, context)

rs2_device_list* rs2_query_devices(const rs2_context* context, rs2_error** error) try
{
    VALIDATE_NOT_NULL(context);

    std::vector<rs2_device_info> results;
    for (auto&& dev_info : context->ctx->query_devices())
    {
        try
        {
            for (unsigned int i = 0; i < dev_info->get_subdevice_count(); i++)
            {
                rs2_device_info d{ context->ctx, dev_info, i };
                results.push_back(d);
            }
        }
        catch (...)
        {
            LOG_WARNING("Could not open device!");
        }
    }

    return new rs2_device_list{ context->ctx, results };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, context)

rs2_time_t rs2_get_context_time(const rs2_context* context, rs2_error** error) try
{
    VALIDATE_NOT_NULL(context);
    return context->ctx->get_time();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, context)

rs2_device_list* rs2_query_adjacent_devices(const rs2_device* device, rs2_error** error) try
{
    VALIDATE_NOT_NULL(device);

    std::vector<rs2_device_info> results;
    try
    {
        auto dev = device->device;
        for (unsigned int i = 0; i < dev->get_endpoints_count(); i++)
        {
            rs2_device_info d{ device->ctx, device->info, i };
            results.push_back(d);
        }
    }
    catch (...)
    {
        LOG_WARNING("Could not open device!");
    }

    return new rs2_device_list{ device->ctx, results };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, device)

int rs2_get_device_count(const rs2_device_list* list, rs2_error** error) try
{
    VALIDATE_NOT_NULL(list);
    return static_cast<int>(list->list.size());
}
HANDLE_EXCEPTIONS_AND_RETURN(0, list)

void rs2_delete_device_list(rs2_device_list* list) try
{
    VALIDATE_NOT_NULL(list);
    delete list;
}
NOEXCEPT_RETURN(, list)

rs2_device* rs2_create_device(const rs2_device_list* list, int index, rs2_error** error) try
{
    VALIDATE_NOT_NULL(list);
    VALIDATE_RANGE(index, 0, (int)list->list.size() - 1);

    return new rs2_device{ list->ctx,
                          list->list[index].info,
                          list->list[index].info->get_device(),
                          list->list[index].subdevice
                        };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, list, index)

void rs2_delete_device(rs2_device* device) try
{
    VALIDATE_NOT_NULL(device);
    delete device;
}
NOEXCEPT_RETURN(, device)

rs2_stream_modes_list* rs2_get_stream_modes(rs2_device* device, rs2_error** error) try
{
    VALIDATE_NOT_NULL(device);
    return new rs2_stream_modes_list{ device->device->get_endpoint(device->subdevice).get_principal_requests() };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, device)

void rs2_get_stream_mode(const rs2_stream_modes_list* list, int index, rs2_stream* stream, int* width, int* height, int* fps, rs2_format* format, rs2_error** error) try
{
    VALIDATE_NOT_NULL(list);
    VALIDATE_RANGE(index, 0, (int)list->list.size() - 1);

    VALIDATE_NOT_NULL(stream);
    VALIDATE_NOT_NULL(width);
    VALIDATE_NOT_NULL(height);
    VALIDATE_NOT_NULL(fps);
    VALIDATE_NOT_NULL(format);

    *stream = list->list[index].stream;
    *width = list->list[index].width;
    *height = list->list[index].height;
    *fps = list->list[index].fps;
    *format = list->list[index].format;
}
HANDLE_EXCEPTIONS_AND_RETURN(, list, index, width, height, fps, format)

int rs2_get_modes_count(const rs2_stream_modes_list* list, rs2_error** error) try
{
    VALIDATE_NOT_NULL(list);
    return static_cast<int>(list->list.size());
}
HANDLE_EXCEPTIONS_AND_RETURN(0, list)

void rs2_delete_modes_list(rs2_stream_modes_list* list) try
{
    VALIDATE_NOT_NULL(list);
    delete list;
}
NOEXCEPT_RETURN(, list)

rs2_raw_data_buffer* rs2_send_and_receive_raw_data(rs2_device* device, void* raw_data_to_send, unsigned size_of_raw_data_to_send, rs2_error** error) try
{
    VALIDATE_NOT_NULL(device);
    auto raw_data_buffer = static_cast<uint8_t*>(raw_data_to_send);
    std::vector<uint8_t> buffer_to_send(raw_data_buffer, raw_data_buffer + size_of_raw_data_to_send);
    auto ret_data = device->device->send_receive_raw_data(buffer_to_send);
    return new rs2_raw_data_buffer{ ret_data };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, device)

const unsigned char* rs2_get_raw_data(const rs2_raw_data_buffer* buffer, rs2_error** error) try
{
    VALIDATE_NOT_NULL(buffer);
    return buffer->buffer.data();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, buffer)

int rs2_get_raw_data_size(const rs2_raw_data_buffer* buffer, rs2_error** error) try
{
    VALIDATE_NOT_NULL(buffer);
    return static_cast<int>(buffer->buffer.size());
}
HANDLE_EXCEPTIONS_AND_RETURN(0, buffer)

void rs2_delete_raw_data(rs2_raw_data_buffer* buffer) try
{
    VALIDATE_NOT_NULL(buffer);
    delete buffer;
}
NOEXCEPT_RETURN(, buffer)

void rs2_open(rs2_device* device, rs2_stream stream,
    int width, int height, int fps, rs2_format format, rs2_error** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(format);
    VALIDATE_ENUM(stream);

    std::vector<rsimpl2::stream_profile> request;
    request.push_back({ stream, static_cast<uint32_t>(width),
            static_cast<uint32_t>(height), static_cast<uint32_t>(fps), format });
    device->device->get_endpoint(device->subdevice).open(request);
}
HANDLE_EXCEPTIONS_AND_RETURN(, device, stream, width, height, fps, format)

void rs2_open_multiple(rs2_device* device,
    const rs2_stream* stream, const int* width, const int* height, const int* fps,
    const rs2_format* format, int count, rs2_error** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_NOT_NULL(stream);
    VALIDATE_NOT_NULL(width);
    VALIDATE_NOT_NULL(height);
    VALIDATE_NOT_NULL(fps);
    VALIDATE_NOT_NULL(format);

    std::vector<rsimpl2::stream_profile> request;
    for (auto i = 0; i < count; i++)
    {
        request.push_back({ stream[i], static_cast<uint32_t>(width[i]),
                            static_cast<uint32_t>(height[i]), static_cast<uint32_t>(fps[i]), format[i] });
    }
    device->device->get_endpoint(device->subdevice).open(request);
}
HANDLE_EXCEPTIONS_AND_RETURN(, device, stream, width, height, fps, format)

void rs2_close(const rs2_device* device, rs2_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    device->device->get_endpoint(device->subdevice).close();
}
HANDLE_EXCEPTIONS_AND_RETURN(, device)

int rs2_is_option_read_only(const rs2_device* device, rs2_option option, rs2_error** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(option);
    return device->device->get_endpoint(device->subdevice).get_option(option).is_read_only();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, device, option)

float rs2_get_option(const rs2_device* device, rs2_option option, rs2_error** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(option);
    return device->device->get_endpoint(device->subdevice).get_option(option).query();
}
HANDLE_EXCEPTIONS_AND_RETURN(0.0f, device, option)

void rs2_set_option(const rs2_device* device, rs2_option option, float value, rs2_error** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(option);
    device->device->get_endpoint(device->subdevice).get_option(option).set(value);
}
HANDLE_EXCEPTIONS_AND_RETURN(, device, option, value)

int rs2_supports_option(const rs2_device* device, rs2_option option, rs2_error** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(option);
    return device->device->get_endpoint(device->subdevice).supports_option(option);
}
HANDLE_EXCEPTIONS_AND_RETURN(0, device, option)

void rs2_get_option_range(const rs2_device* device, rs2_option option,
    float* min, float* max, float* step, float* def, rs2_error** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(option);
    VALIDATE_NOT_NULL(min);
    VALIDATE_NOT_NULL(max);
    VALIDATE_NOT_NULL(step);
    VALIDATE_NOT_NULL(def);
    auto range = device->device->get_endpoint(device->subdevice).get_option(option).get_range();
    *min = range.min;
    *max = range.max;
    *def = range.def;
    *step = range.step;
}
HANDLE_EXCEPTIONS_AND_RETURN(, device, option, min, max, step, def)

const char* rs2_get_camera_info(const rs2_device* device, rs2_camera_info info, rs2_error** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(info);
    if (device->device->get_endpoint(device->subdevice).supports_info(info))
    {
        return device->device->get_endpoint(device->subdevice).get_info(info).c_str();
    }
    throw rsimpl2::invalid_value_exception(rsimpl2::to_string() << "info " << rs2_camera_info_to_string(info) << " not supported by subdevice " << device->subdevice);
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, device, info)

int rs2_supports_camera_info(const rs2_device* device, rs2_camera_info info, rs2_error** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(info);
    return device->device->get_endpoint(device->subdevice).supports_info(info);
}
HANDLE_EXCEPTIONS_AND_RETURN(false, device, info)


void rs2_start(const rs2_device* device, rs2_frame_callback_ptr on_frame, void * user, rs2_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_NOT_NULL(on_frame);
    rsimpl2::frame_callback_ptr callback(
        new rsimpl2::frame_callback(on_frame, user));
    device->device->get_endpoint(device->subdevice).starter().start(RS2_STREAM_ANY, move(callback));
}
HANDLE_EXCEPTIONS_AND_RETURN(, device, on_frame, user)

void rs2_start_stream(const rs2_device* device, rs2_stream stream, rs2_frame_callback_ptr on_frame, void * user, rs2_error ** error) try
{
    VALIDATE_NOT_NULL(device);
   VALIDATE_NOT_NULL(on_frame);
    rsimpl2::frame_callback_ptr callback(
        new rsimpl2::frame_callback(on_frame, user));
    device->device->get_endpoint(device->subdevice).starter().start(stream, move(callback));
}
HANDLE_EXCEPTIONS_AND_RETURN(, device, stream, on_frame, user)

void rs2_set_notifications_callback(const rs2_device * device, rs2_notification_callback_ptr on_notification, void * user, rs2_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_NOT_NULL(on_notification);
    rsimpl2::notifications_callback_ptr callback(
        new rsimpl2::notifications_callback(on_notification, user),
        [](rs2_notifications_callback* p) { delete p; });
    device->device->get_endpoint(device->subdevice).register_notifications_callback(std::move(callback));
}
HANDLE_EXCEPTIONS_AND_RETURN(, device, on_notification, user)

void rs2_start_queue(const rs2_device* device, rs2_stream stream, rs2_frame_queue* queue, rs2_error** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(stream);
    VALIDATE_NOT_NULL(queue);
    rsimpl2::frame_callback_ptr callback(
        new rsimpl2::frame_callback(rs2_enqueue_frame, queue));
    device->device->get_endpoint(device->subdevice).starter().start(stream, move(callback));
}
HANDLE_EXCEPTIONS_AND_RETURN(, device, stream, queue)

void rs2_start_cpp(const rs2_device* device, rs2_frame_callback * callback, rs2_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_NOT_NULL(callback);
    device->device->get_endpoint(device->subdevice).starter().start(RS2_STREAM_ANY, { callback, [](rs2_frame_callback* p) { p->release(); } });
}
HANDLE_EXCEPTIONS_AND_RETURN(, device, callback)

void rs2_start_stream_cpp(const rs2_device* device, rs2_stream stream, rs2_frame_callback * callback, rs2_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_NOT_NULL(callback);
    device->device->get_endpoint(device->subdevice).starter().start(stream, { callback, [](rs2_frame_callback* p) { p->release(); } });
}
HANDLE_EXCEPTIONS_AND_RETURN(, device, stream, callback)

void rs2_set_notifications_callback_cpp(const rs2_device * device, rs2_notifications_callback * callback, rs2_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_NOT_NULL(callback);
    device->device->get_endpoint(device->subdevice).register_notifications_callback({ callback, [](rs2_notifications_callback* p) { p->release(); } });
}
HANDLE_EXCEPTIONS_AND_RETURN(, device, callback)

void rs2_stop(const rs2_device* device, rs2_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    device->device->get_endpoint(device->subdevice).starter().stop(RS2_STREAM_ANY);
}
HANDLE_EXCEPTIONS_AND_RETURN(, device)

void rs2_stop_stream(const rs2_device* device, rs2_stream stream, rs2_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    device->device->get_endpoint(device->subdevice).starter().stop(stream);
}
HANDLE_EXCEPTIONS_AND_RETURN(, stream, device)

double rs2_get_frame_metadata(const rs2_frame * frame, rs2_frame_metadata frame_metadata, rs2_error ** error) try
{
    VALIDATE_NOT_NULL(frame);
    return frame->get()->get_frame_metadata(frame_metadata);
}
HANDLE_EXCEPTIONS_AND_RETURN(0, frame)

const char * rs2_get_notification_description(rs2_notification * notification, rs2_error** error)try
{
    VALIDATE_NOT_NULL(notification);
    return notification->_notification->description.c_str();
}
HANDLE_EXCEPTIONS_AND_RETURN(0,notification)

rs2_time_t rs2_get_notification_timestamp(rs2_notification * notification, rs2_error** error)try
{
    VALIDATE_NOT_NULL(notification);
    return notification->_notification->timestamp;
}
HANDLE_EXCEPTIONS_AND_RETURN(0,notification)

rs2_log_severity rs2_get_notification_severity(rs2_notification * notification, rs2_error** error)try
{
    VALIDATE_NOT_NULL(notification);
    return (rs2_log_severity)notification->_notification->severity;
}
HANDLE_EXCEPTIONS_AND_RETURN(RS2_LOG_SEVERITY_NONE, notification)

rs2_notification_category rs2_get_notification_category(rs2_notification * notification, rs2_error** error)try
{
    VALIDATE_NOT_NULL(notification);
    return (rs2_notification_category)notification->_notification->category;
}
HANDLE_EXCEPTIONS_AND_RETURN(RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR, notification)

int rs2_supports_frame_metadata(const rs2_frame * frame, rs2_frame_metadata frame_metadata, rs2_error ** error) try
{
    VALIDATE_NOT_NULL(frame);
    return frame->get()->supports_frame_metadata(frame_metadata);
}
HANDLE_EXCEPTIONS_AND_RETURN(0, frame)

rs2_time_t rs2_get_frame_timestamp(const rs2_frame * frame_ref, rs2_error ** error) try
{
    VALIDATE_NOT_NULL(frame_ref);
    return frame_ref->get()->get_frame_timestamp();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, frame_ref)

rs2_timestamp_domain rs2_get_frame_timestamp_domain(const rs2_frame * frame_ref, rs2_error ** error) try
{
    VALIDATE_NOT_NULL(frame_ref);
    return frame_ref->get()->get_frame_timestamp_domain();
}
HANDLE_EXCEPTIONS_AND_RETURN(RS2_TIMESTAMP_DOMAIN_COUNT, frame_ref)

const void * rs2_get_frame_data(const rs2_frame * frame_ref, rs2_error ** error) try
{
    VALIDATE_NOT_NULL(frame_ref);
    return frame_ref->get()->get_frame_data();
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, frame_ref)

int rs2_get_frame_width(const rs2_frame * frame_ref, rs2_error ** error) try
{
    VALIDATE_NOT_NULL(frame_ref);
    return frame_ref->get()->get_width();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, frame_ref)

int rs2_get_frame_height(const rs2_frame * frame_ref, rs2_error ** error) try
{
    VALIDATE_NOT_NULL(frame_ref);
    return frame_ref->get()->get_height();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, frame_ref)

int rs2_get_frame_stride_in_bytes(const rs2_frame * frame_ref, rs2_error ** error) try
{
    VALIDATE_NOT_NULL(frame_ref);
    return frame_ref->get()->get_stride();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, frame_ref)


int rs2_get_frame_bits_per_pixel(const rs2_frame * frame_ref, rs2_error ** error) try
{
    VALIDATE_NOT_NULL(frame_ref);
    return frame_ref->get()->get_bpp();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, frame_ref)

rs2_format rs2_get_frame_format(const rs2_frame * frame_ref, rs2_error ** error) try
{
    VALIDATE_NOT_NULL(frame_ref);
    return frame_ref->get()->get_format();
}
HANDLE_EXCEPTIONS_AND_RETURN(RS2_FORMAT_ANY, frame_ref)

rs2_stream rs2_get_frame_stream_type(const rs2_frame * frame_ref, rs2_error ** error) try
{
    VALIDATE_NOT_NULL(frame_ref);
    return frame_ref->get()->get_stream_type();
}
HANDLE_EXCEPTIONS_AND_RETURN(RS2_STREAM_COUNT, frame_ref)


unsigned long long rs2_get_frame_number(const rs2_frame * frame, rs2_error ** error) try
{
    VALIDATE_NOT_NULL(frame);
    return frame->get()->get_frame_number();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, frame)

void rs2_release_frame(rs2_frame * frame) try
{
    VALIDATE_NOT_NULL(frame);
    frame->get()->get_owner()->release_frame_ref(frame);
}
NOEXCEPT_RETURN(, frame)

const char* rs2_get_option_description(const rs2_device* device, rs2_option option, rs2_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(option);
    return device->device->get_endpoint(device->subdevice).get_option(option).get_description();
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, device, option)

rs2_frame* rs2_clone_frame_ref(rs2_frame* frame, rs2_error ** error) try
{
    VALIDATE_NOT_NULL(frame);
    return frame->get()->get_owner()->clone_frame(frame);
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, frame)

const char* rs2_get_option_value_description(const rs2_device* device, rs2_option option, float value, rs2_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(option);
    return device->device->get_endpoint(device->subdevice).get_option(option).get_value_description(value);
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, device, option, value)

rs2_frame_queue* rs2_create_frame_queue(int capacity, rs2_error** error) try
{
    return new rs2_frame_queue(capacity);
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, capacity)

void rs2_delete_frame_queue(rs2_frame_queue* queue) try
{
    VALIDATE_NOT_NULL(queue);
    delete queue;
}
NOEXCEPT_RETURN(, queue)

rs2_frame* rs2_wait_for_frame(rs2_frame_queue* queue, rs2_error** error) try
{
    VALIDATE_NOT_NULL(queue);
    rsimpl2::frame_holder fh;
    if (!queue->queue.dequeue(&fh))
    {
        throw std::runtime_error("Frame did not arrive in time!");
    }

    rs2_frame* result = nullptr;
    std::swap(result, fh.frame);
    return result;
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, queue)

int rs2_poll_for_frame(rs2_frame_queue* queue, rs2_frame** output_frame, rs2_error** error) try
{
    VALIDATE_NOT_NULL(queue);
    VALIDATE_NOT_NULL(output_frame);
    rsimpl2::frame_holder fh;
    if (queue->queue.try_dequeue(&fh))
    {
        rs2_frame* result = nullptr;
        std::swap(result, fh.frame);
        *output_frame = result;
        return true;
    }

    return false;
}
HANDLE_EXCEPTIONS_AND_RETURN(0, queue, output_frame)

void rs2_enqueue_frame(rs2_frame* frame, void* queue) try
{
    VALIDATE_NOT_NULL(frame);
    VALIDATE_NOT_NULL(queue);
    auto q = reinterpret_cast<rs2_frame_queue*>(queue);
    rsimpl2::frame_holder fh;
    fh.frame = frame;
    q->queue.enqueue(std::move(fh));
}
NOEXCEPT_RETURN(, frame, queue)

void rs2_flush_queue(rs2_frame_queue* queue, rs2_error** error) try
{
    VALIDATE_NOT_NULL(queue);
    queue->queue.clear();
}
HANDLE_EXCEPTIONS_AND_RETURN(, queue)

void rs2_get_extrinsics(const rs2_device * from_dev, rs2_stream from_stream,
                        const rs2_device * to_dev, rs2_stream to_stream,
                        rs2_extrinsics * extrin, rs2_error ** error) try
{
    VALIDATE_NOT_NULL(from_dev);
    VALIDATE_NOT_NULL(to_dev);
    VALIDATE_NOT_NULL(extrin);
    VALIDATE_ENUM(from_stream);
    VALIDATE_ENUM(to_stream);

    if (from_dev->device != to_dev->device)
        throw rsimpl2::invalid_value_exception("Extrinsics between the selected devices are unknown!");

    *extrin = from_dev->device->get_extrinsics(from_dev->subdevice, from_stream, to_dev->subdevice, to_stream);
}
HANDLE_EXCEPTIONS_AND_RETURN(, from_dev, from_stream, to_dev, to_stream, extrin)

void rs2_get_motion_intrinsics(const rs2_device * device, rs2_stream stream, rs2_motion_device_intrinsic * intrinsics, rs2_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_NOT_NULL(intrinsics);
    VALIDATE_ENUM(stream);

    *intrinsics = device->device->get_motion_intrinsics(stream);
}
HANDLE_EXCEPTIONS_AND_RETURN(, device, stream, intrinsics)

void rs2_get_stream_intrinsics(const rs2_device * device, rs2_stream stream, int width, int height, int fps,
    rs2_format format, rs2_intrinsics * intrinsics, rs2_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(stream);
    VALIDATE_ENUM(format);
    VALIDATE_NOT_NULL(intrinsics);
    // cast because i've been getting errors. (int->uint32_t requires narrowing conversion)
    *intrinsics = device->device->get_intrinsics(device->subdevice, {stream, uint32_t(width), uint32_t(height), uint32_t(fps), format});
}
HANDLE_EXCEPTIONS_AND_RETURN(, device, intrinsics)


void rs2_hardware_reset(const rs2_device * device, rs2_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    device->device->hardware_reset();    
}
HANDLE_EXCEPTIONS_AND_RETURN(, device)

// Verify  and provide API version encoded as integer value
int rs2_get_api_version(rs2_error ** error) try
{
    // Each component type is within [0-99] range
    VALIDATE_RANGE(RS2_API_MAJOR_VERSION, 0, 99);
    VALIDATE_RANGE(RS2_API_MINOR_VERSION, 0, 99);
    VALIDATE_RANGE(RS2_API_PATCH_VERSION, 0, 99);
    return RS2_API_VERSION;
}
HANDLE_EXCEPTIONS_AND_RETURN(0, RS2_API_MAJOR_VERSION, RS2_API_MINOR_VERSION, RS2_API_PATCH_VERSION)

rs2_context* rs2_create_recording_context(int api_version, const char* filename, const char* section, rs2_recording_mode mode, rs2_error** error) try
{
    VALIDATE_NOT_NULL(filename);
    VALIDATE_NOT_NULL(section);
    verify_version_compatibility(api_version);

    return new rs2_context{ std::make_shared<rsimpl2::context>(rsimpl2::backend_type::record, filename, section, mode) };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, api_version, filename, section, mode)

rs2_context* rs2_create_mock_context(int api_version, const char* filename, const char* section, rs2_error** error) try
{
    VALIDATE_NOT_NULL(filename);
    VALIDATE_NOT_NULL(section);
    verify_version_compatibility(api_version);

    return new rs2_context{ std::make_shared<rsimpl2::context>(rsimpl2::backend_type::playback, filename, section) };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, api_version, filename, section)

void rs2_set_region_of_interest(const rs2_device* device, int min_x, int min_y, int max_x, int max_y, rs2_error ** error) try
{
    VALIDATE_NOT_NULL(device);

    VALIDATE_LE(min_x, max_x);
    VALIDATE_LE(min_y, max_y);
    VALIDATE_LE(0, min_x);
    VALIDATE_LE(0, min_y);

    device->device->get_endpoint(device->subdevice).get_roi_method().set({ min_x, min_y, max_x, max_y });
}
HANDLE_EXCEPTIONS_AND_RETURN(, min_x, min_y, max_x, max_y)

void rs2_get_region_of_interest(const rs2_device* device, int* min_x, int* min_y, int* max_x, int* max_y, rs2_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_NOT_NULL(min_x);
    VALIDATE_NOT_NULL(min_y);
    VALIDATE_NOT_NULL(max_x);
    VALIDATE_NOT_NULL(max_y);

    auto roi = device->device->get_endpoint(device->subdevice).get_roi_method().get();

    *min_x = roi.min_x;
    *min_y = roi.min_y;
    *max_x = roi.max_x;
    *max_y = roi.max_y;
}
HANDLE_EXCEPTIONS_AND_RETURN(, min_x, min_y, max_x, max_y)

rs2_syncer* rs2_create_syncer(const rs2_device* dev, rs2_error** error) try
{
    VALIDATE_NOT_NULL(dev);
    return new rs2_syncer { dev->device->create_syncer() };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, dev)

void rs2_start_syncer(const rs2_device* device, rs2_stream stream, rs2_syncer* syncer, rs2_error** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(stream);
    VALIDATE_NOT_NULL(syncer);
    rsimpl2::frame_callback_ptr callback(
        new rsimpl2::frame_callback(rs2_sync_frame, syncer));
    device->device->get_endpoint(device->subdevice).starter().start(stream, move(callback));
}
HANDLE_EXCEPTIONS_AND_RETURN(, device, stream, syncer)

void rs2_wait_for_frames(rs2_syncer* syncer, unsigned int timeout_ms, rs2_frame** output_array, rs2_error** error) try
{
    VALIDATE_NOT_NULL(syncer);
    VALIDATE_NOT_NULL(output_array);
    auto res = syncer->syncer->wait_for_frames(timeout_ms);
    for (uint32_t i = 0; i < RS2_STREAM_COUNT; i++)
    {
        output_array[i] = nullptr;
    }
    for (auto&& holder : res)
    {
        output_array[holder.frame->get()->get_stream_type()] = holder.frame;
        holder.frame = nullptr;
    }
}
HANDLE_EXCEPTIONS_AND_RETURN(, syncer, timeout_ms, output_array)

int rs2_poll_for_frames(rs2_syncer* syncer, rs2_frame** output_array, rs2_error** error) try
{
    VALIDATE_NOT_NULL(syncer);
    VALIDATE_NOT_NULL(output_array);
    rsimpl2::frameset res;
    if (syncer->syncer->poll_for_frames(res))
    {
        for (uint32_t i = 0; i < RS2_STREAM_COUNT; i++)
        {
            output_array[i] = nullptr;
        }
        for (auto&& holder : res)
        {
            output_array[holder.frame->get()->get_stream_type()] = holder.frame;
            holder.frame = nullptr;
        }
        return 1;
    }
    return 0;
}
HANDLE_EXCEPTIONS_AND_RETURN(0, syncer, output_array)

void rs2_sync_frame(rs2_frame* frame, void* syncer) try
{
    VALIDATE_NOT_NULL(frame);
    VALIDATE_NOT_NULL(syncer);
    ((rs2_syncer*)syncer)->syncer->dispatch_frame(frame);
}
NOEXCEPT_RETURN(, frame, syncer)

void rs2_delete_syncer(rs2_syncer* syncer) try
{
    VALIDATE_NOT_NULL(syncer);
    delete syncer;
}
NOEXCEPT_RETURN(, syncer)

void rs2_free_error(rs2_error * error) { if (error) delete error; }
const char * rs2_get_failed_function(const rs2_error * error) { return error ? error->function : nullptr; }
const char * rs2_get_failed_args(const rs2_error * error) { return error ? error->args.c_str() : nullptr; }
const char * rs2_get_error_message(const rs2_error * error) { return error ? error->message.c_str() : nullptr; }
rs2_exception_type rs2_get_librealsense_exception_type(const rs2_error * error) { return error ? error->exception_type : RS2_EXCEPTION_TYPE_UNKNOWN; }

const char * rs2_stream_to_string(rs2_stream stream) { return rsimpl2::get_string(stream); }
const char * rs2_format_to_string(rs2_format format) { return rsimpl2::get_string(format); }
const char * rs2_distortion_to_string(rs2_distortion distortion) { return rsimpl2::get_string(distortion); }
const char * rs2_option_to_string(rs2_option option) { return rsimpl2::get_string(option); }
const char * rs2_camera_info_to_string(rs2_camera_info info) { return rsimpl2::get_string(info); }
const char * rs2_timestamp_domain_to_string(rs2_timestamp_domain info){ return rsimpl2::get_string(info); }
const char * rs2_notification_category_to_string(rs2_notification_category category){ return rsimpl2::get_string(category); }
const char * rs2_visual_preset_to_string(rs2_visual_preset preset) { return rsimpl2::get_string(preset); }
const char * rs2_log_severity_to_string(rs2_log_severity severity) { return rsimpl2::get_string(severity); }
const char * rs2_exception_type_to_string(rs2_exception_type type) { return rsimpl2::get_string(type); }

void rs2_log_to_console(rs2_log_severity min_severity, rs2_error ** error) try
{
    rsimpl2::log_to_console(min_severity);
}
HANDLE_EXCEPTIONS_AND_RETURN(, min_severity)

void rs2_log_to_file(rs2_log_severity min_severity, const char * file_path, rs2_error ** error) try
{
    rsimpl2::log_to_file(min_severity, file_path);
}
HANDLE_EXCEPTIONS_AND_RETURN(, min_severity, file_path)
