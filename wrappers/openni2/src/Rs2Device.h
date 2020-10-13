#pragma once

#include "Rs2Stream.h"

namespace oni { namespace driver {

class Rs2Device : public DeviceBase
{
	friend class Rs2Driver;

public:

	Rs2Device(class Rs2Driver* driver, rs2_device* device);
	virtual ~Rs2Device();

	virtual OniStatus getSensorInfoList(OniSensorInfo** sensors, int* numSensors);
	virtual StreamBase* createStream(OniSensorType sensorType);
	virtual void destroyStream(StreamBase* streamBase);

	virtual OniStatus setProperty(int propertyId, const void* data, int dataSize);
	virtual OniStatus getProperty(int propertyId, void* data, int* dataSize);
	virtual OniBool isPropertySupported(int propertyId);

	virtual OniStatus invoke(int commandId, void* data, int dataSize);
	virtual OniBool isCommandSupported(int commandId);
	virtual OniStatus tryManualTrigger();

	virtual OniBool isImageRegistrationModeSupported(OniImageRegistrationMode mode);

	inline class Rs2Driver* getDriver() { return m_driver; }
	inline rs2_device* getRsDevice() { return m_device; }
	inline OniDeviceInfo* getInfo() { return &m_info; }
	inline OniImageRegistrationMode getRegistrationMode() const { return m_registrationMode; }

	void updateConfiguration();

protected:

	Rs2Device(const Rs2Device&);
	void operator=(const Rs2Device&);

	OniStatus initialize();
	void shutdown();

	static OniStatus queryDeviceInfo(rs2_device* device, OniDeviceInfo* deviceInfo);

	OniStatus initializeStreams();
	OniStatus addStream(rs2_sensor* sensor, OniSensorType sensorType, int sensorId, int streamId, std::vector<Rs2StreamProfileInfo>* profiles);
	void findStreamProfiles(std::vector<Rs2StreamProfileInfo>* dst, OniSensorType sensorType, int streamId);
	Rs2Stream* findStream(OniSensorType sensorType, int streamId);
	bool hasEnabledStreams();

	OniStatus startPipeline();
	void stopPipeline();
	void restartPipeline();

	void mainLoop();
	void waitForFrames();
	void processFrame(rs2_frame* frame);
	void waitAlignedDepth();

	OniFrame* createOniFrame(rs2_frame* frame, Rs2Stream* stream, Rs2StreamProfileInfo* spi);
	void submitOniFrame(OniFrame* oniFrame, Rs2Stream* stream);

	Rs2Stream* getFrameStream(rs2_frame* frame, Rs2StreamProfileInfo* spi);
	void releaseFrame(rs2_frame* frame);

protected:

	Rs2Mutex m_stateMx;
	Rs2Mutex m_streamsMx;

	class Rs2Driver* m_driver;
	rs2_device* m_device;
	OniImageRegistrationMode m_registrationMode;

	rs2_config* m_config;
	rs2_pipeline* m_pipeline;
	rs2_pipeline_profile* m_pipelineProfile;

	rs2_frame_queue* m_alignQueue;
	rs2_processing_block* m_alignProcessor;

	std::unique_ptr<std::thread> m_thread;
	volatile int m_runFlag;
	volatile int m_configId;
	uint64_t m_framesetId;

	OniDeviceInfo m_info;
	std::vector<Rs2StreamProfileInfo> m_profiles;
	std::vector<OniSensorInfo> m_sensorInfo;
	std::list<class Rs2Stream*> m_streams;
	std::list<class Rs2Stream*> m_createdStreams;
};

}} // namespace
