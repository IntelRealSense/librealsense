#pragma once

#include "Rs2Device.h"

namespace oni { namespace driver {

class Rs2Driver : public DriverBase
{
public:

	Rs2Driver(OniDriverServices* driverServices);
	virtual ~Rs2Driver();

	virtual OniStatus initialize(
		DeviceConnectedCallback connectedCallback, 
		DeviceDisconnectedCallback disconnectedCallback, 
		DeviceStateChangedCallback deviceStateChangedCallback, 
		void* cookie);

	virtual void shutdown();

	static void devicesChangedCallback(rs2_device_list* removed, rs2_device_list* added, void* param);
	virtual void devicesChanged(rs2_device_list* removed, rs2_device_list* added);

	virtual DeviceBase* deviceOpen(const char* uri, const char* mode);
	virtual void deviceClose(DeviceBase* deviceBase);
	virtual OniStatus tryDevice(const char* uri);

	virtual void* enableFrameSync(StreamBase** streams, int streamCount);
	virtual void disableFrameSync(void* frameSyncGroup);

	inline rs2_context* getRsContext() { return m_context; }

protected:

	Rs2Driver(const Rs2Driver&);
	void operator=(const Rs2Driver&);

	void enumerateDevices();

protected:

	Rs2Mutex m_stateMx;
	Rs2Mutex m_devicesMx;
	rs2_context* m_context;
	std::map<std::string, class Rs2Device*> m_devices;
};

}} // namespace
