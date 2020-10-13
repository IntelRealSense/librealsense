#include "Rs2Driver.h"
#include "Rs2Commands.h"
#include "XnDepthShiftTables.h"
#include <librealsense2/rsutil.h>

namespace oni { namespace driver {

Rs2Stream::Rs2Stream(OniSensorType sensorType) 
:
	m_rsType(convertStreamType(sensorType)),
	m_oniType(sensorType),
	m_device(nullptr),
	m_sensor(nullptr),
	m_sensorId(-1),
	m_streamId(-1),
	m_enabled(false),
	m_depthScale(0),
	m_fovX(0),
	m_fovY(0)
{
	rsLogDebug("+Rs2Stream type=%d", (int)m_rsType);
}

Rs2Stream::~Rs2Stream()
{
	rsLogDebug("~Rs2Stream type=%d", (int)m_rsType);

	shutdown();
}

OniStatus Rs2Stream::initialize(class Rs2Device* device, rs2_sensor* sensor, int sensorId, int streamId, std::vector<Rs2StreamProfileInfo>* profiles)
{
	rsTraceFunc("type=%d sensorId=%d streamId=%d", (int)m_rsType, sensorId, streamId);

	m_device = device;
	m_sensor = sensor;
	m_sensorId = sensorId;
	m_streamId = streamId;
	m_profiles = *profiles;

	memset(&m_videoMode, 0, sizeof(m_videoMode));
	memset(&m_intrinsics, 0, sizeof(m_intrinsics));
	memset(&m_extrinsicsDepthToColor, 0, sizeof(m_extrinsicsDepthToColor));

	m_depthScale = 0;
	m_fovX = 0;
	m_fovY = 0;

	if (m_oniType == ONI_SENSOR_DEPTH) m_videoMode.pixelFormat = ONI_PIXEL_FORMAT_DEPTH_1_MM;
	else if (m_oniType == ONI_SENSOR_COLOR) m_videoMode.pixelFormat = ONI_PIXEL_FORMAT_RGB888;
	else m_videoMode.pixelFormat = ONI_PIXEL_FORMAT_GRAY8;
	
	// TODO: maybe use default sensor mode?
	m_videoMode.resolutionX = 640;
	m_videoMode.resolutionY = 480;
	m_videoMode.fps = 30;

	if (m_oniType == ONI_SENSOR_DEPTH)
	{
		// TODO: these tables should be calculated depending on sensor parameters, using hardcoded values as workaround for NITE
		setTable(S2D, sizeof(S2D), m_s2d); // XN_STREAM_PROPERTY_S2D_TABLE
		setTable(D2S, sizeof(D2S), m_d2s); // XN_STREAM_PROPERTY_D2S_TABLE
	}

	updateIntrinsics();

	return ONI_STATUS_OK;
}

void Rs2Stream::shutdown()
{
	if (m_sensor) { rsTraceFunc("type=%d sensorId=%d streamId=%d", (int)m_rsType, m_sensorId, m_streamId); }

	stop();

	if (m_sensor)
	{
		rs2_delete_sensor(m_sensor);
		m_sensor = nullptr;
	}
}

//=============================================================================
// StreamBase overrides
//=============================================================================

OniStatus Rs2Stream::start()
{
	if(!m_enabled)
	{
		rsTraceFunc("type=%d sensorId=%d streamId=%d", (int)m_rsType, m_sensorId, m_streamId);
		m_enabled = true;
		getDevice()->updateConfiguration();
	}

	return ONI_STATUS_OK;
}

void Rs2Stream::stop()
{
	if (m_enabled)
	{
		rsTraceFunc("type=%d sensorId=%d streamId=%d", (int)m_rsType, m_sensorId, m_streamId);
		m_enabled = false;
		getDevice()->updateConfiguration();
	}
}

OniStatus Rs2Stream::invoke(int commandId, void* data, int dataSize)
{
	if (commandId == RS2_PROJECT_POINT_TO_PIXEL && data && dataSize == sizeof(Rs2PointPixel))
	{
		auto pp = (Rs2PointPixel*)data;
		rs2_project_point_to_pixel(pp->pixel, &m_intrinsics, pp->point);
		return ONI_STATUS_OK;
	}

	#if defined(RS2_TRACE_NOT_SUPPORTED_CMDS)
		rsTraceError("Not supported: commandId=%d", commandId);
	#endif
	return ONI_STATUS_NOT_SUPPORTED;
}

OniBool Rs2Stream::isCommandSupported(int commandId)
{
	switch (commandId)
	{
		case RS2_PROJECT_POINT_TO_PIXEL: return true;
	}

	return false;
}

OniStatus Rs2Stream::convertDepthToColorCoordinates(StreamBase* colorStream, int depthX, int depthY, OniDepthPixel depthZ, int* pColorX, int* pColorY)
{
	SCOPED_PROFILER;

	if (m_oniType != ONI_SENSOR_DEPTH)
	{
		rsTraceError("Invalid oniType=%d", (int)m_oniType);
		return ONI_STATUS_ERROR;
	}

	if (!colorStream || ((Rs2Stream*)colorStream)->getOniType() != ONI_SENSOR_COLOR)
	{
		rsTraceError("Invalid colorStream");
		return ONI_STATUS_ERROR;
	}

	if (getDevice()->getRegistrationMode() == ONI_IMAGE_REGISTRATION_DEPTH_TO_COLOR)
	{
		*pColorX = depthX;
		*pColorY = depthY;
	}
	else
	{
		float pixel[2];
		float point[3] = {0, 0, 0};
		float transformed_point[3] = { 0 };
        if (m_needUpdateExtrinsicsDepthToColor) {
            Rs2StreamProfileInfo* spiDepth = getCurrentProfile();
            Rs2StreamProfileInfo* spiColor = ((Rs2Stream*)colorStream)->getCurrentProfile();
            rs2_get_extrinsics(spiDepth->profile, spiColor->profile, &m_extrinsicsDepthToColor, nullptr);
            m_needUpdateExtrinsicsDepthToColor = false;
        }

		// depth pixel -> point
		pixel[0] = (float)depthX;
		pixel[1] = (float)depthY;
		rs2_deproject_pixel_to_point(point, &m_intrinsics, pixel, m_depthScale * depthZ);

		// depth point -> color point
		rs2_transform_point_to_point(transformed_point, &m_extrinsicsDepthToColor, point);

		// point -> color pixel
		rs2_project_point_to_pixel(pixel, &((Rs2Stream*)colorStream)->m_intrinsics, transformed_point);
		*pColorX = (int)pixel[0];
		*pColorY = (int)pixel[1];
	}

	//rsLogDebug("depth %d %d (%d) -> color %d %d", depthX, depthY, depthZ, *pColorX, *pColorY);

	return ONI_STATUS_OK;
}

//=============================================================================
// Internals
//=============================================================================

void Rs2Stream::onStreamStarted()
{
	updateIntrinsics();
}

void Rs2Stream::onPipelineStarted()
{
}

void Rs2Stream::updateIntrinsics()
{
	Rs2StreamProfileInfo* spi = getCurrentProfile();
	rs2_get_video_stream_intrinsics(spi->profile, &m_intrinsics, nullptr);
	rs2_fov(&m_intrinsics, &m_fovX);

	if (m_oniType == ONI_SENSOR_DEPTH)
	{
		m_depthScale = rs2_get_option((const rs2_options*)m_sensor, RS2_OPTION_DEPTH_UNITS, nullptr);
	}
	else
	{
		m_depthScale = 0;
	}
	m_needUpdateExtrinsicsDepthToColor = true;
}

Rs2StreamProfileInfo* Rs2Stream::getCurrentProfile()
{
	SCOPED_PROFILER;

	rs2_format pixelFormat = convertPixelFormat(m_videoMode.pixelFormat);

	for (auto iter = m_profiles.begin(); iter != m_profiles.end(); ++iter)
	{
		Rs2StreamProfileInfo& sp = *iter;

		if (sp.streamType == m_rsType
			&& sp.format == pixelFormat
			&& sp.width == m_videoMode.resolutionX
			&& sp.height == m_videoMode.resolutionY
			&& sp.framerate == m_videoMode.fps)
		{
			return &sp;
		}
	}

	return nullptr;
}

bool Rs2Stream::isVideoModeSupported(OniVideoMode* reqMode)
{
	SCOPED_PROFILER;

	rs2_format pixelFormat = convertPixelFormat(reqMode->pixelFormat);

	for (auto iter = m_profiles.begin(); iter != m_profiles.end(); ++iter)
	{
		const Rs2StreamProfileInfo& sp = *iter;

		if (sp.streamType == m_rsType
			&& sp.format == pixelFormat
			&& sp.width == reqMode->resolutionX
			&& sp.height == reqMode->resolutionY
			&& sp.framerate == reqMode->fps)
		{
			return true;
		}
	}

	return false;
}

bool Rs2Stream::getTable(void* dst, int* size, const std::vector<uint16_t>& table)
{
	const int tableSize = (int)(table.size() * sizeof(uint16_t));
	if (dst && size && *size >= tableSize)
	{
		if (tableSize > 0)
		{
			memcpy(dst, &table[0], tableSize);
		}
		*size = tableSize;
		return true;
	}
	return false;
}

bool Rs2Stream::setTable(const void* src, int size, std::vector<uint16_t>& table)
{
	if (src && size >= 0)
	{
		const int elemCount = size / sizeof(uint16_t);
		if (elemCount > 0)
		{
			table.resize(elemCount);
			memcpy(&table[0], src, elemCount * sizeof(uint16_t));
		}
		else
		{
			table.clear();
		}
		return true;
	}
	return false;
}

}} // namespace
