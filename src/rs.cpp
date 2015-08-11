#include "rs-internal.h"

#ifdef USE_UVC_DEVICES
#include "R200/R200.h"
#include "F200/F200.h"
#endif

#ifdef USE_DSAPI_DEVICES
#include "DsCamera.h"
#endif

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
	if (privateContext)
	{
		uvc_exit(privateContext);
	}
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
			if (desc->idProduct == 2688)
			{
				cameras.push_back(std::make_shared<r200::R200Camera>(list[index], index));
			}

			else if (desc->idProduct == 2662)
			{
				cameras.push_back(std::make_shared<f200::F200Camera>(list[index], index));
			}

			uvc_free_device_descriptor(desc);
		}
		index++;
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

rs_context * rs_create_context(int api_version, rs_error ** error) 
{
	BEGIN
	if (api_version != RS_API_VERSION) throw std::runtime_error("api version mismatch");
	return new rs_context();
	END 
}
void rs_delete_context(rs_context * context, rs_error ** error) { BEGIN delete context; END }
int	rs_get_camera_count(rs_context * context, rs_error ** error) { BEGIN return (int)context->cameras.size(); END }
rs_camera * rs_get_camera(rs_context * context, int index, rs_error ** error) { BEGIN return context->cameras[index].get(); END }

void rs_enable_stream(rs_camera * camera, int stream, rs_error ** error) { BEGIN camera->streamingModeBitfield |= stream; END }
int rs_is_streaming(rs_camera * camera, rs_error ** error) { BEGIN return camera->streamingModeBitfield & RS_STREAM_DEPTH || camera->streamingModeBitfield & RS_STREAM_RGB ? 1 : 0; END }
int	rs_get_camera_index(rs_camera * camera, rs_error ** error) { BEGIN return camera->cameraIdx; END }
uint64_t rs_get_frame_count(rs_camera * camera, rs_error ** error) { BEGIN return camera->frameCount; END }
const uint8_t *	rs_get_color_image(rs_camera * camera, rs_error ** error)
{
	BEGIN
	if (camera->colorFrame->updated)
	{
		std::lock_guard<std::mutex> guard(camera->frameMutex);
		camera->colorFrame->swap_front();
	}
	return reinterpret_cast<const uint8_t *>(camera->colorFrame->front.data());
	END
}

const uint16_t * rs_get_depth_image(rs_camera * camera, rs_error ** error)
{
	BEGIN
	if (camera->depthFrame->updated)
	{
		std::lock_guard<std::mutex> guard(camera->frameMutex);
		camera->depthFrame->swap_front();
	}
	return reinterpret_cast<const uint16_t *>(camera->depthFrame->front.data());
	END
}

int	rs_configure_streams(rs_camera * camera, rs_error ** error) { BEGIN return camera->ConfigureStreams(); END }
void rs_start_stream(rs_camera * camera, int stream, int width, int height, int fps, int format, rs_error ** error) { BEGIN camera->StartStream(stream, { width, height, fps, (rs::FrameFormat)format }); END }
void rs_stop_stream(rs_camera * camera, int stream, rs_error ** error) { BEGIN camera->StopStream(stream); END }

int rs_get_stream_property_i(rs_camera * camera, int stream, int prop, rs_error ** error)
{
	return Try("rs_get_stream_property_i", error, [&]() -> int
	{
		if (stream != RS_STREAM_DEPTH) return 0;
		switch (prop)
		{
		case RS_IMAGE_SIZE_X: return camera->GetDepthIntrinsics().rw;
		case RS_IMAGE_SIZE_Y: return camera->GetDepthIntrinsics().rh;
		default: return 0;
		}
	});
}

float rs_get_stream_property_f(rs_camera * camera, int stream, int prop, rs_error ** error)
{
	return Try("rs_get_stream_property_f", error, [&]() -> float
	{
		if (stream != RS_STREAM_DEPTH) return 0;
		switch (prop)
		{
		case RS_FOCAL_LENGTH_X: return camera->GetDepthIntrinsics().rfx;
		case RS_FOCAL_LENGTH_Y: return camera->GetDepthIntrinsics().rfy;
		case RS_PRINCIPAL_POINT_X: return camera->GetDepthIntrinsics().rpx;
		case RS_PRINCIPAL_POINT_Y: return camera->GetDepthIntrinsics().rpy;
		default: return 0;
		}
	});
}

const char * rs_get_failed_function(rs_error * error) { return error ? error->function.c_str() : nullptr; }
const char * rs_get_error_message(rs_error * error) { return error ? error->message.c_str() : nullptr; }
void rs_free_error(rs_error * error) { if (error) delete error; }
