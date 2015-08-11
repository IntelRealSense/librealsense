#pragma once

#ifndef LIBREALSENSE_R200_CAMERA_H
#define LIBREALSENSE_R200_CAMERA_H

#include "rs-internal.h" // TODO: Migrate this header to be internal to LibRealsense

#ifndef WIN32
#include "CameraHeader.h"
#include "XU.h"
#include "SPI.h"
#include "UVCCamera.h"
#else
#include <DSAPI.h>
#endif

namespace r200
{


#ifndef WIN32
class R200Camera : public rs::UVCCamera
#else
class R200Camera : public rs_camera
#endif
{
#ifndef WIN32
    std::unique_ptr<SPI_Interface> spiInterface;
#else
	DSAPI * ds;
	volatile bool stopThread = false;
	std::thread thread;
	void StopBackgroundCapture();
	void StartBackgroundCapture();
#endif
    rs::StreamConfiguration zConfig;
    rs::StreamConfiguration rgbConfig;
public:
#ifndef WIN32
    R200Camera(uvc_device_t * device, int num);
#else
	R200Camera(DSAPI * ds, int num);
#endif
    virtual ~R200Camera();
    
    virtual bool ConfigureStreams() override;
    
    virtual void StartStream(int streamIdentifier, const rs::StreamConfiguration & config) override;
    
    virtual void StopStream(int streamIdentifier) override;
    
    RectifiedIntrinsics GetRectifiedIntrinsicsZ();
    
    rs::StreamConfiguration GetZConfig() { return zConfig; }
    rs::StreamConfiguration GetRGBConfig() { return rgbConfig; }
    
};
    
} // end namespace r200

#endif