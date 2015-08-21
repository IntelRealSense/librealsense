#ifndef RS_INTERNAL_H
#define RS_INTERNAL_H

#include "../include/librealsense/rs.h"

#include <cassert>      // For assert
#include <cstring>      // For memcpy
#include <sstream>      // For ostringstream
#include <memory>       // For shared_ptr
#include <vector>       // For vector
#include <thread>       // For thread
#include <mutex>        // For mutex

// Currently, we will use DSAPI on Windows and libuvc everywhere else
#ifdef WIN32
#define USE_DSAPI_DEVICES
#else
#define USE_UVC_DEVICES
#endif

#ifdef USE_UVC_DEVICES
#include <libuvc/libuvc.h>
#endif

namespace rs
{
	struct ToString
	{
		std::ostringstream ss;
		template<class T> ToString & operator << (const T & val) { ss << val; return *this; }
		operator std::string() const { return ss.str(); }
	};
    
} // end namespace rs

struct rs_error
{
	std::string function;
	std::string message;
};

struct rs_camera
{
    virtual const char * GetCameraName() const = 0;

    virtual void EnableStream(int stream, int width, int height, int fps, int format) = 0;
    virtual void EnableStreamPreset(int stream, int preset) = 0;
    virtual void StartStreaming() = 0;
    virtual void StopStreaming() = 0;
    virtual void WaitAllStreams() = 0;

    virtual float GetDepthScale() const = 0;
    virtual rs_intrinsics GetStreamIntrinsics(int stream) const = 0;
    virtual rs_extrinsics GetStreamExtrinsics(int from, int to) const = 0;
    virtual const void * GetImagePixels(int stream) const = 0;    
};

struct rs_context
{
    #ifdef USE_UVC_DEVICES
	uvc_context_t * privateContext;
    #endif
	std::vector<std::shared_ptr<rs_camera>> cameras;
    
	rs_context();
	~rs_context();

	void QueryDeviceList();
};

#endif
