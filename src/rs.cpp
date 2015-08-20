#include "rs-internal.h"

#include "R200/R200.h"
#include "F200/F200.h"
#include "DsCamera.h"

rs_context::rs_context()
{
    #ifdef USE_UVC_DEVICES
	uvc_error_t initStatus = uvc_init(&privateContext, NULL);
	if (initStatus < 0)
	{
		uvc_perror(initStatus, "uvc_init");
		throw std::runtime_error("Could not initialize UVC context");
	}
    #endif

	QueryDeviceList();
}

rs_context::~rs_context()
{
	cameras.clear(); // tear down cameras before context
	std::this_thread::sleep_for(std::chrono::milliseconds(500));

    #ifdef USE_UVC_DEVICES
    if (privateContext) uvc_exit(privateContext);
    #endif
}

void rs_context::QueryDeviceList()
{
    #ifdef USE_UVC_DEVICES
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
            case 2688: cameras.push_back(std::make_shared<r200::R200Camera>(privateContext, list[index])); break;
            case 2662: cameras.push_back(std::make_shared<f200::F200Camera>(privateContext, list[index])); break;
			}
			uvc_free_device_descriptor(desc);
		}

        ++index;
	}

	uvc_free_device_list(list, 1);
    #endif

    #ifdef USE_DSAPI_DEVICES
	int n = DSGetNumberOfCameras(true);
	for (int i = 0; i < n; ++i)
	{
		uint32_t serialNo = DSGetCameraSerialNumber(i);
		auto ds = DSCreate(DS_DS4_PLATFORM, serialNo);
		if (ds) cameras.push_back(std::make_shared<DsCamera>(ds, i));
	}
    #endif
}

////////////////////////
// API implementation //
////////////////////////

rs_context * rs_create_context(int api_version, rs_error ** error) 
{
    BEGIN_EXCEPTION_FIREWALL
	if (api_version != RS_API_VERSION) throw std::runtime_error("api version mismatch");
	return new rs_context();
    END_EXCEPTION_FIREWALL
}

int	rs_get_camera_count(rs_context * context, rs_error ** error)
{
    BEGIN_EXCEPTION_FIREWALL
    return (int)context->cameras.size();
    END_EXCEPTION_FIREWALL
}

rs_camera * rs_get_camera(rs_context * context, int index, rs_error ** error)
{
    BEGIN_EXCEPTION_FIREWALL
    return context->cameras[index].get();
    END_EXCEPTION_FIREWALL
}

void rs_delete_context(rs_context * context, rs_error ** error)
{
    BEGIN_EXCEPTION_FIREWALL
    delete context;
    END_EXCEPTION_FIREWALL
}

const char * rs_get_camera_name(rs_camera * camera, rs_error ** error)
{
    BEGIN_EXCEPTION_FIREWALL
    return camera->cameraName.c_str();
    END_EXCEPTION_FIREWALL
}

void rs_enable_stream(struct rs_camera * camera, int stream, int width, int height, int fps, int format, struct rs_error ** error)
{
    BEGIN_EXCEPTION_FIREWALL
    camera->EnableStream(stream, width, height, fps, format);
    END_EXCEPTION_FIREWALL
}

void rs_enable_stream_preset(struct rs_camera * camera, int stream, int preset, struct rs_error ** error)
{
    BEGIN_EXCEPTION_FIREWALL
    camera->EnableStreamPreset(stream, preset);
    END_EXCEPTION_FIREWALL
}

void rs_start_streaming(struct rs_camera * camera, struct rs_error ** error)
{
    BEGIN_EXCEPTION_FIREWALL
    camera->StartStreaming();
    END_EXCEPTION_FIREWALL
}

void rs_stop_streaming(struct rs_camera * camera, struct rs_error ** error)
{
    BEGIN_EXCEPTION_FIREWALL
    camera->StopStreaming();
    END_EXCEPTION_FIREWALL
}

void rs_wait_all_streams(struct rs_camera * camera, struct rs_error ** error)
{
    BEGIN_EXCEPTION_FIREWALL
    camera->WaitAllStreams();
    END_EXCEPTION_FIREWALL
}

const uint8_t *	rs_get_color_image(rs_camera * camera, rs_error ** error)
{
    BEGIN_EXCEPTION_FIREWALL
    return camera->GetColorImage();
    END_EXCEPTION_FIREWALL
}

const uint16_t * rs_get_depth_image(rs_camera * camera, rs_error ** error)
{
    BEGIN_EXCEPTION_FIREWALL
    return camera->GetDepthImage();
    END_EXCEPTION_FIREWALL
}

float rs_get_depth_scale(rs_camera * camera, rs_error ** error)
{
    BEGIN_EXCEPTION_FIREWALL
    return camera->GetDepthScale();
    END_EXCEPTION_FIREWALL
}

void rs_get_stream_intrinsics(struct rs_camera * camera, int stream, struct rs_intrinsics * intrin, struct rs_error ** error)
{
    BEGIN_EXCEPTION_FIREWALL
	*intrin = camera->GetStreamIntrinsics(stream);
    END_EXCEPTION_FIREWALL
}

void rs_get_stream_extrinsics(struct rs_camera * camera, int stream_from, int stream_to, struct rs_extrinsics * extrin, struct rs_error ** error)
{
    BEGIN_EXCEPTION_FIREWALL
	*extrin = camera->GetStreamExtrinsics(stream_from, stream_to);
    END_EXCEPTION_FIREWALL
}

const char * rs_get_failed_function(rs_error * error) { return error ? error->function.c_str() : nullptr; }
const char * rs_get_error_message(rs_error * error) { return error ? error->message.c_str() : nullptr; }
void rs_free_error(rs_error * error) { if (error) delete error; }
