#include "Rs2Driver.h"

#if defined(PROF_ENABLED)
#include "Profiler.inl"
#endif

namespace oni { namespace driver {

Rs2Driver::Rs2Driver(OniDriverServices* driverServices)
:
	DriverBase(driverServices),
	m_context(nullptr)
{
	rsLogDebug("+Rs2Driver");
	INIT_PROFILER;
}

Rs2Driver::~Rs2Driver()
{
	rsLogDebug("~Rs2Driver");
	shutdown();
	SHUT_PROFILER;
}

OniStatus Rs2Driver::initialize(
	DeviceConnectedCallback connectedCallback, 
	DeviceDisconnectedCallback disconnectedCallback, 
	DeviceStateChangedCallback deviceStateChangedCallback, 
	void* cookie)
{
	rsTraceFunc("");

	for (;;)
	{
		Rs2ScopedMutex lock(m_stateMx);

		if (m_context)
		{
			rsTraceError("Already initialized");
			break;
		}

		if (DriverBase::initialize(connectedCallback, disconnectedCallback, deviceStateChangedCallback, cookie) != ONI_STATUS_OK)
		{
			rsTraceError("DriverBase::initialize failed");
			break;
		}

		Rs2Error e;
		m_context = rs2_create_context(RS2_API_VERSION, &e);
		if (!e.success())
		{
			rsTraceError("rs2_create_context failed: %s", e.get_message());
			break;
		}

		enumerateDevices();

		rs2_set_devices_changed_callback(m_context, devicesChangedCallback, this, &e);
		if (!e.success())
		{
			rsTraceError("rs2_set_devices_changed_callback failed: %s", e.get_message());
			break;
		}

		rsLogDebug("Rs2Driver INITIALIZED");
		return ONI_STATUS_OK;
	}

	shutdown();
	return ONI_STATUS_ERROR;
}

void Rs2Driver::shutdown()
{
	if (m_context) { rsTraceFunc(""); }

	Rs2ScopedMutex lock(m_stateMx);

	{
		Rs2ScopedMutex devLock(m_devicesMx);
		for (auto iter = m_devices.begin(); iter != m_devices.end(); ++iter) { delete(iter->second); }
		m_devices.clear();
	}

	if (m_context)
	{
		rs2_delete_context(m_context);
		m_context = nullptr;
	}
}

void Rs2Driver::enumerateDevices()
{
	rsTraceFunc("");

	Rs2Error e;
	rs2_device_list* deviceList = rs2_query_devices(m_context, &e);
	if (!e.success())
	{
		rsTraceError("rs2_query_devices failed: %s", e.get_message());
	}
	else
	{
		devicesChanged(nullptr, deviceList);
		rs2_delete_device_list(deviceList);
	}
}

void Rs2Driver::devicesChangedCallback(rs2_device_list* removed, rs2_device_list* added, void* param)
{
	((Rs2Driver*)param)->devicesChanged(removed, added);
}

void Rs2Driver::devicesChanged(rs2_device_list* removed, rs2_device_list* added)
{
	rsTraceFunc("removed=%p added=%p", removed, added);

	Rs2ScopedMutex lock(m_devicesMx);
	Rs2Error e;

	if (removed)
	{
		std::list<Rs2Device*> removedList;

		for (auto iter = m_devices.begin(); iter != m_devices.end(); ++iter)
		{
			if (rs2_device_list_contains(removed, iter->second->getRsDevice(), &e))
			{
				removedList.push_back(iter->second);
			}
		}

		for (auto iter = removedList.begin(); iter != removedList.end(); ++iter)
		{
			rsLogDebug("deviceDisconnected");
			deviceDisconnected((*iter)->getInfo());
		}
	}

	if (added)
	{
		const int count = rs2_get_device_count(added, &e);
		for (int i = 0; i < count; i++)
		{
			rs2_device* device = rs2_create_device(added, i, &e);
			if (!e.success())
			{
				rsTraceError("rs2_create_device failed: %s", e.get_message());
			}
			else
			{
				rsLogDebug("deviceConnected");
				OniDeviceInfo info;
				Rs2Device::queryDeviceInfo(device, &info);
				deviceConnected(&info);
				rs2_delete_device(device);
			}
		}
	}
}

DeviceBase* Rs2Driver::deviceOpen(const char* uri, const char* mode)
{
	rsTraceFunc("uri=%s", uri);

	rs2_device_list* deviceList = nullptr;
	rs2_device* device = nullptr;
	Rs2Device* deviceObj = nullptr;

	for (;;)
	{
		Rs2ScopedMutex lock(m_devicesMx);

		if (m_devices.find(uri) != m_devices.end())
		{
			rsTraceError("Already opened");
			break;
		}

		Rs2Error e;
		deviceList = rs2_query_devices(m_context, &e);
		if (!e.success())
		{
			rsTraceError("rs2_query_devices failed: %s", e.get_message());
			break;
		}

		const int count = rs2_get_device_count(deviceList, &e);
		for (int i = 0; i < count; i++)
		{
			if (device) { rs2_delete_device(device); }

			device = rs2_create_device(deviceList, i, &e);
			if (!e.success())
			{
				rsTraceError("rs2_create_device failed: %s", e.get_message());
				break;
			}

			const char* serial = rs2_get_device_info(device, RS2_CAMERA_INFO_SERIAL_NUMBER, &e);
			if (!e.success())
			{
				rsTraceError("rs2_get_device_info failed: %s", e.get_message());
				break;
			}

			if (strcmp(uri, serial) == 0)
			{
				deviceObj = new Rs2Device(this, device);
				if (deviceObj->initialize() != ONI_STATUS_OK)
				{
					rsTraceError("Rs2Device::initialize failed");
					delete(deviceObj);
					deviceObj = nullptr;
				}
				else
				{
					m_devices[serial] = deviceObj;
					device = nullptr; // don't release device handle
				}
				break;
			}
		}

		break;
	}

	if (device) { rs2_delete_device(device); }
	if (deviceList) { rs2_delete_device_list(deviceList); }

	return deviceObj;
}

void Rs2Driver::deviceClose(DeviceBase* deviceBase)
{
	rsTraceFunc("ptr=%p", deviceBase);

	if (deviceBase)
	{
		Rs2ScopedMutex lock(m_devicesMx);

		Rs2Device* deviceObj = (Rs2Device*)deviceBase;
		m_devices.erase(deviceObj->getInfo()->uri);
		delete(deviceObj);
	}
}

OniStatus Rs2Driver::tryDevice(const char* uri)
{
	rsTraceFunc("uri=%s", uri);

	return ONI_STATUS_ERROR;
}

void* Rs2Driver::enableFrameSync(StreamBase** streams, int streamCount)
{
	rsTraceError("FrameSync not supported");

	return nullptr;
}

void Rs2Driver::disableFrameSync(void* frameSyncGroup)
{
	rsTraceError("FrameSync not supported");
}

#if !defined(XN_NEW)
#define XN_NEW(type, arg) new type(arg)
#endif

#if !defined(XN_DELETE)
#define XN_DELETE(arg) delete arg
#endif

ONI_EXPORT_DRIVER(Rs2Driver);

}} // namespace
