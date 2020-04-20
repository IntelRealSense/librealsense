#pragma once

#include "Rs2Base.h"

namespace oni { namespace driver {

struct Rs2StreamProfileInfo
{
	const rs2_stream_profile* profile;
	int sensorId;
	int profileId;
	int streamId;
	rs2_stream streamType;
	rs2_format format;
	int stride;
	int width;
	int height;
	int framerate;
};

class Rs2Stream : public StreamBase
{
	friend class Rs2Device;

public:

	Rs2Stream(OniSensorType sensorType);
	virtual ~Rs2Stream();

	virtual OniStatus setProperty(int propertyId, const void* data, int dataSize);
	virtual OniStatus getProperty(int propertyId, void* data, int* dataSize);
	virtual OniBool isPropertySupported(int propertyId);

	virtual OniStatus invoke(int commandId, void* data, int dataSize);
	virtual OniBool isCommandSupported(int commandId);

	virtual OniStatus start();
	virtual void stop();

	virtual OniStatus convertDepthToColorCoordinates(StreamBase* colorStream, int depthX, int depthY, OniDepthPixel depthZ, int* pColorX, int* pColorY);

	inline class Rs2Device* getDevice() { return m_device; }
	inline rs2_stream getRsType() const { return m_rsType; }
	inline OniSensorType getOniType() const { return m_oniType; }
	inline int getSensorId() const { return m_sensorId; }
	inline int getStreamId() const { return m_streamId; }
	inline const OniVideoMode& getVideoMode() const { return m_videoMode; }
	inline bool isEnabled() const { return m_enabled; }

protected:

	Rs2Stream(const Rs2Stream&);
	void operator=(const Rs2Stream&);

	OniStatus initialize(class Rs2Device* device, rs2_sensor* sensor, int sensorId, int streamId, std::vector<Rs2StreamProfileInfo>* profiles);
	void shutdown();

	void onStreamStarted();
	void onPipelineStarted();

	void updateIntrinsics();
	Rs2StreamProfileInfo* getCurrentProfile();
	bool isVideoModeSupported(OniVideoMode* reqMode);

	bool getTable(void* dst, int* size, const std::vector<uint16_t>& table);
	bool setTable(const void* src, int size, std::vector<uint16_t>& table);

protected:

	rs2_stream m_rsType;
	OniSensorType m_oniType;

	class Rs2Device* m_device;
	rs2_sensor* m_sensor;
	int m_sensorId;
	int m_streamId;
	bool m_enabled;

	std::vector<Rs2StreamProfileInfo> m_profiles;
	OniVideoMode m_videoMode;
	rs2_intrinsics m_intrinsics;
	rs2_extrinsics m_extrinsicsDepthToColor;
	bool m_needUpdateExtrinsicsDepthToColor;
	float m_depthScale;
	float m_fovX;
	float m_fovY;

	std::vector<uint16_t> m_s2d;
	std::vector<uint16_t> m_d2s;
};

class Rs2DepthStream : public Rs2Stream
{
public:
	Rs2DepthStream() : Rs2Stream(ONI_SENSOR_DEPTH) {}
};

class Rs2ColorStream : public Rs2Stream
{
public:
	Rs2ColorStream() : Rs2Stream(ONI_SENSOR_COLOR) {}
};

class Rs2InfraredStream : public Rs2Stream
{
public:
	Rs2InfraredStream() : Rs2Stream(ONI_SENSOR_IR) {}
};

}} // namespace
