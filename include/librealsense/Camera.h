#ifndef LIBREALSENSE_CAMERA_H
#define LIBREALSENSE_CAMERA_H

#include <librealsense/Common.h>

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

	//@tofix - this has an overloaded meaning for R200
	#define STREAM_DEPTH 1
	#define STREAM_LR 2
	#define STREAM_RGB 4

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

	////////////////
	// UVC Camera //
	////////////////

	class Camera
	{
		NO_MOVE(Camera);
		
	protected:

		int cameraIdx;

		USBDeviceInfo usbInfo = {};

		//bool hardwareInitialized = false;

		uint32_t streamingModeBitfield = 0;
		uint64_t frameCount = 0;

		std::mutex frameMutex;
		std::unique_ptr<TripleBufferedFrame> depthFrame;
		std::unique_ptr<TripleBufferedFrame> colorFrame;

	public:
		Camera(int index) : cameraIdx(index) {}
		virtual ~Camera() {}

		void EnableStream(int whichStream) { streamingModeBitfield |= whichStream; }

		virtual bool ConfigureStreams() = 0;
		virtual void StartStream(int streamIdentifier, const StreamConfiguration & config) = 0;
		virtual void StopStream(int streamIdentifier) = 0;

		uint16_t * GetDepthImage();
		uint8_t * GetColorImage();

		bool IsStreaming();
		int GetCameraIndex() { return cameraIdx; }
		uint64_t GetFrameCount() { return frameCount; }
	};

} // end namespace rs

#endif