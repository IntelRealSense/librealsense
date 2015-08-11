#ifndef RS_INTERNAL_H
#define RS_INTERNAL_H

#include "../include/librealsense/rs.h"

#ifdef WIN32
#include <DSAPI/DSFactory.h"
#endif

#ifndef WIN32
#include "libuvc/libuvc.h"
#endif

#include "Common.h"

namespace rs
{
	enum class FrameFormat {
		UNKNOWN = 0,
		ANY = 0, // Any supported format
		UNCOMPRESSED,
		COMPRESSED,
		YUYV, // YUYV/YUV2/YUV422: YUV encoding with one luminance value per pixel and one UV (chrominance) pair for every two pixels.
		UYVY,
		Y12I,
		Y16,
		Y8,
		Z16,
		RGB,
		BGR,
		MJPEG,
		GRAY8,
		BY8,
		INVI, //IR
		RELI, //IR
		INVR, //Depth
		INVZ, //Depth
		INRI, //Depth (24 bit)
		COUNT
	};

	struct StreamConfiguration
	{
		int width;
		int height;
		int fps;
		FrameFormat format;
	};

	struct USBDeviceInfo
	{
		std::string serial;
		uint16_t vid;
		uint16_t pid;
	};

} // end namespace rs

struct rs_error
{
	std::string function;
	std::string message;
};

template<class T> T DefResult(T *) { return T(); }
inline void DefResult(void *) {}
template<class F> auto Try(const char * name, rs_error ** error, F f) -> decltype(f())
{
	if (error) *error = nullptr;
	try { return f(); }
	catch (const std::exception & e)
	{
		if (error) *error = new rs_error{name, e.what()}; // TODO: Handle case where THIS code throws
		return DefResult((decltype(f()) *)nullptr);
	}
	catch (...)
	{
		if (error) *error = new rs_error{name, "unknown error"}; // TODO: Handle case where THIS code throws
		return DefResult((decltype(f()) *)nullptr);
	}
}
#define BEGIN return Try(__FUNCTION__, error, [&]() {
#define END });

struct rs_camera
{
	NO_MOVE(rs_camera);

	int cameraIdx;

	rs::USBDeviceInfo usbInfo = {};

	uint32_t streamingModeBitfield = 0;
	uint64_t frameCount = 0;

	std::mutex frameMutex;
	std::unique_ptr<TripleBufferedFrame> depthFrame;
	std::unique_ptr<TripleBufferedFrame> colorFrame;

	rs_camera(int index) : cameraIdx(index) {}
	virtual ~rs_camera() {}

	virtual bool ConfigureStreams() = 0;
	virtual void StartStream(int streamIdentifier, const rs::StreamConfiguration & config) = 0;
	virtual void StopStream(int streamIdentifier) = 0;
};

struct rs_context
{
	NO_MOVE(rs_context);
#ifndef WIN32
	uvc_context_t * privateContext;
#endif
	std::vector<std::shared_ptr<rs_camera>> cameras;
    
	rs_context();
	~rs_context();

	void QueryDeviceList();
    
    //@tofix
    void * GetLibusbContext() { return uvc_get_libusb_context(privateContext); }
};

#endif
