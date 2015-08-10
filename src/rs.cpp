#include <librealsense/rs.h>
#include <librealsense/R200/R200.h>

typedef struct RScontext_ * RScontext;
typedef struct RScamera_ * RScamera;

rs::CameraContext * in(RScontext context) { return reinterpret_cast<rs::CameraContext *>(context); }
rs::Camera * in(RScamera camera) { return reinterpret_cast<rs::Camera *>(camera); }
RScontext out(rs::CameraContext * context) { return reinterpret_cast<RScontext>(context); }
RScamera out(rs::Camera * camera) { return reinterpret_cast<RScamera>(camera); }

RScontext			rsCreateContext() { return out(new rs::CameraContext()); }
void				rsDeleteContext(RScontext context) { delete in(context); }
int					rsGetCameraCount(RScontext context) { return in(context)->cameras.size(); }
RScamera			rsGetCamera(RScontext context, int index) { return out(in(context)->cameras[index].get()); }

void				rsEnableStream(RScamera camera, RSenum stream) { in(camera)->EnableStream(stream); }
int					rsConfigureStreams(RScamera camera) { return in(camera)->ConfigureStreams(); }
void				rsStartStream(RScamera camera, RSenum stream, int width, int height, int fps, RSenum format) { in(camera)->StartStream(stream, { width, height, fps, (rs::FrameFormat)format }); }
void				rsStopStream(RScamera camera, RSenum stream) { in(camera)->StopStream(stream); }
const uint16_t *	rsGetDepthImage(RScamera camera) { return in(camera)->GetDepthImage(); }
const uint8_t *		rsGetColorImage(RScamera camera) { return in(camera)->GetColorImage(); }
int					rsIsStreaming(RScamera camera) { return in(camera)->IsStreaming(); }
int					rsGetCameraIndex(RScamera camera) { return in(camera)->GetCameraIndex(); }
uint64_t			rsGetFrameCount(RScamera camera) { return in(camera)->GetFrameCount(); }

int rsGetStreamPropertyi(RScamera camera, RSenum stream, RSenum prop)
{
	auto cam = in(camera);
	if (auto r200 = dynamic_cast<r200::R200Camera *>(cam))
	{
		if (stream != RS_STREAM_DEPTH) return 0;
		auto intrin = r200->GetRectifiedIntrinsicsZ();
		switch (prop)
		{
		case RS_IMAGE_SIZE_X: return intrin.rw;
		case RS_IMAGE_SIZE_Y: return intrin.rh;
		default: return 0;
		}
	}
	return 0;
}

float rsGetStreamPropertyf(RScamera camera, RSenum stream, RSenum prop)
{
	auto cam = in(camera);
	if (auto r200 = dynamic_cast<r200::R200Camera *>(cam))
	{
		if (stream != RS_STREAM_DEPTH) return 0;
		auto intrin = r200->GetRectifiedIntrinsicsZ();
		switch (prop)
		{
		case RS_FOCAL_LENGTH_X: return intrin.rfx;
		case RS_FOCAL_LENGTH_Y: return intrin.rfy;
		case RS_PRINCIPAL_POINT_X: return intrin.rpx;
		case RS_PRINCIPAL_POINT_Y: return intrin.rpy;
		default: return 0;
		}
	}
	return 0;
}