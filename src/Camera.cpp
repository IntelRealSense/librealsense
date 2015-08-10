#include "rs-internal.h"

void rsEnableStream(RScamera camera, RSenum stream, RSerror * error) 
{
	return Try("rsEnableStream", error, [&]() { camera->streamingModeBitfield |= stream; });
}

int rsIsStreaming(RScamera camera, RSerror * error) 
{
	return Try("rsIsStreaming", error, [&]() { return camera->streamingModeBitfield & RS_STREAM_DEPTH || camera->streamingModeBitfield & RS_STREAM_RGB ? 1 : 0; });
}

int	rsGetCameraIndex(RScamera camera, RSerror * error)
{
	return Try("rsGetCameraIndex", error, [&]() { return camera->cameraIdx; });
}

uint64_t rsGetFrameCount(RScamera camera, RSerror * error)
{
	return Try("rsGetFrameCount", error, [&]() { return camera->frameCount; });
}

const uint8_t *	rsGetColorImage(RScamera camera, RSerror * error)
{
	return Try("rsGetColorImage", error, [&]()
	{
		if (camera->colorFrame->updated)
		{
			std::lock_guard<std::mutex> guard(camera->frameMutex);
			camera->colorFrame->swap_front();
		}
		return reinterpret_cast<const uint8_t *>(camera->colorFrame->front.data());
	});
}

const uint16_t * rsGetDepthImage(RScamera camera, RSerror * error)
{
	return Try("rsGetDepthImage", error, [&]()
	{
		if (camera->depthFrame->updated)
		{
			std::lock_guard<std::mutex> guard(camera->frameMutex);
			camera->depthFrame->swap_front();
		}
		return reinterpret_cast<const uint16_t *>(camera->depthFrame->front.data());
	});
}

int	rsConfigureStreams(RScamera camera, RSerror * error)
{
	return Try("rsConfigureStreams", error, [&]() { return camera->ConfigureStreams(); });
}

void rsStartStream(RScamera camera, RSenum stream, int width, int height, int fps, RSenum format, RSerror * error)
{ 
	return Try("rsStartStream", error, [&]() { camera->StartStream(stream, { width, height, fps, (rs::FrameFormat)format }); });
}

void rsStopStream(RScamera camera, RSenum stream, RSerror * error)
{
	return Try("rsStopStream", error, [&]() { camera->StopStream(stream); });
}
