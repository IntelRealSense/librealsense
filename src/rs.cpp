// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <functional>   // For function
#include <climits>

#include "context.h"
#include "device.h"
#include "archive.h"
#include "concurrency.h"

////////////////////////
// API implementation //
////////////////////////

struct rs_error
{
    std::string message;
    const char * function;
    std::string args;
};

struct rs_device_info
{
    std::shared_ptr<rsimpl::device_info> info;
};

struct rs_raw_data_buffer
{
    std::vector<uint8_t> buffer;
};

struct rs_stream_profile_list
{
    std::vector<rsimpl::stream_request> list;
};

struct rs_active_stream
{
    std::shared_ptr<rsimpl::streaming_lock> lock;
};

struct rs_device
{
    std::shared_ptr<rsimpl::device> device;

};

struct rs_context
{
    std::shared_ptr<rsimpl::context> ctx;
};

struct rs_device_list
{
    std::shared_ptr<rsimpl::context> ctx;
    std::vector<std::shared_ptr<rsimpl::device_info>> list;
};

struct frame_holder
{
    rs_frame* frame;

    ~frame_holder()
    {
        if (frame) rs_release_frame(frame);
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

struct rs_frame_queue
{
    explicit rs_frame_queue(int cap)
        : queue(cap)
    {
    }

    single_consumer_queue<frame_holder> queue;
};

// This facility allows for translation of exceptions to rs_error structs at the API boundary
namespace rsimpl
{
    template<class T> void stream_args(std::ostream & out, const char * names, const T & last) { out << names << ':' << last; }
    template<class T, class... U> void stream_args(std::ostream & out, const char * names, const T & first, const U &... rest)
    {
        while(*names && *names != ',') out << *names++;
        out << ':' << first << ", ";
        while(*names && (*names == ',' || isspace(*names))) ++names;
        stream_args(out, names, rest...);
    }

    static void translate_exception(const char * name, std::string args, rs_error ** error)
    {
        try { throw; }
        catch (const std::exception & e) { if (error) *error = new rs_error {e.what(), name, move(args)}; } // todo - Handle case where THIS code throws
        catch (...) { if (error) *error = new rs_error {"unknown error", name, move(args)}; } // todo - Handle case where THIS code throws
    }
}

#define NOEXCEPT_RETURN(R, ...) catch(...) { std::ostringstream ss; rsimpl::stream_args(ss, #__VA_ARGS__, __VA_ARGS__); rs_error* e; rsimpl::translate_exception(__FUNCTION__, ss.str(), &e); LOG_WARNING(rs_get_error_message(e)); rs_free_error(e); return R; }
#define HANDLE_EXCEPTIONS_AND_RETURN(R, ...) catch(...) { std::ostringstream ss; rsimpl::stream_args(ss, #__VA_ARGS__, __VA_ARGS__); rsimpl::translate_exception(__FUNCTION__, ss.str(), error); return R; }
#define VALIDATE_NOT_NULL(ARG) if(!(ARG)) throw std::runtime_error("null pointer passed for argument \"" #ARG "\"");
#define VALIDATE_ENUM(ARG) if(!rsimpl::is_valid(ARG)) { std::ostringstream ss; ss << "bad enum value for argument \"" #ARG "\""; throw std::runtime_error(ss.str()); }
#define VALIDATE_RANGE(ARG, MIN, MAX) if((ARG) < (MIN) || (ARG) > (MAX)) { std::ostringstream ss; ss << "out of range value for argument \"" #ARG "\""; throw std::runtime_error(ss.str()); }
#define VALIDATE_LE(ARG, MAX) if((ARG) > (MAX)) { std::ostringstream ss; ss << "out of range value for argument \"" #ARG "\""; throw std::runtime_error(ss.str()); }
#define VALIDATE_NATIVE_STREAM(ARG) VALIDATE_ENUM(ARG); if(ARG >= RS_STREAM_NATIVE_COUNT) { std::ostringstream ss; ss << "argument \"" #ARG "\" must be a native stream"; throw std::runtime_error(ss.str()); }

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
    if (major(version) == 0) return rsimpl::to_string() << version;
    return rsimpl::to_string() << major(version) << "." << minor(version) << "." << patch(version);
}

void report_version_mismatch(int runtime, int compiletime)
{
    throw std::runtime_error(rsimpl::to_string() << "API version mismatch: librealsense.so was compiled with API version " 
        << api_version_to_string(runtime) << " but the application was compiled with " 
        << api_version_to_string(compiletime) << "! Make sure correct version of the library is installed (make install)");
}

void verify_version_compatibility(int api_version)
{
    rs_error* error = nullptr;
    auto runtime_api_version = rs_get_api_version(&error);
    if (error) throw std::runtime_error(rs_get_error_message(error));

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

rs_context * rs_create_context(int api_version, rs_error ** error) try
{
    verify_version_compatibility(api_version);
    
    return new rs_context{ std::make_shared<rsimpl::context>(rsimpl::backend_type::standard) };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, api_version)

void rs_delete_context(rs_context * context) try
{
    VALIDATE_NOT_NULL(context);
    delete context;
}
NOEXCEPT_RETURN(, context)

rs_device_list* rs_query_devices(const rs_context* context, rs_error** error) try
{
    VALIDATE_NOT_NULL(context);
    return new rs_device_list{ context->ctx, context->ctx->query_devices() };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, context)

int rs_get_device_count(const rs_device_list* list, rs_error** error) try
{
    VALIDATE_NOT_NULL(list);
    return static_cast<int>(list->list.size());
}
HANDLE_EXCEPTIONS_AND_RETURN(0, list)

void rs_delete_device_list(rs_device_list* list) try
{
    VALIDATE_NOT_NULL(list);
    delete list;
}
catch (...) {}

rs_device* rs_create_device(const rs_device_list* list, int index, rs_error** error) try
{
    VALIDATE_NOT_NULL(list);
    VALIDATE_RANGE(index, 0, (int)list->list.size() - 1);
    return new rs_device{ list->list[index]->create(list->ctx->get_backend()) };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, list, index)

void rs_delete_device(rs_device* device) try
{
    VALIDATE_NOT_NULL(device);
    delete device;
}
NOEXCEPT_RETURN(, device)

rs_stream_modes_list* rs_get_stream_modes(rs_device* device, rs_error** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(subdevice);
    return new rs_stream_modes_list{ device->device->get_endpoint(subdevice).get_principal_requests() };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, device)

void rs_get_stream_mode(const rs_stream_modes_list* list, int index, rs_stream* stream, int* width, int* height, int* fps, rs_format* format, rs_error** error) try
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

int rs_get_modes_count(const rs_stream_modes_list* list, rs_error** error) try
{
    VALIDATE_NOT_NULL(list);
    return static_cast<int>(list->list.size());
}
HANDLE_EXCEPTIONS_AND_RETURN(0, list)

void rs_delete_modes_list(rs_stream_modes_list* list) try
{
    VALIDATE_NOT_NULL(list);
    delete list;
}
NOEXCEPT_RETURN(, list)

rs_raw_data_buffer* rs_send_and_receive_raw_data(rs_device* device, void* raw_data_to_send, unsigned size_of_raw_data_to_send, rs_error** error) try
{
    VALIDATE_NOT_NULL(device);
    auto raw_data_buffer = static_cast<uint8_t*>(raw_data_to_send);
    std::vector<uint8_t> buffer_to_send(raw_data_buffer, raw_data_buffer + size_of_raw_data_to_send);
    auto ret_data = device->device->send_receive_raw_data(buffer_to_send);
    return new rs_raw_data_buffer{ ret_data };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, device)

const unsigned char* rs_get_raw_data(const rs_raw_data_buffer* buffer, rs_error** error) try
{
    VALIDATE_NOT_NULL(buffer);
    return buffer->buffer.data();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, buffer)

int rs_get_raw_data_size(const rs_raw_data_buffer* buffer, rs_error** error) try
{
    VALIDATE_NOT_NULL(buffer);
    return static_cast<int>(buffer->buffer.size());
}
HANDLE_EXCEPTIONS_AND_RETURN(0, buffer)

void rs_delete_raw_data(rs_raw_data_buffer* buffer) try
{
    VALIDATE_NOT_NULL(buffer);
    delete buffer;
}
NOEXCEPT_RETURN(, buffer)

rs_streaming_lock* rs_open(rs_device* device, rs_stream stream, 
    int width, int height, int fps, rs_format format, rs_error** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(subdevice);
    VALIDATE_ENUM(format);
    VALIDATE_ENUM(stream);

    std::vector<rsimpl::stream_request> request;
    request.push_back({ stream, static_cast<uint32_t>(width),
            static_cast<uint32_t>(height), static_cast<uint32_t>(fps), format });
    auto result = new rs_streaming_lock{ device->device->get_endpoint(subdevice).configure(request) };
    result->lock->set_owner(result);
    return result;
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, device, stream, width, height, fps, format)

rs_streaming_lock* rs_open_many(rs_device* device, 
    const rs_stream* stream, const int* width, const int* height, const int* fps, 
    const rs_format* format, int count, rs_error** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_NOT_NULL(stream);
    VALIDATE_NOT_NULL(width);
    VALIDATE_NOT_NULL(height);
    VALIDATE_NOT_NULL(fps);
    VALIDATE_NOT_NULL(format);

    std::vector<rsimpl::stream_request> request;
    for (auto i = 0; i < count; i++)
    {
        request.push_back({ stream[i], static_cast<uint32_t>(width[i]), 
                            static_cast<uint32_t>(height[i]), static_cast<uint32_t>(fps[i]), format[i] });
    }
    auto result = new rs_streaming_lock{ device->device->get_endpoint(subdevice).configure(request) };
    result->lock->set_owner(result);
    return result;
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, device, stream, width, height, fps, format)

void rs_close(rs_streaming_lock* lock) try
{
    VALIDATE_NOT_NULL(lock);
    delete lock;
}
NOEXCEPT_RETURN(, lock)

float rs_get_option(const rs_device* device, rs_option option, rs_error** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(option);
    return device->device->get_option(subdevice, option).query();
}
HANDLE_EXCEPTIONS_AND_RETURN(0.0f, device, option)

void rs_set_option(const rs_device* device, rs_option option, float value, rs_error** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(option);
    device->device->get_option(subdevice, option).set(value);
}
HANDLE_EXCEPTIONS_AND_RETURN(, device, option, value)

int rs_supports_option(const rs_device* device, rs_option option, rs_error** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(option);
    return device->device->supports_option(subdevice, option);
}
HANDLE_EXCEPTIONS_AND_RETURN(0, device, option)

void rs_get_option_range(const rs_device* device, rs_option option, 
    float* min, float* max, float* step, float* def, rs_error** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(option);
    VALIDATE_NOT_NULL(min);
    VALIDATE_NOT_NULL(max);
    VALIDATE_NOT_NULL(step);
    VALIDATE_NOT_NULL(def);
    auto range = device->device->get_option(subdevice, option).get_range();
    *min = range.min;
    *max = range.max;
    *def = range.def;
    *step = range.step;
}
HANDLE_EXCEPTIONS_AND_RETURN(, device, option, min, max, step, def)

const char* rs_get_camera_info(const rs_device* device, rs_camera_info info, rs_error** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(info);
    return device->device->get_info(info).c_str();
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, device, info)

int rs_supports_camera_info(const rs_device* device, rs_camera_info info, rs_error** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(info);
    return device->device->supports_info(info);
}
HANDLE_EXCEPTIONS_AND_RETURN(false, device, info)


void rs_start(rs_streaming_lock* lock, rs_frame_callback_ptr on_frame, void * user, rs_error ** error) try
{
    VALIDATE_NOT_NULL(lock);
    VALIDATE_NOT_NULL(on_frame);
    rsimpl::frame_callback_ptr callback(
        new rsimpl::frame_callback(lock, on_frame, user), 
        [](rs_frame_callback* p) { delete p; });
    lock->lock->play(std::move(callback));
}
HANDLE_EXCEPTIONS_AND_RETURN(, lock, on_frame, user)

void rs_start_cpp(rs_streaming_lock* lock, rs_frame_callback * callback, rs_error ** error) try
{
    VALIDATE_NOT_NULL(lock);
    VALIDATE_NOT_NULL(callback);
    lock->lock->play({ callback, [](rs_frame_callback* p) { p->release(); } });
}
HANDLE_EXCEPTIONS_AND_RETURN(, lock, callback)

void rs_stop(rs_streaming_lock* lock, rs_error ** error) try
{
    VALIDATE_NOT_NULL(lock);
    lock->lock->stop();
}
HANDLE_EXCEPTIONS_AND_RETURN(, lock)

double rs_get_frame_metadata(const rs_frame * frame, rs_frame_metadata frame_metadata, rs_error ** error) try
{
    VALIDATE_NOT_NULL(frame);
    return frame->get()->get_frame_metadata(frame_metadata);
}
HANDLE_EXCEPTIONS_AND_RETURN(0, frame)

int rs_supports_frame_metadata(const rs_frame * frame, rs_frame_metadata frame_metadata, rs_error ** error) try
{
    VALIDATE_NOT_NULL(frame);
    return frame->get()->supports_frame_metadata(frame_metadata);
}
HANDLE_EXCEPTIONS_AND_RETURN(0, frame)

double rs_get_frame_timestamp(const rs_frame * frame_ref, rs_error ** error) try
{
    VALIDATE_NOT_NULL(frame_ref);
    return frame_ref->get()->get_frame_timestamp();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, frame_ref)

rs_timestamp_domain rs_get_frame_timestamp_domain(const rs_frame * frame_ref, rs_error ** error) try
{
    VALIDATE_NOT_NULL(frame_ref);
    return frame_ref->get()->get_frame_timestamp_domain();
}
HANDLE_EXCEPTIONS_AND_RETURN(RS_TIMESTAMP_DOMAIN_COUNT, frame_ref)

const void * rs_get_frame_data(const rs_frame * frame_ref, rs_error ** error) try
{
    VALIDATE_NOT_NULL(frame_ref);
    return frame_ref->get()->get_frame_data();
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, frame_ref)

int rs_get_frame_width(const rs_frame * frame_ref, rs_error ** error) try
{
    VALIDATE_NOT_NULL(frame_ref);
    return frame_ref->get()->get_width();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, frame_ref)

int rs_get_frame_height(const rs_frame * frame_ref, rs_error ** error) try
{
    VALIDATE_NOT_NULL(frame_ref);
    return frame_ref->get()->get_height();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, frame_ref)

int rs_get_frame_stride_in_bytes(const rs_frame * frame_ref, rs_error ** error) try
{
    VALIDATE_NOT_NULL(frame_ref);
    return frame_ref->get()->get_stride();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, frame_ref)


int rs_get_frame_bits_per_pixel(const rs_frame * frame_ref, rs_error ** error) try
{
    VALIDATE_NOT_NULL(frame_ref);
    return frame_ref->get()->get_bpp();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, frame_ref)

rs_format rs_get_frame_format(const rs_frame * frame_ref, rs_error ** error) try
{
    VALIDATE_NOT_NULL(frame_ref);
    return frame_ref->get()->get_format();
}
HANDLE_EXCEPTIONS_AND_RETURN(RS_FORMAT_ANY, frame_ref)

rs_stream rs_get_frame_stream_type(const rs_frame * frame_ref, rs_error ** error) try
{
    VALIDATE_NOT_NULL(frame_ref);
    return frame_ref->get()->get_stream_type();
}
HANDLE_EXCEPTIONS_AND_RETURN(RS_STREAM_COUNT, frame_ref)


unsigned long long rs_get_frame_number(const rs_frame * frame, rs_error ** error) try
{
    VALIDATE_NOT_NULL(frame);
    return frame->get()->get_frame_number();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, frame)

void rs_release_frame(rs_frame * frame) try
{
    VALIDATE_NOT_NULL(frame);
    frame->get()->get_owner()->release_frame_ref(frame);
}
NOEXCEPT_RETURN(, frame)

const char* rs_get_subdevice_option_description(const rs_device* device, rs_option option, rs_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(option);
    return device->device->get_option(subdevice, option).get_description();
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, device, option)

rs_frame* rs_clone_frame_ref(rs_frame* frame, rs_error ** error) try
{
    VALIDATE_NOT_NULL(frame);
    return frame->get()->get_owner()->clone_frame(frame);
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, frame)

const char* rs_get_subdevice_option_value_description(const rs_device* device, rs_option option, float value, rs_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(option);
    return device->device->get_option(subdevice, option).get_value_description(value);
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, device, option, value)

rs_frame_queue* rs_create_frame_queue(int capacity, rs_error** error) try
{
    return new rs_frame_queue(capacity);
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, capacity)

void rs_delete_frame_queue(rs_frame_queue* queue) try
{
    VALIDATE_NOT_NULL(queue);
    delete queue;
}
NOEXCEPT_RETURN(, queue)

rs_frame* rs_wait_for_frame(rs_frame_queue* queue, rs_error** error) try
{
    VALIDATE_NOT_NULL(queue);
    auto frame_holder = queue->queue.dequeue();
    rs_frame* result = nullptr;
    std::swap(result, frame_holder.frame);
    return result;
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, queue)

int rs_poll_for_frame(rs_frame_queue* queue, rs_frame** output_frame, rs_error** error) try
{
    VALIDATE_NOT_NULL(queue);
    VALIDATE_NOT_NULL(output_frame);
    frame_holder fh;
    if (queue->queue.try_dequeue(&fh))
    {
        rs_frame* result = nullptr;
        std::swap(result, fh.frame);
        *output_frame = result;
        return true;
    }

    return false;
}
HANDLE_EXCEPTIONS_AND_RETURN(0, queue, output_frame)

void rs_enqueue_frame(rs_frame* frame, void* queue) try
{
    VALIDATE_NOT_NULL(frame);
    VALIDATE_NOT_NULL(queue);
    auto q = reinterpret_cast<rs_frame_queue*>(queue);
    frame_holder fh;
    fh.frame = frame;
    q->queue.enqueue(std::move(fh));
}
NOEXCEPT_RETURN(, frame, queue)

void rs_flush_queue(rs_frame_queue* queue, rs_error** error) try
{
    VALIDATE_NOT_NULL(queue);
    queue->queue.flush();
}
HANDLE_EXCEPTIONS_AND_RETURN(, queue)

void rs_get_device_extrinsics(const rs_device * from, const rs_device * to, rs_extrinsics * extrin, rs_error ** error) try
{
    VALIDATE_NOT_NULL(from);
    VALIDATE_NOT_NULL(to);
    VALIDATE_NOT_NULL(extrin);
    *extrin = device->device->get_extrinsics(from, to);
}
HANDLE_EXCEPTIONS_AND_RETURN(, from, to, extrin)

void rs_get_stream_intrinsics(const rs_device * device, rs_stream stream, int width, int height, int fps, 
    rs_format format, rs_intrinsics * intrinsics, rs_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(stream);
    VALIDATE_ENUM(format);
    VALIDATE_NOT_NULL(intrinsics);
    // cast because i've been getting errors. (int->uint32_t requires narrowing conversion)
    *intrinsics = device->device->get_intrinsics(subdevice, {stream, uint32_t(width), uint32_t(height), uint32_t(fps), format});
}
HANDLE_EXCEPTIONS_AND_RETURN(, device, intrinsics)

float rs_get_device_depth_scale(const rs_device * device, rs_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    return device->device->get_depth_scale();
}
HANDLE_EXCEPTIONS_AND_RETURN(0.0f, device)

// Verify  and provide API version encoded as integer value
int rs_get_api_version(rs_error ** error) try
{
    // Each component type is within [0-99] range
    VALIDATE_RANGE(RS_API_MAJOR_VERSION, 0, 99);
    VALIDATE_RANGE(RS_API_MINOR_VERSION, 0, 99);
    VALIDATE_RANGE(RS_API_PATCH_VERSION, 0, 99);
    return RS_API_VERSION;
}
HANDLE_EXCEPTIONS_AND_RETURN(0, RS_API_MAJOR_VERSION, RS_API_MINOR_VERSION, RS_API_PATCH_VERSION)

rs_context* rs_create_recording_context(int api_version, const char* filename, const char* section, rs_recording_mode mode, rs_error** error) try
{
    VALIDATE_NOT_NULL(filename);
    VALIDATE_NOT_NULL(section);
    verify_version_compatibility(api_version);

    return new rs_context{ std::make_shared<rsimpl::context>(rsimpl::backend_type::record, filename, section, mode) };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, api_version, filename, section, mode)

rs_context* rs_create_mock_context(int api_version, const char* filename, const char* section, rs_error** error) try
{
    VALIDATE_NOT_NULL(filename);
    VALIDATE_NOT_NULL(section);
    verify_version_compatibility(api_version);

    return new rs_context{ std::make_shared<rsimpl::context>(rsimpl::backend_type::playback, filename, section) };
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, api_version, filename, section)

void rs_free_error(rs_error * error) { if (error) delete error; }
const char * rs_get_failed_function(const rs_error * error) { return error ? error->function : nullptr; }
const char * rs_get_failed_args(const rs_error * error) { return error ? error->args.c_str() : nullptr; }
const char * rs_get_error_message(const rs_error * error) { return error ? error->message.c_str() : nullptr; }


const char * rs_stream_to_string(rs_stream stream) { return rsimpl::get_string(stream); }
const char * rs_format_to_string(rs_format format) { return rsimpl::get_string(format); }
const char * rs_distortion_to_string(rs_distortion distortion) { return rsimpl::get_string(distortion); }
const char * rs_option_to_string(rs_option option) { return rsimpl::get_string(option); }
const char * rs_camera_info_to_string(rs_camera_info info) { return rsimpl::get_string(info); }
const char * rs_timestamp_domain_to_string(rs_timestamp_domain info){ return rsimpl::get_string(info); }
const char * rs_visual_preset_to_string(rs_visual_preset preset) { return rsimpl::get_string(preset); }

void rs_log_to_console(rs_log_severity min_severity, rs_error ** error) try
{
    rsimpl::log_to_console(min_severity);
}
HANDLE_EXCEPTIONS_AND_RETURN(, min_severity)

void rs_log_to_file(rs_log_severity min_severity, const char * file_path, rs_error ** error) try
{
    rsimpl::log_to_file(min_severity, file_path);
}
HANDLE_EXCEPTIONS_AND_RETURN(, min_severity, file_path)
