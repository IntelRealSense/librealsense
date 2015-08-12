#include "../Common.h"
#include "R200.h"

#include <sstream>

#if defined(_WIN64)
#ifdef _DEBUG
#pragma comment(lib, "dsapi.dbg.lib")
#else
#pragma comment(lib, "dsapi.lib")
#endif
#elif defined(_WIN32)
#ifdef _DEBUG
#pragma comment(lib, "dsapi32.dbg.lib")
#else
#pragma comment(lib, "dsapi32.lib")
#endif
#endif

using namespace rs;

namespace r200
{
    
#ifndef WIN32
R200Camera::R200Camera(uvc_device_t * device, int idx) : UVCCamera(device, idx)
{
    
}

R200Camera::~R200Camera()
{
    
}

//@tofix protect against calling this multiple times
bool R200Camera::ConfigureStreams()
{
    if (streamingModeBitfield == 0)
        throw std::invalid_argument("No streams have been configured...");
    
    //@tofix: Test for successful open
    if (streamingModeBitfield & RS_STREAM_DEPTH)
    {
        StreamInterface * stream = new StreamInterface();
        stream->camera = this;
        
        bool status = OpenStreamOnSubdevice(hardware, stream->uvcHandle, 1);
        streamInterfaces.insert(std::pair<int, StreamInterface *>(RS_STREAM_DEPTH, stream));
    }
    
    if (streamingModeBitfield & RS_STREAM_RGB)
    {
        StreamInterface * stream = new StreamInterface();
        stream->camera = this;
        
        bool status = OpenStreamOnSubdevice(hardware, stream->uvcHandle, 2);
        streamInterfaces.insert(std::pair<int, StreamInterface *>(RS_STREAM_RGB, stream));
    }
    
    
    /*
    if (streamingModeBitfield & STREAM_LR)
    {
        openDevice(0);
    }
    */
    
    uvc_device_descriptor_t * desc;
    if(uvc_get_device_descriptor(hardware, &desc) == UVC_SUCCESS)
    {
        if (desc->serialNumber) usbInfo.serial = desc->serialNumber;
        if (desc->idVendor) usbInfo.vid = desc->idVendor;
        if (desc->idProduct) usbInfo.pid = desc->idProduct;
        uvc_free_device_descriptor(desc);
    }
    
    std::cout << "Serial Number: " << usbInfo.serial << std::endl;
    std::cout << "USB VID: " << usbInfo.vid << std::endl;
    std::cout << "USB PID: " << usbInfo.pid << std::endl;
    
    auto oneTimeInitialize = [&](uvc_device_handle_t * uvc_handle)
    {
        spiInterface.reset(new SPI_Interface(uvc_handle));
        
        spiInterface->Initialize();
        
        //uvc_print_diag(uvc_handle, stderr);

        std::cout << "Firmware Revision: " << GetFirmwareVersion(uvc_handle) << std::endl;

        spiInterface->LogDebugInfo();
        
        if (!SetStreamIntent(uvc_handle, streamingModeBitfield))
        {
            throw std::runtime_error("Could not set stream intent. Replug camera?");
        }
    };
    
    // We only need to do this once, so check if any stream has been configured
    if (streamInterfaces[RS_STREAM_DEPTH]->uvcHandle)
    {
        //uvc_print_stream_ctrl(&streamInterfaces[STREAM_DEPTH]->ctrl, stderr);
        oneTimeInitialize(streamInterfaces[RS_STREAM_DEPTH]->uvcHandle);
    }
    
    else if (streamInterfaces[RS_STREAM_RGB]->uvcHandle)
    {
       //uvc_print_stream_ctrl(&streamInterfaces[STREAM_RGB]->ctrl, stderr);
       oneTimeInitialize(streamInterfaces[RS_STREAM_RGB]->uvcHandle);
    }
    
    return true;
}

void R200Camera::StartStream(int streamIdentifier, const StreamConfiguration & c)
{
    //  if (isStreaming) throw std::runtime_error("Camera is already streaming");
    
    auto stream = streamInterfaces[streamIdentifier];
    
    if (stream->uvcHandle)
    {
        stream->fmt = static_cast<uvc_frame_format>(c.format);
        
        uvc_error_t status = uvc_get_stream_ctrl_format_size(stream->uvcHandle, &stream->ctrl, stream->fmt, c.width, c.height, c.fps);
        
        if (status < 0)
        {
            uvc_perror(status, "uvc_get_stream_ctrl_format_size");
            throw std::runtime_error("Open camera_handle Failed");
        }
        
        //@tofix - check streaming mode as well
        if (c.format == FrameFormat::Z16)
        {
            depthFrame.reset(new TripleBufferedFrame(c.width, c.height, 2));
            zConfig = c;
        }

		else if (c.format == FrameFormat::YUYV)
        {
            colorFrame.reset(new TripleBufferedFrame(c.width, c.height, 3));
            rgbConfig = c;
        }
        
        uvc_error_t startStreamResult = uvc_start_streaming(stream->uvcHandle, &stream->ctrl, &UVCCamera::cb, stream, 0);
        
        if (startStreamResult < 0)
        {
            uvc_perror(startStreamResult, "start_stream");
            throw std::runtime_error("Could not start stream");
        }
    }
    
    //@tofix - else what?
    
}

void R200Camera::StopStream(int streamNum)
{
    //@tofix - uvc_stream_stop with a real stream handle -> index with map that we have
    //uvc_stop_streaming(deviceHandle);
}
    
RectifiedIntrinsics R200Camera::GetRectifiedIntrinsicsZ()
{
    auto calibration = spiInterface->GetCalibration();
    return calibration.modesLR[0]; // Warning: assume mode 0 here (628x468)
}
#else

static void CheckDS(DSAPI * ds, const std::string & call, bool b)
{
	if (!b)
	{
		throw std::runtime_error("DSAPI::" + call + "() returned " + DSStatusString(ds->getLastErrorStatus()) + ":\n  " + ds->getLastErrorDescription());
	}
}

R200Camera::R200Camera(DSAPI * ds, int idx) : rs_camera(idx), ds(ds)
{

}

R200Camera::~R200Camera()
{
	StopBackgroundCapture();
	DSDestroy(ds);
}

//@tofix protect against calling this multiple times
bool R200Camera::ConfigureStreams()
{
	if (streamingModeBitfield == 0)
		throw std::invalid_argument("No streams have been configured...");

	CheckDS(ds, "probeConfiguration", ds->probeConfiguration());
	CheckDS(ds, "enableZ", ds->enableZ(false));

	uint32_t serialNo;
	CheckDS(ds, "getCameraSerialNumber", ds->getCameraSerialNumber(serialNo));
	std::ostringstream ss; ss << serialNo;
	usbInfo.serial = ss.str();
	usbInfo.vid = 0; // TODO: Fix
	usbInfo.pid = 0; // TODO: Fix

	std::cout << "Serial Number: " << usbInfo.serial << std::endl;
	std::cout << "USB VID: " << usbInfo.vid << std::endl;
	std::cout << "USB PID: " << usbInfo.pid << std::endl;
	return true;
}

void R200Camera::StopBackgroundCapture()
{
	if (thread.joinable())
	{
		stopThread = true;
		thread.join();
		stopThread = false;
	}
}

void R200Camera::StartBackgroundCapture()
{
	StopBackgroundCapture();

	thread = std::thread([this]()
	{
		while (!stopThread)
		{
			CheckDS(ds, "grab", ds->grab());

			if (ds->isZEnabled())
			{
				memcpy(depthFrame->back.data(), ds->getZImage(), (ds->zWidth() * ds->zHeight() * sizeof(uint16_t)));
				std::lock_guard<std::mutex> lock(frameMutex);
				depthFrame->swap_back();
			}

			auto third = ds->accessThird();
			if(third && third->isThirdEnabled())
			{
				//@tofix - this is a bit silly to overallocate. Blame Leo.
				static uint8_t color_cvt[1920 * 1080 * 3]; // YUYV = 16 bits in in -> 24 out
				convert_yuyv_rgb((uint8_t *)third->getThirdImage(), third->thirdWidth(), third->thirdHeight(), color_cvt);
				memcpy(colorFrame->back.data(), color_cvt, (third->thirdWidth() * third->thirdHeight()) * 3);
				std::lock_guard<std::mutex> lock(frameMutex);
				colorFrame->swap_back();
			}
		}

	});
}

void R200Camera::StartStream(int streamIdentifier, const StreamConfiguration & c)
{
	StopBackgroundCapture();
	CheckDS(ds, "stopCapture", ds->stopCapture());

	if (streamIdentifier & RS_STREAM_DEPTH)
	{
		if (c.format != FrameFormat::Z16) throw std::runtime_error("Only Z16 is supported");
		CheckDS(ds, "enableZ", ds->enableZ(true));
		CheckDS(ds, "setLRZResolutionMode", ds->setLRZResolutionMode(true, c.width, c.height-1, c.fps ? c.fps : 60, DS_LUMINANCE8));
		depthFrame.reset(new TripleBufferedFrame(c.width, c.height-1, sizeof(uint16_t)));
		zConfig = c;
	}
	else if (streamIdentifier & RS_STREAM_RGB)
	{
		if (c.format != FrameFormat::YUYV) throw std::runtime_error("Only YUYV is supported");

		auto third = ds->accessThird();
		if (!third) throw std::runtime_error("Unable to start RGB stream");
		CheckDS(ds, "enableThird", third->enableThird(true));
		CheckDS(ds, "setThirdResolutionMode", third->setThirdResolutionMode(false, c.width, c.height, c.fps ? c.fps : 60, DS_NATIVE_YUY2));
		colorFrame.reset(new TripleBufferedFrame(c.width, c.height, 3));
		rgbConfig = c;
	}

	CheckDS(ds, "startCapture", ds->startCapture());
	StartBackgroundCapture();
}

void R200Camera::StopStream(int streamNum)
{
	//@tofix - uvc_stream_stop with a real stream handle -> index with map that we have
	//uvc_stop_streaming(deviceHandle);
}

RectifiedIntrinsics R200Camera::GetRectifiedIntrinsicsZ()
{
	DSCalibRectParameters params;
	CheckDS(ds, "getCalibRectParameters", ds->getCalibRectParameters(params));
	return reinterpret_cast<const RectifiedIntrinsics &>(params.modesLR[0][0]);
}

#endif

} // end namespace r200
