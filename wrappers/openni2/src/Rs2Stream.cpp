#include "Rs2Driver.h"
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

	if (m_oniType == ONI_SENSOR_DEPTH) m_videoMode.pixelFormat = ONI_PIXEL_FORMAT_DEPTH_1_MM;
	else if (m_oniType == ONI_SENSOR_COLOR) m_videoMode.pixelFormat = ONI_PIXEL_FORMAT_RGB888;
	else m_videoMode.pixelFormat = ONI_PIXEL_FORMAT_GRAY8;
	
	m_videoMode.resolutionX = 640;
	m_videoMode.resolutionY = 480;
	m_videoMode.fps = 30;

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
	return ONI_STATUS_NOT_SUPPORTED;
}

OniBool Rs2Stream::isCommandSupported(int commandId)
{
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
		// TODO: not sure this works correctly :D
		float point[3] = {0, 0, 0};
		float pixel[2];
		pixel[0] = (float)depthX;
		pixel[1] = (float)depthY;

		rs2_deproject_pixel_to_point(point, &m_intrinsics, pixel, depthZ * m_depthScale);
		rs2_project_point_to_pixel(pixel, &((Rs2Stream*)colorStream)->m_intrinsics, point);

		*pColorX = (int)pixel[0];
		*pColorY = (int)pixel[1];
	}

	//rsLogDebug("depth %d %d (%d) -> color %d %d", depthX, depthY, depthZ, *pColorX, *pColorY);

	return ONI_STATUS_OK;
}

//=============================================================================
// Internals
//=============================================================================

void Rs2Stream::onPipelineStarted()
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

	rsLogDebug("type=%d sensorId=%d streamId=%d fovX=%f fovY=%f depthScale=%f", 
		(int)m_rsType, m_sensorId, m_streamId, m_fovX, m_fovY, m_depthScale);
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

}} // namespace
