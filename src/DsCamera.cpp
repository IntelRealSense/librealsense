#include "DsCamera.h"

#ifdef USE_DSAPI_DEVICES
static void CheckDS(DSAPI * ds, const std::string & call, bool b)
{
	if (!b)
	{
		throw std::runtime_error("DSAPI::" + call + "() returned " + DSStatusString(ds->getLastErrorStatus()) + ":\n  " + ds->getLastErrorDescription());
	}
}

DsCamera::DsCamera(DSAPI * ds, int idx) : rs_camera(idx), ds(ds)
{

}

DsCamera::~DsCamera()
{
	StopBackgroundCapture();
	DSDestroy(ds);
}

//@tofix protect against calling this multiple times
bool DsCamera::ConfigureStreams()
{
	if (streamingModeBitfield == 0)
		throw std::invalid_argument("No streams have been configured...");

	CheckDS(ds, "probeConfiguration", ds->probeConfiguration());
	CheckDS(ds, "enableZ", ds->enableZ(false));

	uint32_t serialNo;
	CheckDS(ds, "getCameraSerialNumber", ds->getCameraSerialNumber(serialNo));
	usbInfo.serial = rs::ToString() << serialNo;
	usbInfo.vid = 0; // TODO: Fix
	usbInfo.pid = 0; // TODO: Fix

	std::cout << "Serial Number: " << usbInfo.serial << std::endl;
	std::cout << "USB VID: " << usbInfo.vid << std::endl;
	std::cout << "USB PID: " << usbInfo.pid << std::endl;
	return true;
}

void DsCamera::StopBackgroundCapture()
{
	if (thread.joinable())
	{
		stopThread = true;
		thread.join();
		stopThread = false;
	}
}

void DsCamera::StartBackgroundCapture()
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
			if (third && third->isThirdEnabled())
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

void DsCamera::StartStream(int streamIdentifier, const rs::StreamConfiguration & c)
{
	StopBackgroundCapture();
	CheckDS(ds, "stopCapture", ds->stopCapture());

	if (streamIdentifier & RS_STREAM_DEPTH)
	{
		if (c.format != rs::FrameFormat::Z16) throw std::runtime_error("Only Z16 is supported");
		CheckDS(ds, "enableZ", ds->enableZ(true));
		CheckDS(ds, "setLRZResolutionMode", ds->setLRZResolutionMode(true, c.width, c.height - 1, c.fps ? c.fps : 60, DS_LUMINANCE8));
		depthFrame.reset(new TripleBufferedFrame(c.width, c.height - 1, sizeof(uint16_t)));
		zConfig = c;
	}
	else if (streamIdentifier & RS_STREAM_RGB)
	{
		if (c.format != rs::FrameFormat::YUYV) throw std::runtime_error("Only YUYV is supported");

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

void DsCamera::StopStream(int streamNum)
{

}

RectifiedIntrinsics DsCamera::GetDepthIntrinsics()
{
	DSCalibRectParameters params;
	CheckDS(ds, "getCalibRectParameters", ds->getCalibRectParameters(params));
	return reinterpret_cast<const RectifiedIntrinsics &>(params.modesLR[0][0]);
}

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
#endif