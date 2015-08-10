#include <librealsense/rs.h>
#include <librealsense/R200/R200.h>

const char * rs_get_failed_function(rs_error * error) { return error ? error->function.c_str() : nullptr; }
const char * rs_get_error_message(rs_error * error) { return error ? error->message.c_str() : nullptr; }
void rs_free_error(rs_error * error) { if (error) delete error; }

int rs_get_stream_property_i(rs_camera * camera, int stream, int prop, rs_error ** error)
{
	return Try("rs_get_stream_property_i", error, [&]() -> int
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

float rs_get_stream_property_f(rs_camera * camera, int stream, int prop, rs_error ** error)
{
	return Try("rs_get_stream_property_f", error, [&]() -> float
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