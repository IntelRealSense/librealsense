#include "context.h"
#include "device.h"

#include <climits>

////////////////////////
// API implementation //
////////////////////////

struct rs_error
{
    std::string message;
    const char * function;
    std::string args;
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
        catch (const std::exception & e) { if (error) *error = new rs_error {e.what(), name, move(args)}; } // TODO: Handle case where THIS code throws
        catch (...) { if (error) *error = new rs_error {"unknown error", name, move(args)}; } // TODO: Handle case where THIS code throws
    }
}
#define HANDLE_EXCEPTIONS_AND_RETURN(R, ...) catch(...) { std::ostringstream ss; rsimpl::stream_args(ss, #__VA_ARGS__, __VA_ARGS__); rsimpl::translate_exception(__FUNCTION__, ss.str(), error); return R; }
#define VALIDATE_NOT_NULL(ARG) if(!ARG) throw std::runtime_error("null pointer passed for argument \"" #ARG "\"");
#define VALIDATE_ENUM(ARG) if(!rsimpl::is_valid(ARG)) { std::ostringstream ss; ss << "bad enum value for argument \"" #ARG "\""; throw std::runtime_error(ss.str()); }
#define VALIDATE_RANGE(ARG, MIN, MAX) if(ARG < MIN || ARG > MAX) { std::ostringstream ss; ss << "out of range value for argument \"" #ARG "\""; throw std::runtime_error(ss.str()); }

rs_context * rs_create_context(int api_version, rs_error ** error) try
{
    if (api_version != RS_API_VERSION) throw std::runtime_error("api version mismatch");
    return new rs_context();
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, api_version)

void rs_delete_context(rs_context * context, rs_error ** error) try
{
    VALIDATE_NOT_NULL(context);
    delete context;
}
HANDLE_EXCEPTIONS_AND_RETURN(, context)

int rs_get_device_count(const rs_context * context, rs_error ** error) try
{
    VALIDATE_NOT_NULL(context);
    return (int)context->devices.size();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, context)

rs_device * rs_get_device(const rs_context * context, int index, rs_error ** error) try
{
    VALIDATE_NOT_NULL(context);
    VALIDATE_RANGE(index, 0, (int)context->devices.size()-1);
    return context->devices[index].get();
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, context, index)



const char * rs_get_device_name(const rs_device * device, rs_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    return device->get_name();
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, device)

int rs_device_supports_option(const rs_device * device, rs_option option, rs_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(option);
    return device->supports_option(option);
}
HANDLE_EXCEPTIONS_AND_RETURN(0, device, option)

void rs_get_device_extrinsics(const rs_device * device, rs_stream from, rs_stream to, rs_extrinsics * extrin, rs_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(from);
    VALIDATE_ENUM(to);
    VALIDATE_NOT_NULL(extrin);
    *extrin = device->get_stream_extrinsics(from, to);
}
HANDLE_EXCEPTIONS_AND_RETURN(, device, from, to, extrin)



void rs_enable_stream(rs_device * device, rs_stream stream, int width, int height, rs_format format, int fps, rs_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(stream);
    VALIDATE_RANGE(width, 0, INT_MAX);
    VALIDATE_RANGE(height, 0, INT_MAX);
    VALIDATE_ENUM(format);
    VALIDATE_RANGE(fps, 0, INT_MAX);
    device->enable_stream(stream, width, height, format, fps);
}
HANDLE_EXCEPTIONS_AND_RETURN(, device, stream, width, height, format, fps)

void rs_enable_stream_preset(rs_device * device, rs_stream stream, rs_preset preset, rs_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(stream);
    VALIDATE_ENUM(preset);
    device->enable_stream_preset(stream, preset);
}
HANDLE_EXCEPTIONS_AND_RETURN(, device, stream, preset)

int rs_stream_is_enabled(const rs_device * device, rs_stream stream, rs_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(stream);
    return device->is_stream_enabled(stream);
}
HANDLE_EXCEPTIONS_AND_RETURN(0, device, stream)



void rs_start_device(rs_device * device, rs_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    device->start_capture();
}
HANDLE_EXCEPTIONS_AND_RETURN(, device)

void rs_stop_device(rs_device * device, rs_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    device->stop_capture();
}
HANDLE_EXCEPTIONS_AND_RETURN(, device)

int rs_device_is_streaming(const rs_device * device, rs_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    return device->is_capturing();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, device)

float rs_get_device_depth_scale(const rs_device * device, rs_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    return device->get_depth_scale();
}
HANDLE_EXCEPTIONS_AND_RETURN(0.0f, device)

rs_format rs_get_stream_format(const rs_device * device, rs_stream stream, rs_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(stream);
    return device->get_stream_format(stream);
}
HANDLE_EXCEPTIONS_AND_RETURN(RS_FORMAT_ANY, device, stream)

void rs_get_stream_intrinsics(const rs_device * device, rs_stream stream, rs_intrinsics * intrin, rs_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(stream);
    VALIDATE_NOT_NULL(intrin);
    *intrin = device->get_stream_intrinsics(stream);
}
HANDLE_EXCEPTIONS_AND_RETURN(, device, stream, intrin)



void rs_wait_for_frames(rs_device * device, int stream_bits, rs_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    device->wait_all_streams();
}
HANDLE_EXCEPTIONS_AND_RETURN(, device)

int rs_get_frame_number(const rs_device * device, rs_stream stream, rs_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(stream);
    return device->get_image_frame_number(stream);
}
HANDLE_EXCEPTIONS_AND_RETURN(0, device, stream)

const void * rs_get_frame_data(const rs_device * device, rs_stream stream, rs_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(stream);
    return device->get_image_pixels(stream);
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, device, stream)



void rs_set_device_option(rs_device * device, rs_option option, int value, rs_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(option);
    if(!device->supports_option(option)) throw std::runtime_error("option not supported by this device");
    device->set_option(option, value);
}
HANDLE_EXCEPTIONS_AND_RETURN(, device, option, value)

int rs_get_device_option(const rs_device * device, rs_option option, rs_error ** error) try
{
    VALIDATE_NOT_NULL(device);
    VALIDATE_ENUM(option);
    if(!device->supports_option(option)) throw std::runtime_error("option not supported by this device");
    return device->get_option(option);
}
HANDLE_EXCEPTIONS_AND_RETURN(0, device, option)



const char * rs_get_stream_name(rs_stream stream, rs_error ** error) try
{
    VALIDATE_ENUM(stream);
    return rsimpl::get_string(stream);
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, stream)

const char * rs_get_format_name(rs_format format, rs_error ** error) try
{
   VALIDATE_ENUM(format);
   return rsimpl::get_string(format);
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, format)

const char * rs_get_preset_name(rs_preset preset, rs_error ** error) try
{
    VALIDATE_ENUM(preset);
    return rsimpl::get_string(preset);
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, preset)

const char * rs_get_distortion_name(rs_distortion distortion, rs_error ** error) try
{
   VALIDATE_ENUM(distortion);
   return rsimpl::get_string(distortion);
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, distortion)

const char * rs_get_option_name(rs_option option, rs_error ** error) try
{
    VALIDATE_ENUM(option);
    return rsimpl::get_string(option);
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, option)



void rs_free_error(rs_error * error) { if (error) delete error; }
const char * rs_get_failed_function(const rs_error * error) { return error ? error->function : nullptr; }
const char * rs_get_failed_args(const rs_error * error) { return error ? error->args.c_str() : nullptr; }
const char * rs_get_error_message(const rs_error * error) { return error ? error->message.c_str() : nullptr; }



const char * rs_stream_to_string(rs_stream stream) { return rsimpl::get_string(stream); }
const char * rs_format_to_string(rs_format format) { return rsimpl::get_string(format); }
const char * rs_preset_to_string(rs_preset preset) { return rsimpl::get_string(preset); }
const char * rs_distortion_to_string(rs_distortion distortion) { return rsimpl::get_string(distortion); }
const char * rs_option_to_string(rs_option option) { return rsimpl::get_string(option); }
