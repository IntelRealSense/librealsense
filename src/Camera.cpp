#include <librealsense/Camera.h>

namespace rs
{
	uint16_t * Camera::GetDepthImage()
	{
		if (depthFrame->updated)
		{
			std::lock_guard<std::mutex> guard(frameMutex);
			depthFrame->swap_front();
		}
		uint16_t * framePtr = reinterpret_cast<uint16_t *>(depthFrame->front.data());
		return framePtr;
	}

	uint8_t * Camera::GetColorImage()
	{
		if (colorFrame->updated)
		{
			std::lock_guard<std::mutex> guard(frameMutex);
			colorFrame->swap_front();
		}
		uint8_t * framePtr = reinterpret_cast<uint8_t *>(colorFrame->front.data());
		return framePtr;
	}

	bool Camera::IsStreaming()
	{
		if (streamingModeBitfield & STREAM_DEPTH) return true;
		else if (streamingModeBitfield & STREAM_RGB) return true;
		return false;
	}
}