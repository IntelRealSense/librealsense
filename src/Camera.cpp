#include "rs-internal.h"

void rsEnableStream(RScamera camera, RSenum stream, RSerror * error) { BEGIN camera->streamingModeBitfield |= stream; END }
int rsIsStreaming(RScamera camera, RSerror * error) { BEGIN return camera->streamingModeBitfield & RS_STREAM_DEPTH || camera->streamingModeBitfield & RS_STREAM_RGB ? 1 : 0; END }
int	rsGetCameraIndex(RScamera camera, RSerror * error) { BEGIN return camera->cameraIdx; END }
uint64_t rsGetFrameCount(RScamera camera, RSerror * error) { BEGIN return camera->frameCount; END }
const uint8_t *	rsGetColorImage(RScamera camera, RSerror * error)
{
	BEGIN
	if (camera->colorFrame->updated)
	{
		std::lock_guard<std::mutex> guard(camera->frameMutex);
		camera->colorFrame->swap_front();
	}
	return reinterpret_cast<const uint8_t *>(camera->colorFrame->front.data());
	END
}

const uint16_t * rsGetDepthImage(RScamera camera, RSerror * error)
{
	BEGIN
	if (camera->depthFrame->updated)
	{
		std::lock_guard<std::mutex> guard(camera->frameMutex);
		camera->depthFrame->swap_front();
	}
	return reinterpret_cast<const uint16_t *>(camera->depthFrame->front.data());
	END
}

int	rsConfigureStreams(RScamera camera, RSerror * error) { BEGIN return camera->ConfigureStreams(); END }
void rsStartStream(RScamera camera, RSenum stream, int width, int height, int fps, RSenum format, RSerror * error) { BEGIN camera->StartStream(stream, { width, height, fps, (rs::FrameFormat)format }); END }
void rsStopStream(RScamera camera, RSenum stream, RSerror * error) { BEGIN camera->StopStream(stream); END }
