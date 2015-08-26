#include "rs-internal.h"

#include "R200/R200.h"
#include "F200/F200.h"

rs_context::rs_context()
{
	uvc_error_t initStatus = uvc_init(&privateContext, NULL);
	if (initStatus < 0)
	{
		uvc_perror(initStatus, "uvc_init");
		throw std::runtime_error("Could not initialize UVC context");
    }

	QueryDeviceList();
}

rs_context::~rs_context()
{
	cameras.clear(); // tear down cameras before context
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

    if (privateContext) uvc_exit(privateContext);
}

void rs_context::QueryDeviceList()
{
	uvc_device_t **list;

	uvc_error_t status = uvc_get_device_list(privateContext, &list);
	if (status != UVC_SUCCESS)
	{
		uvc_perror(status, "uvc_get_device_list");
		return;
	}

	size_t index = 0;
	while (list[index] != nullptr)
	{
		auto dev = list[index];
		uvc_ref_device(dev);

		uvc_device_descriptor_t * desc;
		if (uvc_get_device_descriptor(dev, &desc) == UVC_SUCCESS)
		{
            switch(desc->idProduct)
            {
            case 2688: cameras.push_back(std::make_shared<rsimpl::r200::R200Camera>(privateContext, list[index])); break;
            case 2662: cameras.push_back(std::make_shared<rsimpl::f200::F200Camera>(privateContext, list[index])); break;
			}
			uvc_free_device_descriptor(desc);
		}

        ++index;
	}

	uvc_free_device_list(list, 1);
}

////////////////////////
// API implementation //
////////////////////////

// This facility allows for translation of exceptions to rs_error structs at the API boundary

static void translate_exception(const char * name, rs_error ** error)
{
    try { throw; }
    catch (const std::exception & e) { if (error) *error = new rs_error{ name, e.what() }; } // TODO: Handle case where THIS code throws
    catch (...) { if (error) *error = new rs_error{ name, "unknown error" }; } // TODO: Handle case where THIS code throws
}
#define HANDLE_EXCEPTIONS_AND_RETURN(...) catch(...) { translate_exception(__FUNCTION__, error); return __VA_ARGS__; }

rs_context * rs_create_context(int api_version, rs_error ** error) try
{
	if (api_version != RS_API_VERSION) throw std::runtime_error("api version mismatch");
	return new rs_context();
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr)

int	rs_get_camera_count(rs_context * context, rs_error ** error) try
{
    return (int)context->cameras.size();
}
HANDLE_EXCEPTIONS_AND_RETURN(0)

rs_camera * rs_get_camera(rs_context * context, int index, rs_error ** error) try
{
    return context->cameras[index].get();
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr)

void rs_delete_context(rs_context * context, rs_error ** error) try
{
    delete context;
}
HANDLE_EXCEPTIONS_AND_RETURN()

const char * rs_get_camera_name(rs_camera * camera, rs_error ** error) try
{
    return camera->GetCameraName();
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr)

void rs_enable_stream(struct rs_camera * camera, enum rs_stream stream, int width, int height, enum rs_format format, int fps, struct rs_error ** error) try
{
    camera->EnableStream(stream, width, height, fps, format);
}
HANDLE_EXCEPTIONS_AND_RETURN()

void rs_enable_stream_preset(struct rs_camera * camera, enum rs_stream stream, enum rs_preset preset, struct rs_error ** error) try
{
    camera->EnableStreamPreset(stream, preset);
}
HANDLE_EXCEPTIONS_AND_RETURN()

int rs_is_stream_enabled(struct rs_camera * camera, enum rs_stream stream, struct rs_error ** error) try
{
    return camera->IsStreamEnabled(stream);
}
HANDLE_EXCEPTIONS_AND_RETURN(0)

void rs_start_streaming(struct rs_camera * camera, struct rs_error ** error) try
{
    camera->StartStreaming();
}
HANDLE_EXCEPTIONS_AND_RETURN()

void rs_stop_streaming(struct rs_camera * camera, struct rs_error ** error) try
{
    camera->StopStreaming();
}
HANDLE_EXCEPTIONS_AND_RETURN()

void rs_wait_all_streams(struct rs_camera * camera, struct rs_error ** error) try
{
    camera->WaitAllStreams();
}
HANDLE_EXCEPTIONS_AND_RETURN()

const void * rs_get_image_pixels(rs_camera * camera, enum rs_stream stream, rs_error ** error) try
{
    return camera->GetImagePixels(stream);
}
HANDLE_EXCEPTIONS_AND_RETURN(nullptr)

float rs_get_depth_scale(rs_camera * camera, rs_error ** error) try
{
    return camera->GetDepthScale();
}
HANDLE_EXCEPTIONS_AND_RETURN(0.0f)

void rs_get_stream_intrinsics(struct rs_camera * camera, enum rs_stream stream, struct rs_intrinsics * intrin, struct rs_error ** error) try
{
	*intrin = camera->GetStreamIntrinsics(stream);
}
HANDLE_EXCEPTIONS_AND_RETURN()

void rs_get_stream_extrinsics(struct rs_camera * camera, enum rs_stream from, enum rs_stream to, struct rs_extrinsics * extrin, struct rs_error ** error) try
{
    *extrin = camera->GetStreamExtrinsics(from, to);
}
HANDLE_EXCEPTIONS_AND_RETURN()

const char * rs_get_failed_function(rs_error * error) { return error ? error->function.c_str() : nullptr; }
const char * rs_get_error_message(rs_error * error) { return error ? error->message.c_str() : nullptr; }
void rs_free_error(rs_error * error) { if (error) delete error; }
