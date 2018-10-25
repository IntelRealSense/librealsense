#include "Rs2Base.h"

namespace oni { namespace driver {

// StreamType

bool isSupportedStreamType(rs2_stream type)
{
	switch (type)
	{
		case RS2_STREAM_DEPTH:
		case RS2_STREAM_COLOR:
		case RS2_STREAM_INFRARED:
			return true;
	}
	return false;
}

OniSensorType convertStreamType(rs2_stream type)
{
	switch (type)
	{
		case RS2_STREAM_DEPTH: return ONI_SENSOR_DEPTH;
		case RS2_STREAM_COLOR: return ONI_SENSOR_COLOR;
		case RS2_STREAM_INFRARED: return ONI_SENSOR_IR;
	}
	rsTraceError("Invalid rs2_stream=%d", (int)type);
	RS2_ASSERT(false);
	return (OniSensorType)0;
}

rs2_stream convertStreamType(OniSensorType type)
{
	switch (type)
	{
		case ONI_SENSOR_DEPTH: return RS2_STREAM_DEPTH;
		case ONI_SENSOR_COLOR: return RS2_STREAM_COLOR;
		case ONI_SENSOR_IR: return RS2_STREAM_INFRARED;
	}
	rsTraceError("Invalid OniSensorType=%d", (int)type);
	RS2_ASSERT(false);
	return (rs2_stream)0;
}

// PixelFormat

bool isSupportedPixelFormat(rs2_format type)
{
	switch (type)
	{
		case RS2_FORMAT_Z16:
		case RS2_FORMAT_YUYV:
		case RS2_FORMAT_RGB8:
		case RS2_FORMAT_Y8:
		case RS2_FORMAT_Y16:
			return true;
	}
	return false;
}

int getPixelFormatBytes(rs2_format type)
{
	switch (type)
	{
		case RS2_FORMAT_Z16: return 2;
		case RS2_FORMAT_YUYV: return 2;
		case RS2_FORMAT_RGB8: return 3;
		case RS2_FORMAT_Y8: return 1;
		case RS2_FORMAT_Y16: return 2;
	}
	rsTraceError("Invalid rs2_format=%d", (int)type);
	RS2_ASSERT(false);
	return 0;
}

OniPixelFormat convertPixelFormat(rs2_format type)
{
	switch (type)
	{
		case RS2_FORMAT_Z16: return ONI_PIXEL_FORMAT_DEPTH_1_MM;
		case RS2_FORMAT_YUYV: return ONI_PIXEL_FORMAT_YUYV;
		case RS2_FORMAT_RGB8: return ONI_PIXEL_FORMAT_RGB888;
		case RS2_FORMAT_Y8: return ONI_PIXEL_FORMAT_GRAY8;
		case RS2_FORMAT_Y16: return ONI_PIXEL_FORMAT_GRAY16;
	}
	rsTraceError("Invalid rs2_format=%d", (int)type);
	RS2_ASSERT(false);
	return (OniPixelFormat)0;
}

rs2_format convertPixelFormat(OniPixelFormat type)
{
	switch (type)
	{
		case ONI_PIXEL_FORMAT_DEPTH_1_MM: return RS2_FORMAT_Z16;
		case ONI_PIXEL_FORMAT_YUYV: return RS2_FORMAT_YUYV;
		case ONI_PIXEL_FORMAT_RGB888: return RS2_FORMAT_RGB8;
		case ONI_PIXEL_FORMAT_GRAY8: return RS2_FORMAT_Y8;
		case ONI_PIXEL_FORMAT_GRAY16: return RS2_FORMAT_Y16;
	}
	rsTraceError("Invalid OniPixelFormat=%d", (int)type);
	RS2_ASSERT(false);
	return (rs2_format)0;
}

}} // namespace
