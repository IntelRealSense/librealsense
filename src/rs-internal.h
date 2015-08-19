#ifndef RS_INTERNAL_H
#define RS_INTERNAL_H

#include "../include/librealsense/rs.h"

// Currently, we will use DSAPI on Windows and libuvc everywhere else
#ifdef WIN32
#define USE_DSAPI_DEVICES
#else
#define USE_UVC_DEVICES
#endif

#ifdef USE_UVC_DEVICES
#include <libuvc/libuvc.h>
#endif

#include "Common.h"

namespace rs
{
	enum class FrameFormat
    {
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
	NO_MOVE(rs_camera);

	int cameraIdx;
    std::string cameraName;

	rs::USBDeviceInfo usbInfo = {};

	uint64_t frameCount = 0;

    TripleBufferedFrame<uint16_t> depthFrame;
    TripleBufferedFrame<uint8_t> colorFrame;
    std::vector<float> vertices;

	rs_camera(int index) : cameraIdx(index) {}
    ~rs_camera() {}

    virtual void EnableStream(int stream, int width, int height, int fps, rs::FrameFormat format) = 0;
    virtual void EnableStreamPreset(int stream, int preset) = 0;
    virtual void StartStreaming() = 0;
    virtual void StopStreaming() = 0;

    void WaitAllStreams();
    
	virtual rs_intrinsics GetStreamIntrinsics(int stream) = 0;
	virtual rs_extrinsics GetStreamExtrinsics(int from, int to) = 0;
    virtual void ComputeVertexImage() = 0;
    virtual float GetDepthScale() = 0;

    const uint8_t * GetColorImage() const { return colorFrame.front_data(); }
    const uint16_t * GetDepthImage() const { return depthFrame.front_data(); }
    const float * GetVertexImage() const { return vertices.data(); }
};

struct rs_context
{
	NO_MOVE(rs_context);
    #ifdef USE_UVC_DEVICES
	uvc_context_t * privateContext;
    #endif
	std::vector<std::shared_ptr<rs_camera>> cameras;
    
	rs_context();
	~rs_context();

	void QueryDeviceList();
};

// This facility allows for translation of exceptions to rs_error structs at the API boundary
template<class T> T DefResult(T *) { return T(); }
inline void DefResult(void *) {}
template<class F> auto Try(const char * name, rs_error ** error, F f) -> decltype(f())
{
	if (error) *error = nullptr;
	try { return f(); }
	catch (const std::exception & e)
	{
		if (error) *error = new rs_error{ name, e.what() }; // TODO: Handle case where THIS code throws
		return DefResult((decltype(f()) *)nullptr);
	}
	catch (...)
	{
		if (error) *error = new rs_error{ name, "unknown error" }; // TODO: Handle case where THIS code throws
		return DefResult((decltype(f()) *)nullptr);
	}
}
#define BEGIN_EXCEPTION_FIREWALL return Try(__FUNCTION__, error, [&]() {
#define END_EXCEPTION_FIREWALL });

#endif
