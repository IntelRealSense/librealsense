#pragma once

#ifndef LIBREALSENSE_DS_CAMERA_H
#define LIBREALSENSE_DS_CAMERA_H

#include "rs-internal.h"

#ifdef USE_DSAPI_DEVICES
#include <DSAPI.h>
#include <DSAPI/DSFactory.h>
class DsCamera : public rs_camera
{
	DSAPI * ds;
	rs::StreamConfiguration zConfig;
	rs::StreamConfiguration rgbConfig;
	volatile bool stopThread = false;
	std::thread thread;

	void StopBackgroundCapture();
	void StartBackgroundCapture();
public:
	DsCamera(DSAPI * ds, int num);
	~DsCamera();

	bool ConfigureStreams() override;
	void StartStream(int streamIdentifier, const rs::StreamConfiguration & config) override;
	void StopStream(int streamIdentifier) override;
	RectifiedIntrinsics GetDepthIntrinsics() override;

	rs::StreamConfiguration GetZConfig() { return zConfig; }
	rs::StreamConfiguration GetRGBConfig() { return rgbConfig; }
};
#endif

#endif
