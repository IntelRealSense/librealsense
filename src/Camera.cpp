#include "rs-internal.h"

void rs_enable_stream(rs_camera * camera, int stream, rs_error ** error) { BEGIN camera->streamingModeBitfield |= stream; END }
int rs_is_streaming(rs_camera * camera, rs_error ** error) { BEGIN return camera->streamingModeBitfield & RS_STREAM_DEPTH || camera->streamingModeBitfield & RS_STREAM_RGB ? 1 : 0; END }
int	rs_get_camera_index(rs_camera * camera, rs_error ** error) { BEGIN return camera->cameraIdx; END }
uint64_t rs_get_frame_count(rs_camera * camera, rs_error ** error) { BEGIN return camera->frameCount; END }
const uint8_t *	rs_get_color_image(rs_camera * camera, rs_error ** error)
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

const uint16_t * rs_get_depth_image(rs_camera * camera, rs_error ** error)
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

int	rs_configure_streams(rs_camera * camera, rs_error ** error) { BEGIN return camera->ConfigureStreams(); END }
void rs_start_stream(rs_camera * camera, int stream, int width, int height, int fps, int format, rs_error ** error) { BEGIN camera->StartStream(stream, { width, height, fps, (rs::FrameFormat)format }); END }
void rs_stop_stream(rs_camera * camera, int stream, rs_error ** error) { BEGIN camera->StopStream(stream); END }
