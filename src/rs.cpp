#include "context.h"
#include "camera.h"

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

int rs_get_camera_count(rs_context * context, rs_error ** error) try
{
    VALIDATE_NOT_NULL(context);
    return (int)context->cameras.size();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, context)

rs_camera * rs_get_camera(rs_context * context, int index, rs_error ** error) try
{
    VALIDATE_NOT_NULL(context);
    VALIDATE_RANGE(index, 0, (int)context->cameras.size()-1);
    return context->cameras[index].get();
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, context, index)

void rs_delete_context(rs_context * context, rs_error ** error) try
{
    VALIDATE_NOT_NULL(context);
    delete context;
}
HANDLE_EXCEPTIONS_AND_RETURN(, context)

const char * rs_get_camera_name(rs_camera * camera, rs_error ** error) try
{
    VALIDATE_NOT_NULL(camera);
    return camera->get_name();
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, camera)

void rs_enable_stream(rs_camera * camera, rs_stream stream, int width, int height, rs_format format, int fps, rs_error ** error) try
{
    VALIDATE_NOT_NULL(camera);
    VALIDATE_ENUM(stream);
    VALIDATE_RANGE(width, 0, INT_MAX);
    VALIDATE_RANGE(height, 0, INT_MAX);
    VALIDATE_ENUM(format);
    VALIDATE_RANGE(fps, 0, INT_MAX);
    camera->enable_stream(stream, width, height, format, fps);
}
HANDLE_EXCEPTIONS_AND_RETURN(, camera, stream, width, height, format, fps)

void rs_enable_stream_preset(rs_camera * camera, rs_stream stream, rs_preset preset, rs_error ** error) try
{
    VALIDATE_NOT_NULL(camera);
    VALIDATE_ENUM(stream);
    VALIDATE_ENUM(preset);
    camera->enable_stream_preset(stream, preset);
}
HANDLE_EXCEPTIONS_AND_RETURN(, camera, stream, preset)

int rs_is_stream_enabled(rs_camera * camera, rs_stream stream, rs_error ** error) try
{
    VALIDATE_NOT_NULL(camera);
    VALIDATE_ENUM(stream);
    return camera->is_stream_enabled(stream);
}
HANDLE_EXCEPTIONS_AND_RETURN(0, camera, stream)

void rs_start_capture(rs_camera * camera, rs_error ** error) try
{
    VALIDATE_NOT_NULL(camera);
    camera->start_capture();
}
HANDLE_EXCEPTIONS_AND_RETURN(, camera)

void rs_stop_capture(rs_camera * camera, rs_error ** error) try
{
    VALIDATE_NOT_NULL(camera);
    camera->stop_capture();
}
HANDLE_EXCEPTIONS_AND_RETURN(, camera)

int rs_is_capturing(rs_camera * camera, rs_error ** error) try
{
    VALIDATE_NOT_NULL(camera);
    return camera->is_streaming();
}
HANDLE_EXCEPTIONS_AND_RETURN(0, camera)

void rs_wait_all_streams(rs_camera * camera, rs_error ** error) try
{
    VALIDATE_NOT_NULL(camera);
    camera->wait_all_streams();
}
HANDLE_EXCEPTIONS_AND_RETURN(, camera)

float rs_get_depth_scale(rs_camera * camera, rs_error ** error) try
{
    VALIDATE_NOT_NULL(camera);
    return camera->get_depth_scale();
}
HANDLE_EXCEPTIONS_AND_RETURN(0.0f, camera)

rs_format rs_get_stream_format(rs_camera * camera, rs_stream stream, rs_error ** error) try
{
    VALIDATE_NOT_NULL(camera);
    VALIDATE_ENUM(stream);
    return camera->get_image_format(stream);
}
HANDLE_EXCEPTIONS_AND_RETURN(RS_FORMAT_ANY, camera, stream)

void rs_get_stream_intrinsics(rs_camera * camera, rs_stream stream, rs_intrinsics * intrin, rs_error ** error) try
{
    VALIDATE_NOT_NULL(camera);
    VALIDATE_ENUM(stream);
    VALIDATE_NOT_NULL(intrin);
    *intrin = camera->get_stream_intrinsics(stream);
}
HANDLE_EXCEPTIONS_AND_RETURN(, camera, stream, intrin)

void rs_get_stream_extrinsics(rs_camera * camera, rs_stream from, rs_stream to, rs_extrinsics * extrin, rs_error ** error) try
{
    VALIDATE_NOT_NULL(camera);
    VALIDATE_ENUM(from);
    VALIDATE_ENUM(to);
    VALIDATE_NOT_NULL(extrin);
    *extrin = camera->get_stream_extrinsics(from, to);
}
HANDLE_EXCEPTIONS_AND_RETURN(, camera, from, to, extrin)

const void * rs_get_image_pixels(rs_camera * camera, rs_stream stream, rs_error ** error) try
{
    VALIDATE_NOT_NULL(camera);
    VALIDATE_ENUM(stream);
    return camera->get_image_pixels(stream);
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr, camera, stream)

int rs_get_image_frame_number(rs_camera * camera, rs_stream stream, rs_error ** error) try
{
    VALIDATE_NOT_NULL(camera);
    VALIDATE_ENUM(stream);
    return camera->get_image_frame_number(stream);
}
HANDLE_EXCEPTIONS_AND_RETURN(0, camera, stream)

int rs_camera_supports_option(rs_camera * camera, rs_option option, rs_error ** error) try
{
    VALIDATE_NOT_NULL(camera);
    VALIDATE_ENUM(option);
    return camera->supports_option(option);
}
HANDLE_EXCEPTIONS_AND_RETURN(0, camera, option)

int rs_get_camera_option(rs_camera * camera, rs_option option, rs_error ** error) try
{
    VALIDATE_NOT_NULL(camera);
    VALIDATE_ENUM(option);
    if(!camera->supports_option(option)) throw std::runtime_error("option not supported by this camera");
    return camera->get_option(option);
}
HANDLE_EXCEPTIONS_AND_RETURN(0, camera, option)

void rs_set_camera_option(rs_camera * camera, rs_option option, int value, rs_error ** error) try
{
    VALIDATE_NOT_NULL(camera);
    VALIDATE_ENUM(option);
    if(!camera->supports_option(option)) throw std::runtime_error("option not supported by this camera");
    camera->set_option(option, value);
}
HANDLE_EXCEPTIONS_AND_RETURN(, camera, option, value)

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

const char * rs_get_failed_function(rs_error * error) { return error ? error->function : nullptr; }
const char * rs_get_failed_args(rs_error * error) { return error ? error->args.c_str() : nullptr; }
const char * rs_get_error_message(rs_error * error) { return error ? error->message.c_str() : nullptr; }
void rs_free_error(rs_error * error) { if (error) delete error; }
