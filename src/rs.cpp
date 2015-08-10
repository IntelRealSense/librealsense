#include <librealsense/rs.h>
#include <librealsense/R200/R200.h>

const char * rsGetFailedFunction(RSerror error) { return error ? error->function.c_str() : nullptr; }
const char * rsGetErrorMessage(RSerror error) {	return error ? error->message.c_str() : nullptr; }
void rsFreeError(RSerror error) { if(error) delete error; }

int rsGetStreamPropertyi(RScamera camera, RSenum stream, RSenum prop, RSerror * error)
{
	return Try("rsGetStreamPropertyi", error, [&]() -> int
	{
		if (auto r200 = dynamic_cast<r200::R200Camera *>(camera))
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
	});
}

float rsGetStreamPropertyf(RScamera camera, RSenum stream, RSenum prop, RSerror * error)
{
	return Try("rsGetStreamPropertyf", error, [&]() -> float
	{
		if (auto r200 = dynamic_cast<r200::R200Camera *>(camera))
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
	});
}