#include "Rs2Driver.h"
#include "Rs2Commands.h"
#include <librealsense2/rsutil.h>

#define WORKER_THREAD_IDLE_MS 500
#define WORKER_THREAD_STOP_TIMEOUT_MS 5000
#define WAIT_FRAMESET_TIMEOUT_MS 2000
#define WAIT_ALIGNED_DEPTH_TIMEOUT_MS 100

namespace oni { namespace driver {

Rs2Device::Rs2Device(Rs2Driver* driver, rs2_device* device)
: 
	m_driver(driver),
	m_device(device),
	m_registrationMode(ONI_IMAGE_REGISTRATION_OFF),
	m_config(nullptr),
	m_pipeline(nullptr),
	m_pipelineProfile(nullptr),
	m_alignQueue(nullptr),
	m_alignProcessor(nullptr),
	m_runFlag(false),
	m_configId(0),
	m_framesetId(0)
{
	rsLogDebug("+Rs2Device");
}

Rs2Device::~Rs2Device()
{
	rsLogDebug("~Rs2Device");

	shutdown();
}

OniStatus Rs2Device::initialize()
{
	rsTraceFunc("");

	for (;;)
	{
		Rs2ScopedMutex lock(m_stateMx);

		if (m_thread.get())
		{
			rsTraceError("Already initialized");
			break;
		}

		if (queryDeviceInfo(m_device, &m_info) != ONI_STATUS_OK)
		{
			rsTraceError("queryDeviceInfo failed");
			break;
		}

		{
			Rs2ScopedMutex streamLock(m_streamsMx);

			if (initializeStreams() != ONI_STATUS_OK)
			{
				rsTraceError("initializeStreams failed");
				break;
			}
		}

		m_configId = 0;
		m_runFlag = true;

		try {
			m_thread.reset(new std::thread(&Rs2Device::mainLoop, this));
		}
		catch (std::exception& ex) {
			rsTraceError("std::thread failed: %s", ex.what());
			break;
		}

		return ONI_STATUS_OK;
	}

	shutdown();
	return ONI_STATUS_ERROR;
}

void Rs2Device::shutdown()
{
	if (m_device) { rsTraceFunc(""); }

	Rs2ScopedMutex lock(m_stateMx);

	m_runFlag = false;
	if (m_thread)
	{
		m_thread->join();
		m_thread = nullptr;
	}

	{
		Rs2ScopedMutex streamLock(m_streamsMx);

		for (auto iter = m_createdStreams.begin(); iter != m_createdStreams.end(); ++iter) { delete(*iter); }
		m_createdStreams.clear();

		for (auto iter = m_streams.begin(); iter != m_streams.end(); ++iter) { delete(*iter); }
		m_streams.clear();
	}

	for (auto iter = m_sensorInfo.begin(); iter != m_sensorInfo.end(); ++iter) { delete[](iter->pSupportedVideoModes); }
	m_sensorInfo.clear();
	m_profiles.clear();

	if (m_device)
	{
		rs2_delete_device(m_device);
		m_device = nullptr;
	}
}

OniStatus Rs2Device::queryDeviceInfo(rs2_device* device, OniDeviceInfo* deviceInfo)
{
	rsTraceFunc("");

	Rs2Error e;
	const char* serial = rs2_get_device_info(device, RS2_CAMERA_INFO_SERIAL_NUMBER, &e);
	if (!e.success()) return ONI_STATUS_ERROR;

	const char* name = rs2_get_device_info(device, RS2_CAMERA_INFO_NAME, &e);
	if (!e.success()) return ONI_STATUS_ERROR;

	strncpy(deviceInfo->uri, serial, sizeof(deviceInfo->uri) - 1);

	#ifdef RS2_EMULATE_PRIMESENSE_HARDWARE
		strncpy(deviceInfo->name, "PS1080", sizeof(deviceInfo->name) - 1);
		strncpy(deviceInfo->vendor, "PrimeSense", sizeof(deviceInfo->vendor) - 1);
		deviceInfo->usbVendorId = 7463;
		deviceInfo->usbProductId = 1537;
	#else
		strncpy(deviceInfo->name, name, sizeof(deviceInfo->name) - 1);
		strncpy(deviceInfo->vendor, "", sizeof(deviceInfo->vendor) - 1);
		deviceInfo->usbVendorId = 0;
		deviceInfo->usbProductId = 0;
	#endif

	rsLogDebug("DEVICE serial=%s name=%s", serial, name);

	return ONI_STATUS_OK;
}

//=============================================================================
// DeviceBase overrides
//=============================================================================

OniStatus Rs2Device::getSensorInfoList(OniSensorInfo** sensors, int* numSensors)
{
	rsTraceFunc("");

	Rs2ScopedMutex lock(m_stateMx);

	*numSensors = (int)m_sensorInfo.size();
	*sensors = ((*numSensors > 0) ? &m_sensorInfo[0] : nullptr);

	return ONI_STATUS_OK;
}

StreamBase* Rs2Device::createStream(OniSensorType sensorType)
{
	rsTraceFunc("sensorType=%d", (int)sensorType);

	Rs2ScopedMutex lock(m_streamsMx);

	for (auto iter = m_streams.begin(); iter != m_streams.end(); ++iter)
	{
		Rs2Stream* streamObj = *iter;
		if (streamObj->getOniType() == sensorType)
		{
			m_createdStreams.push_back(streamObj);
			m_streams.remove(streamObj);
			return streamObj;
		}
	}

	return nullptr;
}

void Rs2Device::destroyStream(StreamBase* streamBase)
{
	rsTraceFunc("ptr=%p", streamBase);

	if (streamBase)
	{
		Rs2ScopedMutex lock(m_streamsMx);

		Rs2Stream* streamObj = (Rs2Stream*)streamBase;
		streamObj->stop();

		m_streams.push_back(streamObj);
		m_createdStreams.remove(streamObj);
	}
}

OniStatus Rs2Device::invoke(int commandId, void* data, int dataSize)
{
	if (commandId == RS2_PROJECT_POINT_TO_PIXEL && data && dataSize == sizeof(Rs2PointPixel))
	{
		for (auto iter = m_createdStreams.begin(); iter != m_createdStreams.end(); ++iter)
		{
			Rs2Stream* stream = *iter;
			if (stream->getOniType() == ONI_SENSOR_DEPTH)
			{
				auto pp = (Rs2PointPixel*)data;
				rs2_project_point_to_pixel(pp->pixel, &stream->m_intrinsics, pp->point);
				return ONI_STATUS_OK;
			}
		}
		return ONI_STATUS_NO_DEVICE;
	}

	#if defined(RS2_TRACE_NOT_SUPPORTED_CMDS)
		rsTraceError("Not supported: commandId=%d", commandId);
	#endif
	return ONI_STATUS_NOT_SUPPORTED;
}

OniBool Rs2Device::isCommandSupported(int commandId)
{
	switch (commandId)
	{
		case RS2_PROJECT_POINT_TO_PIXEL: return true;
	}

	return false;
}

OniStatus Rs2Device::tryManualTrigger()
{
	return ONI_STATUS_OK;
}

OniBool Rs2Device::isImageRegistrationModeSupported(OniImageRegistrationMode mode)
{
	return true;
}

//=============================================================================
// Start/Stop
//=============================================================================

OniStatus Rs2Device::startPipeline()
{
	rsTraceFunc("");

	for (;;)
	{
		if (m_pipeline)
		{
			rsTraceError("Already started");
			break;
		}

		Rs2Error e;

		rsLogDebug("rs2_create_config");
		m_config = rs2_create_config(&e);
		if (!e.success())
		{
			rsTraceError("rs2_create_config failed: %s", e.get_message());
			break;
		}

		rsLogDebug("rs2_config_enable_device %s", m_info.uri);
		rs2_config_enable_device(m_config, m_info.uri, &e);
		if (!e.success())
		{
			rsTraceError("rs2_config_enable_device failed: %s", e.get_message());
			break;
		}

		rs2_config_disable_all_streams(m_config, &e);
		bool enableStreamError = false;

		{
			Rs2ScopedMutex lock(m_streamsMx);

			for (auto iter = m_createdStreams.begin(); iter != m_createdStreams.end(); ++iter)
			{
				Rs2Stream* stream = *iter;
				if (stream->isEnabled())
				{
					const OniVideoMode& mode = stream->getVideoMode();

					rsLogDebug("ENABLE STREAM type=%d sensorId=%d streamId=%d %dx%d @%d", 
						(int)stream->getRsType(), stream->getSensorId(), stream->getStreamId(), mode.resolutionX, mode.resolutionY, mode.fps);

					rs2_config_enable_stream(
						m_config, stream->getRsType(), stream->getStreamId(), 
						mode.resolutionX, mode.resolutionY, convertPixelFormat(mode.pixelFormat), mode.fps, &e
					);
					if (!e.success())
					{
						rsTraceError("rs2_config_enable_stream failed: %s", e.get_message());
						enableStreamError = true;
						break;
					}

					stream->onStreamStarted();
				}
			}
		}

		if (enableStreamError)
		{
			rsTraceError("enable_stream error");
			break;
		}

		// pipeline

		rs2_context* context = getDriver()->getRsContext();
		rsLogDebug("rs2_create_pipeline");
		m_pipeline = rs2_create_pipeline(context, &e);
		if (!e.success())
		{
			rsTraceError("rs2_create_pipeline failed: %s", e.get_message());
			break;
		}

		rsLogDebug("rs2_pipeline_start_with_config");
		m_pipelineProfile = rs2_pipeline_start_with_config(m_pipeline, m_config, &e);
		if (!e.success())
		{
			rsTraceError("rs2_pipeline_start_with_config failed: %s", e.get_message());
			break;
		}

		{
			Rs2ScopedMutex lock(m_streamsMx);

			for (auto iter = m_createdStreams.begin(); iter != m_createdStreams.end(); ++iter)
			{
				Rs2Stream* stream = *iter;
				if (stream->isEnabled())
				{
					stream->onPipelineStarted();
				}
			}
		}

		// depth to color aligner

		m_alignQueue = rs2_create_frame_queue(1, &e);
		if (!e.success())
		{
			rsTraceError("rs2_create_frame_queue failed: %s", e.get_message());
			break;
		}

		m_alignProcessor = rs2_create_align(RS2_STREAM_COLOR, &e);
		if (!e.success())
		{
			rsTraceError("rs2_create_align failed: %s", e.get_message());
			break;
		}

		rs2_start_processing_queue(m_alignProcessor, m_alignQueue, &e);
		if (!e.success())
		{
			rsTraceError("rs2_start_processing_queue failed: %s", e.get_message());
			break;
		}

		rsLogDebug("STARTED");
		return ONI_STATUS_OK;
	}

	stopPipeline();
	return ONI_STATUS_ERROR;
}

void Rs2Device::stopPipeline()
{
	if (m_pipeline) { rsTraceFunc(""); }

	if (m_pipeline)
	{
		Rs2Error e;
		rsLogDebug("rs2_pipeline_stop");
		rs2_pipeline_stop(m_pipeline, &e);
	}
	if (m_alignProcessor)
	{
		rs2_delete_processing_block(m_alignProcessor);
		m_alignProcessor = nullptr;
	}
	if (m_alignQueue)
	{
		rs2_delete_frame_queue(m_alignQueue);
		m_alignQueue = nullptr;
	}
	if (m_pipelineProfile)
	{
		rsLogDebug("rs2_delete_pipeline_profile");
		rs2_delete_pipeline_profile(m_pipelineProfile);
		m_pipelineProfile = nullptr;
	}
	if (m_config)
	{
		rsLogDebug("rs2_delete_config");
		rs2_delete_config(m_config);
		m_config = nullptr;
	}
	if (m_pipeline)
	{
		rsLogDebug("rs2_delete_pipeline");
		rs2_delete_pipeline(m_pipeline);
		m_pipeline = nullptr;
		rsLogDebug("STOPPED");
	}
}

void Rs2Device::restartPipeline()
{
	rsTraceFunc("");

	stopPipeline();

	bool hasStreams;
	{
		Rs2ScopedMutex lock(m_streamsMx);
		hasStreams = hasEnabledStreams();
	}

	if (hasStreams && m_runFlag)
	{
		startPipeline();
	}
}

//=============================================================================
// MainLoop
//=============================================================================

void Rs2Device::mainLoop()
{
	rsTraceFunc("");

	try
	{
		int configId = 0;
		while (m_runFlag)
		{
			const int curConfigId = m_configId;
			if (configId != curConfigId) // configuration changed since last tick
			{
				configId = curConfigId;
				restartPipeline();
			}

			if (m_pipelineProfile)
			{
				waitForFrames();
			}
			else
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(WORKER_THREAD_IDLE_MS));
			}
		}
	}
	catch (...)
	{
		rsTraceError("Unhandled exception");
	}

	stopPipeline();
}

void Rs2Device::updateConfiguration()
{
	rsTraceFunc("");

	m_configId++;
}

void Rs2Device::waitForFrames()
{
	SCOPED_PROFILER;

	Rs2Error e;
	rs2_frame* frameset;
	{
		NAMED_PROFILER("rs2_pipeline_wait_for_frames");
		frameset = rs2_pipeline_wait_for_frames(m_pipeline, WAIT_FRAMESET_TIMEOUT_MS, &e);
	}
	if (!e.success())
	{
		rsTraceError("rs2_pipeline_wait_for_frames failed: %s", e.get_message());
		return;
	}

	const int nframes = rs2_embedded_frames_count(frameset, &e);
	//rsLogDebug("frameset %llu (%d)", m_framesetId, nframes);

	if (m_registrationMode == ONI_IMAGE_REGISTRATION_DEPTH_TO_COLOR)
	{
		rs2_frame_add_ref(frameset, &e);
		{
			NAMED_PROFILER("rs2_process_frame");
			rs2_process_frame(m_alignProcessor, frameset, &e);
			if (!e.success())
			{
				rsTraceError("rs2_process_frame failed: %s", e.get_message());
			}
		}
	}

	for (int i = 0; i < nframes; ++i)
    {
		rs2_frame* frame = rs2_extract_frame(frameset, i, &e);
		if (frame)
		{
			processFrame(frame);
			releaseFrame(frame);
		}
	}

	releaseFrame(frameset);
	++m_framesetId;

	if (m_registrationMode == ONI_IMAGE_REGISTRATION_DEPTH_TO_COLOR)
	{
		waitAlignedDepth();
	}
}

void Rs2Device::processFrame(rs2_frame* frame)
{
	SCOPED_PROFILER;

	Rs2StreamProfileInfo spi;
	Rs2Stream* stream = getFrameStream(frame, &spi);
	if (stream)
	{
		if (m_registrationMode == ONI_IMAGE_REGISTRATION_OFF || spi.streamType != RS2_STREAM_DEPTH)
		{
			OniFrame* oniFrame = createOniFrame(frame, stream, &spi);
			if (oniFrame)
			{
				submitOniFrame(oniFrame, stream);
			}
		}
	}
}

void Rs2Device::waitAlignedDepth()
{
	SCOPED_PROFILER;

	Rs2Error e;
	rs2_frame* frameset;
	{
		NAMED_PROFILER("rs2_wait_for_frame");
		frameset = rs2_wait_for_frame(m_alignQueue, WAIT_ALIGNED_DEPTH_TIMEOUT_MS, &e);
	}
	if (!e.success())
	{
		rsTraceError("rs2_wait_for_frame failed: %s", e.get_message());
		return;
	}

	const int nframes = rs2_embedded_frames_count(frameset, &e);
	for (int i = 0; i < nframes; ++i)
	{
		rs2_frame* frame = rs2_extract_frame(frameset, i, &e);
		if (frame)
		{
			Rs2StreamProfileInfo spi;
			Rs2Stream* stream = getFrameStream(frame, &spi);
			if (stream && spi.streamType == RS2_STREAM_DEPTH)
			{
				OniFrame* oniFrame = createOniFrame(frame, stream, &spi);
				if (oniFrame)
				{
					submitOniFrame(oniFrame, stream);
				}
			}
			releaseFrame(frame);
		}
	}

	releaseFrame(frameset);
}

OniFrame* Rs2Device::createOniFrame(rs2_frame* frame, Rs2Stream* stream, Rs2StreamProfileInfo* spi)
{
	SCOPED_PROFILER;

	if (!frame || !stream || !stream->isEnabled() || !spi)
	{
		return nullptr;
	}

	Rs2Error e;
	OniFrame* oniFrame;
	{
		NAMED_PROFILER("StreamServices::acquireFrame");
		oniFrame = stream->getServices().acquireFrame();
		if (!oniFrame)
		{
			rsTraceError("acquireFrame failed");
			return nullptr;
		}
	}

	oniFrame->sensorType = stream->getOniType();
	oniFrame->timestamp = (uint64_t)(rs2_get_frame_timestamp(frame, &e) * 1000.0); // millisecond to microsecond
	oniFrame->frameIndex = (int)rs2_get_frame_number(frame, &e);

	oniFrame->width = rs2_get_frame_width(frame, &e);
	oniFrame->height = rs2_get_frame_height(frame, &e);
	oniFrame->stride = rs2_get_frame_stride_in_bytes(frame, &e);
	const size_t frameSize = oniFrame->stride * oniFrame->height;

	OniVideoMode mode;
	mode.pixelFormat = convertPixelFormat(spi->format);
	mode.resolutionX = oniFrame->width;
	mode.resolutionY = oniFrame->height;
	mode.fps = spi->framerate;

	oniFrame->videoMode = mode;
	oniFrame->croppingEnabled = false;
	oniFrame->cropOriginX = 0;
	oniFrame->cropOriginY = 0;

	if (frameSize != oniFrame->dataSize)
	{
		rsTraceError("invalid frame: rsSize=%u oniSize=%u", (unsigned int)frameSize, (unsigned int)oniFrame->dataSize);
		stream->getServices().releaseFrame(oniFrame);
		return nullptr;
	}

	const void* frameData = rs2_get_frame_data(frame, &e);
	if (!e.success())
	{
		rsTraceError("rs2_get_frame_data failed: %s", e.get_message());
		stream->getServices().releaseFrame(oniFrame);
		return nullptr;
	}
	
	{
		NAMED_PROFILER("_copyFrameData");
		memcpy(oniFrame->data, frameData, frameSize);
	}

	return oniFrame;
}

void Rs2Device::submitOniFrame(OniFrame* oniFrame, Rs2Stream* stream)
{
	SCOPED_PROFILER;

	if (stream->getOniType() == ONI_SENSOR_DEPTH) // HACK: clamp depth to OpenNI hardcoded max value
	{
		NAMED_PROFILER("_clampDepth");
		uint16_t* depth = (uint16_t*)oniFrame->data;
		for (int i = 0; i < oniFrame->width * oniFrame->height; ++i)
		{
			if (*depth >= ONI_MAX_DEPTH) { *depth = ONI_MAX_DEPTH - 1; }
			++depth;
		}
	}
	{
		NAMED_PROFILER("StreamServices::raiseNewFrame");
		stream->raiseNewFrame(oniFrame);
	}
	{
		NAMED_PROFILER("StreamServices::releaseFrame");
		stream->getServices().releaseFrame(oniFrame);
	}
}

Rs2Stream* Rs2Device::getFrameStream(rs2_frame* frame, Rs2StreamProfileInfo* spi)
{
	SCOPED_PROFILER;

	Rs2Error e;
	const rs2_stream_profile* profile = rs2_get_frame_stream_profile(frame, &e);
	if (e.success())
	{
		memset(spi, 0, sizeof(Rs2StreamProfileInfo));
		rs2_get_stream_profile_data(profile, &spi->streamType, &spi->format, &spi->streamId, &spi->profileId, &spi->framerate, &e);
		if (e.success())
		{
			OniSensorType sensorType = convertStreamType(spi->streamType);
			{
				Rs2ScopedMutex lock(m_streamsMx);
				return findStream(sensorType, spi->streamId);
			}
		}
	}
	return nullptr;
}

void Rs2Device::releaseFrame(rs2_frame* frame)
{
	NAMED_PROFILER("rs2_release_frame");
	rs2_release_frame(frame);
}

//=============================================================================
// Stream initialization
//=============================================================================

OniStatus Rs2Device::initializeStreams()
{
	rsTraceFunc("");

	std::map<int, rs2_stream> sensorStreams;
	
	Rs2Error e;
	rs2_sensor_list* sensorList = rs2_query_sensors(m_device, &e);
	if (sensorList)
	{
		const int nsensors = rs2_get_sensors_count(sensorList, &e);
		for (int sensorId = 0; sensorId < nsensors; sensorId++)
		{
			rsLogDebug("SENSOR %d", sensorId);

			rs2_sensor* sensor = rs2_create_sensor(sensorList, sensorId, &e);
			if (sensor)
			{
				sensorStreams.clear();

				rs2_stream_profile_list* profileList = rs2_get_stream_profiles(sensor, &e);
				if (profileList)
				{
					const int nprofiles = rs2_get_stream_profiles_count(profileList, &e);
					for (int profileId = 0; profileId < nprofiles; profileId++)
					{
						const rs2_stream_profile* profile = rs2_get_stream_profile(profileList, profileId, &e);
						if (profile)
						{
							Rs2StreamProfileInfo spi;
							spi.profile = profile;
							spi.sensorId = sensorId;
							rs2_get_stream_profile_data(profile, &spi.streamType, &spi.format, &spi.streamId, &spi.profileId, &spi.framerate, &e);

							if (e.success() && isSupportedStreamType(spi.streamType) && isSupportedPixelFormat(spi.format))
							{
								rs2_get_video_stream_resolution(profile, &spi.width, &spi.height, &e);
								if (e.success())
								{
									#if 1
									rsLogDebug("\ttype=%d sensorId=%d streamId=%d profileId=%d format=%d width=%d height=%d framerate=%d", 
										(int)spi.streamType, (int)spi.sensorId, (int)spi.streamId, (int)spi.profileId, (int)spi.format, (int)spi.width, (int)spi.height, (int)spi.framerate);
									#endif

									m_profiles.push_back(spi);
									sensorStreams[spi.streamId] = spi.streamType;
								}
							}
						}
					}
					rs2_delete_stream_profiles_list(profileList);
				}

				for (auto iter = sensorStreams.begin(); iter != sensorStreams.end(); ++iter)
				{
					rsLogDebug("UNIQ streamId (%d) -> type (%d)", iter->first, (int)iter->second);
				}

				for (auto iter = sensorStreams.begin(); iter != sensorStreams.end(); ++iter)
				{
					const OniSensorType oniType = convertStreamType(iter->second);

					std::vector<Rs2StreamProfileInfo> profiles;
					findStreamProfiles(&profiles, oniType, iter->first);

					if (addStream(sensor, oniType, sensorId, iter->first, &profiles) == ONI_STATUS_OK)
					{
						sensor = nullptr;
					}
				}

				if (sensor) { rs2_delete_sensor(sensor); }
			}
		}
		rs2_delete_sensor_list(sensorList);
	}

	rsLogDebug("FILL OniSensorInfo");
	for (auto iter = m_streams.begin(); iter != m_streams.end(); ++iter)
	{
		Rs2Stream* stream = *iter;
		#if 1
		rsLogDebug("STREAM type=%d sensorId=%d streamId=%d", (int)stream->getRsType(), stream->getSensorId(), stream->getStreamId());
		#endif

		std::vector<Rs2StreamProfileInfo> profiles;
		findStreamProfiles(&profiles, stream->getOniType(), stream->getStreamId());

		OniSensorInfo info;
		info.sensorType = stream->getOniType();
		info.numSupportedVideoModes = (int)profiles.size();
		info.pSupportedVideoModes = nullptr;

		if (info.numSupportedVideoModes > 0)
		{
			info.pSupportedVideoModes = new OniVideoMode[info.numSupportedVideoModes];
			int modeId = 0;

			for (auto p = profiles.begin(); p != profiles.end(); ++p)
			{
				OniVideoMode& mode = info.pSupportedVideoModes[modeId];
				mode.pixelFormat = convertPixelFormat(p->format);
				mode.resolutionX = p->width;
				mode.resolutionY = p->height;
				mode.fps = p->framerate;
				modeId++;

				#if 1
				rsLogDebug("\ttype=%d sensorId=%d streamId=%d profileId=%d format=%d width=%d height=%d framerate=%d",
					(int)p->streamType, (int)p->sensorId, (int)p->streamId, (int)p->profileId, (int)p->format, (int)p->width, (int)p->height, (int)p->framerate);
				#endif
			}

			m_sensorInfo.push_back(info);
		}
	}

	return ONI_STATUS_OK;
}

OniStatus Rs2Device::addStream(rs2_sensor* sensor, OniSensorType sensorType, int sensorId, int streamId, std::vector<Rs2StreamProfileInfo>* profiles)
{
	rsTraceFunc("type=%d sensorId=%d streamId=%d", (int)convertStreamType(sensorType), sensorId, streamId);

	Rs2Stream* streamObj = nullptr;
	switch (sensorType)
	{
		case ONI_SENSOR_IR: streamObj = new Rs2InfraredStream(); break;
		case ONI_SENSOR_COLOR: streamObj = new Rs2ColorStream(); break;
		case ONI_SENSOR_DEPTH: streamObj = new Rs2DepthStream(); break;

		default:
		{
			rsTraceError("Invalid type=%d", (int)sensorType);
			return ONI_STATUS_ERROR;
		}
	}

	if (streamObj->initialize(this, sensor, sensorId, streamId, profiles) != ONI_STATUS_OK)
	{
		rsTraceError("Rs2Stream::initialize failed");
		delete(streamObj);
		return ONI_STATUS_ERROR;
	}

	m_streams.push_back(streamObj);
	return ONI_STATUS_OK;
}

void Rs2Device::findStreamProfiles(std::vector<Rs2StreamProfileInfo>* dst, OniSensorType sensorType, int streamId)
{
	const rs2_stream rsType = convertStreamType(sensorType);

	for (auto iter = m_profiles.begin(); iter != m_profiles.end(); ++iter)
	{
		Rs2StreamProfileInfo& p = *iter;
		if (p.streamType == rsType && p.streamId == streamId)
		{
			dst->push_back(p);
		}
	}
}

Rs2Stream* Rs2Device::findStream(OniSensorType sensorType, int streamId)
{
	for (auto iter = m_createdStreams.begin(); iter != m_createdStreams.end(); ++iter)
	{
		Rs2Stream* stream = *iter;
		if (stream->getOniType() == sensorType && stream->getStreamId() == streamId)
		{
			return stream;
		}
	}

	return nullptr;
}

bool Rs2Device::hasEnabledStreams()
{
	for (auto iter = m_createdStreams.begin(); iter != m_createdStreams.end(); ++iter)
	{
		Rs2Stream* stream = *iter;
		if (stream->isEnabled())
		{
			return true;
		}
	}

	return false;
}

}} // namespace
