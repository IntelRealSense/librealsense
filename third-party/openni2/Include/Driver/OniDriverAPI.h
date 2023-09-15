/*****************************************************************************
*                                                                            *
*  OpenNI 2.x Alpha                                                          *
*  Copyright (C) 2012 PrimeSense Ltd.                                        *
*                                                                            *
*  This file is part of OpenNI.                                              *
*                                                                            *
*  Licensed under the Apache License, Version 2.0 (the "License");           *
*  you may not use this file except in compliance with the License.          *
*  You may obtain a copy of the License at                                   *
*                                                                            *
*      http://www.apache.org/licenses/LICENSE-2.0                            *
*                                                                            *
*  Unless required by applicable law or agreed to in writing, software       *
*  distributed under the License is distributed on an "AS IS" BASIS,         *
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  *
*  See the License for the specific language governing permissions and       *
*  limitations under the License.                                            *
*                                                                            *
*****************************************************************************/
#ifndef _ONI_DRIVER_API_H_
#define _ONI_DRIVER_API_H_

#include "OniPlatform.h"
#include "OniCTypes.h"
#include "OniCProperties.h"
#include "OniDriverTypes.h"
#include <stdarg.h>

namespace oni { namespace driver {

class DeviceBase;
class StreamBase;

typedef void (ONI_CALLBACK_TYPE* DeviceConnectedCallback)(const OniDeviceInfo*, void* pCookie);
typedef void (ONI_CALLBACK_TYPE* DeviceDisconnectedCallback)(const OniDeviceInfo*, void* pCookie);
typedef void (ONI_CALLBACK_TYPE* DeviceStateChangedCallback)(const OniDeviceInfo* deviceId, int errorState, void* pCookie);
typedef void (ONI_CALLBACK_TYPE* NewFrameCallback)(StreamBase* streamId, OniFrame*, void* pCookie);
typedef void (ONI_CALLBACK_TYPE* PropertyChangedCallback)(void* sender, int propertyId, const void* data, int dataSize, void* pCookie);

class StreamServices : public OniStreamServices
{
public:
	int getDefaultRequiredFrameSize()
	{
		return OniStreamServices::getDefaultRequiredFrameSize(streamServices);
	}

	OniFrame* acquireFrame()
	{
		return OniStreamServices::acquireFrame(streamServices);
	}

	void addFrameRef(OniFrame* pFrame)
	{
		OniStreamServices::addFrameRef(streamServices, pFrame);
	}

	void releaseFrame(OniFrame* pFrame)
	{
		OniStreamServices::releaseFrame(streamServices, pFrame);
	}
};

class StreamBase
{
public:
	StreamBase() : m_newFrameCallback(NULL), m_propertyChangedCallback(NULL) {}
	virtual ~StreamBase() {}

	virtual void setServices(StreamServices* pStreamServices) { m_pServices = pStreamServices; }

	virtual OniStatus setProperty(int /*propertyId*/, const void* /*data*/, int /*dataSize*/) {return ONI_STATUS_NOT_IMPLEMENTED;}
	virtual OniStatus getProperty(int /*propertyId*/, void* /*data*/, int* /*pDataSize*/) {return ONI_STATUS_NOT_IMPLEMENTED;}
	virtual OniBool isPropertySupported(int /*propertyId*/) {return FALSE;}
	virtual OniStatus invoke(int /*commandId*/, void* /*data*/, int /*dataSize*/) {return ONI_STATUS_NOT_IMPLEMENTED;}
	virtual OniBool isCommandSupported(int /*commandId*/) {return FALSE;}

	virtual int getRequiredFrameSize() { return getServices().getDefaultRequiredFrameSize(); }

	virtual OniStatus start() = 0;
	virtual void stop() = 0;

	virtual void setNewFrameCallback(NewFrameCallback handler, void* pCookie) { m_newFrameCallback = handler; m_newFrameCallbackCookie = pCookie; }
	virtual void setPropertyChangedCallback(PropertyChangedCallback handler, void* pCookie) { m_propertyChangedCallback = handler; m_propertyChangedCookie = pCookie; }

	virtual void notifyAllProperties() { return; }

	virtual OniStatus convertDepthToColorCoordinates(StreamBase* /*colorStream*/, int /*depthX*/, int /*depthY*/, OniDepthPixel /*depthZ*/, int* /*pColorX*/, int* /*pColorY*/) { return ONI_STATUS_NOT_SUPPORTED; }

protected:
	void raiseNewFrame(OniFrame* pFrame) { (*m_newFrameCallback)(this, pFrame, m_newFrameCallbackCookie); }
	void raisePropertyChanged(int propertyId, const void* data, int dataSize) { (*m_propertyChangedCallback)(this, propertyId, data, dataSize, m_propertyChangedCookie); }

	StreamServices& getServices() { return *m_pServices; }

private:
	StreamServices* m_pServices;
	NewFrameCallback m_newFrameCallback;
	void* m_newFrameCallbackCookie;
	PropertyChangedCallback m_propertyChangedCallback;
	void* m_propertyChangedCookie;
};

class DeviceBase
{
public:
	DeviceBase() {}
	virtual ~DeviceBase() {}

	virtual OniStatus getSensorInfoList(OniSensorInfo** pSensorInfos, int* numSensors) = 0;

	virtual StreamBase* createStream(OniSensorType) = 0;
	virtual void destroyStream(StreamBase* pStream) = 0;

	virtual OniStatus setProperty(int /*propertyId*/, const void* /*data*/, int /*dataSize*/) {return ONI_STATUS_NOT_IMPLEMENTED;}
	virtual OniStatus getProperty(int /*propertyId*/, void* /*data*/, int* /*pDataSize*/) {return ONI_STATUS_NOT_IMPLEMENTED;}
	virtual OniBool isPropertySupported(int /*propertyId*/) {return FALSE;}
	virtual OniStatus invoke(int /*commandId*/, void* /*data*/, int /*dataSize*/) {return ONI_STATUS_NOT_IMPLEMENTED;}
	virtual OniBool isCommandSupported(int /*commandId*/) {return FALSE;}
	virtual OniStatus tryManualTrigger() {return ONI_STATUS_OK;}

	virtual void setPropertyChangedCallback(PropertyChangedCallback handler, void* pCookie) { m_propertyChangedCallback = handler; m_propertyChangedCookie = pCookie; }
	virtual void notifyAllProperties() { return; }

	virtual OniBool isImageRegistrationModeSupported(OniImageRegistrationMode mode) { return (mode == ONI_IMAGE_REGISTRATION_OFF); }

protected:
	void raisePropertyChanged(int propertyId, const void* data, int dataSize) { (*m_propertyChangedCallback)(this, propertyId, data, dataSize, m_propertyChangedCookie); }

private:
	PropertyChangedCallback m_propertyChangedCallback;
	void* m_propertyChangedCookie;
};

class DriverServices
{
public:
	DriverServices(OniDriverServices* pDriverServices) : m_pDriverServices(pDriverServices) {}

	void errorLoggerAppend(const char* format, ...)
	{
		va_list args;
		va_start(args, format);
		m_pDriverServices->errorLoggerAppend(m_pDriverServices->driverServices, format, args);
		va_end(args);
	}

	void errorLoggerClear()
	{
		m_pDriverServices->errorLoggerClear(m_pDriverServices->driverServices);
	}

	void log(int severity, const char* file, int line, const char* mask, const char* message)
	{
		m_pDriverServices->log(m_pDriverServices->driverServices, severity, file, line, mask, message);
	}

private:
	OniDriverServices* m_pDriverServices;
};

class DriverBase
{
public:
	DriverBase(OniDriverServices* pDriverServices) : m_services(pDriverServices)
	{}

	virtual ~DriverBase() {}

	virtual OniStatus initialize(DeviceConnectedCallback connectedCallback, DeviceDisconnectedCallback disconnectedCallback, DeviceStateChangedCallback deviceStateChangedCallback, void* pCookie)
	{
		m_deviceConnectedEvent = connectedCallback;
		m_deviceDisconnectedEvent = disconnectedCallback;
		m_deviceStateChangedEvent = deviceStateChangedCallback;
		m_pCookie = pCookie;
		return ONI_STATUS_OK;
	}

	virtual DeviceBase* deviceOpen(const char* uri, const char* mode) = 0;
	virtual void deviceClose(DeviceBase* pDevice) = 0;

	virtual void shutdown() = 0;

	virtual OniStatus tryDevice(const char* /*uri*/) { return ONI_STATUS_ERROR;}

	virtual void* enableFrameSync(StreamBase** /*pStreams*/, int /*streamCount*/) { return NULL; }
	virtual void disableFrameSync(void* /*frameSyncGroup*/) {}

protected:
	void deviceConnected(const OniDeviceInfo* pInfo) { (m_deviceConnectedEvent)(pInfo, m_pCookie); }
	void deviceDisconnected(const OniDeviceInfo* pInfo) { (m_deviceDisconnectedEvent)(pInfo, m_pCookie); }
	void deviceStateChanged(const OniDeviceInfo* pInfo, int errorState) { (m_deviceStateChangedEvent)(pInfo, errorState, m_pCookie); }

	DriverServices& getServices() { return m_services; }

private:
	DeviceConnectedCallback m_deviceConnectedEvent;
	DeviceDisconnectedCallback m_deviceDisconnectedEvent;
	DeviceStateChangedCallback m_deviceStateChangedEvent;
	void* m_pCookie;

	DriverServices m_services;
};

}} // oni::driver

#define ONI_EXPORT_DRIVER(DriverClass)																						\
																															\
oni::driver::DriverBase* g_pDriver = NULL;																					\
																															\
/* As Driver */																												\
ONI_C_API_EXPORT void oniDriverCreate(OniDriverServices* driverServices) {													\
	g_pDriver = XN_NEW(DriverClass, driverServices);																		\
}																															\
ONI_C_API_EXPORT void oniDriverDestroy()																					\
{																															\
	g_pDriver->shutdown();																									\
	XN_DELETE(g_pDriver); g_pDriver = NULL;																					\
}																															\
ONI_C_API_EXPORT OniStatus oniDriverInitialize(oni::driver::DeviceConnectedCallback deviceConnectedCallback,				\
										oni::driver::DeviceDisconnectedCallback deviceDisconnectedCallback,					\
										oni::driver::DeviceStateChangedCallback deviceStateChangedCallback,					\
										void* pCookie)																		\
{																															\
	return g_pDriver->initialize(deviceConnectedCallback, deviceDisconnectedCallback, deviceStateChangedCallback, pCookie);	\
}																															\
																															\
ONI_C_API_EXPORT OniStatus oniDriverTryDevice(const char* uri)																\
{																															\
	return g_pDriver->tryDevice(uri);																						\
}																															\
																															\
/* As Device */																												\
ONI_C_API_EXPORT oni::driver::DeviceBase* oniDriverDeviceOpen(const char* uri, const char* mode)							\
{																															\
	return g_pDriver->deviceOpen(uri, mode);																				\
}																															\
ONI_C_API_EXPORT void oniDriverDeviceClose(oni::driver::DeviceBase* pDevice)												\
{																															\
	g_pDriver->deviceClose(pDevice);																						\
}																															\
																															\
ONI_C_API_EXPORT OniStatus oniDriverDeviceGetSensorInfoList(oni::driver::DeviceBase* pDevice, OniSensorInfo** pSensorInfos,	\
															int* numSensors)												\
{																															\
	return pDevice->getSensorInfoList(pSensorInfos, numSensors);															\
}																															\
																															\
ONI_C_API_EXPORT oni::driver::StreamBase* oniDriverDeviceCreateStream(oni::driver::DeviceBase* pDevice,						\
																		OniSensorType sensorType)							\
{																															\
	return pDevice->createStream(sensorType);																				\
}																															\
																															\
ONI_C_API_EXPORT void oniDriverDeviceDestroyStream(oni::driver::DeviceBase* pDevice, oni::driver::StreamBase* pStream)		\
{																															\
	return pDevice->destroyStream(pStream);																					\
}																															\
																															\
ONI_C_API_EXPORT OniStatus oniDriverDeviceSetProperty(oni::driver::DeviceBase* pDevice, int propertyId,						\
													const void* data, int dataSize)											\
{																															\
	return pDevice->setProperty(propertyId, data, dataSize);																\
}																															\
ONI_C_API_EXPORT OniStatus oniDriverDeviceGetProperty(oni::driver::DeviceBase* pDevice, int propertyId,						\
													void* data, int* pDataSize)												\
{																															\
	return pDevice->getProperty(propertyId, data, pDataSize);																\
}																															\
ONI_C_API_EXPORT OniBool oniDriverDeviceIsPropertySupported(oni::driver::DeviceBase* pDevice, int propertyId)				\
{																															\
	return pDevice->isPropertySupported(propertyId);																		\
}																															\
ONI_C_API_EXPORT void oniDriverDeviceSetPropertyChangedCallback(oni::driver::DeviceBase* pDevice,							\
	oni::driver::PropertyChangedCallback handler, void* pCookie)															\
{																															\
	pDevice->setPropertyChangedCallback(handler, pCookie);																	\
}																															\
ONI_C_API_EXPORT void oniDriverDeviceNotifyAllProperties(oni::driver::DeviceBase* pDevice)									\
{																															\
	pDevice->notifyAllProperties();																							\
}																															\
ONI_C_API_EXPORT OniStatus oniDriverDeviceInvoke(oni::driver::DeviceBase* pDevice, int commandId,							\
												void* data, int dataSize)													\
{																															\
	return pDevice->invoke(commandId, data, dataSize);																		\
}																															\
ONI_C_API_EXPORT OniBool oniDriverDeviceIsCommandSupported(oni::driver::DeviceBase* pDevice, int commandId)					\
{																															\
	return pDevice->isCommandSupported(commandId);																			\
}																															\
ONI_C_API_EXPORT OniStatus oniDriverDeviceTryManualTrigger(oni::driver::DeviceBase* pDevice)								\
{																															\
	return pDevice->tryManualTrigger();																						\
}																															\
ONI_C_API_EXPORT OniBool oniDriverDeviceIsImageRegistrationModeSupported(oni::driver::DeviceBase* pDevice,					\
	OniImageRegistrationMode mode)																							\
{																															\
	return pDevice->isImageRegistrationModeSupported(mode);																	\
}																															\
																															\
/* As Stream */																												\
ONI_C_API_EXPORT void oniDriverStreamSetServices(oni::driver::StreamBase* pStream, OniStreamServices* pServices)			\
{																															\
	pStream->setServices((oni::driver::StreamServices*)pServices);															\
}																															\
																															\
ONI_C_API_EXPORT OniStatus oniDriverStreamSetProperty(oni::driver::StreamBase* pStream, int propertyId,						\
													const void* data, int dataSize)											\
{																															\
	return pStream->setProperty(propertyId, data, dataSize);																\
}																															\
ONI_C_API_EXPORT OniStatus oniDriverStreamGetProperty(oni::driver::StreamBase* pStream, int propertyId, void* data,			\
													int* pDataSize)															\
{																															\
	return pStream->getProperty(propertyId, data, pDataSize);																\
}																															\
ONI_C_API_EXPORT OniBool oniDriverStreamIsPropertySupported(oni::driver::StreamBase* pStream, int propertyId)				\
{																															\
	return pStream->isPropertySupported(propertyId);																		\
}																															\
ONI_C_API_EXPORT void oniDriverStreamSetPropertyChangedCallback(oni::driver::StreamBase* pStream,							\
													oni::driver::PropertyChangedCallback handler, void* pCookie)			\
{																															\
	pStream->setPropertyChangedCallback(handler, pCookie);																	\
}																															\
ONI_C_API_EXPORT void oniDriverStreamNotifyAllProperties(oni::driver::StreamBase* pStream)									\
{																															\
	pStream->notifyAllProperties();																							\
}																															\
ONI_C_API_EXPORT OniStatus oniDriverStreamInvoke(oni::driver::StreamBase* pStream, int commandId,							\
												void* data, int dataSize)													\
{																															\
	return pStream->invoke(commandId, data, dataSize);																		\
}																															\
ONI_C_API_EXPORT OniBool oniDriverStreamIsCommandSupported(oni::driver::StreamBase* pStream, int commandId)					\
{																															\
	return pStream->isCommandSupported(commandId);																			\
}																															\
																															\
ONI_C_API_EXPORT OniStatus oniDriverStreamStart(oni::driver::StreamBase* pStream)											\
{																															\
	return pStream->start();																								\
}																															\
ONI_C_API_EXPORT void oniDriverStreamStop(oni::driver::StreamBase* pStream)													\
{																															\
	pStream->stop();																										\
}																															\
																															\
ONI_C_API_EXPORT int oniDriverStreamGetRequiredFrameSize(oni::driver::StreamBase* pStream)									\
{																															\
	return pStream->getRequiredFrameSize();																					\
}																															\
																															\
ONI_C_API_EXPORT void oniDriverStreamSetNewFrameCallback(oni::driver::StreamBase* pStream,									\
														oni::driver::NewFrameCallback handler, void* pCookie)				\
{																															\
	pStream->setNewFrameCallback(handler, pCookie);																			\
}																															\
																															\
ONI_C_API_EXPORT OniStatus oniDriverStreamConvertDepthToColorCoordinates(oni::driver::StreamBase* pDepthStream,				\
	oni::driver::StreamBase* pColorStream, int depthX, int depthY, OniDepthPixel depthZ, int* pColorX, int* pColorY)		\
{																															\
	return pDepthStream->convertDepthToColorCoordinates(pColorStream, depthX, depthY, depthZ, pColorX, pColorY);			\
}																															\
																															\
ONI_C_API_EXPORT void* oniDriverEnableFrameSync(oni::driver::StreamBase** pStreams, int streamCount)						\
{																															\
	return g_pDriver->enableFrameSync(pStreams, streamCount);																\
}																															\
																															\
ONI_C_API_EXPORT void oniDriverDisableFrameSync(void* frameSyncGroup)														\
{																															\
	return g_pDriver->disableFrameSync(frameSyncGroup);																		\
}																															\

#endif // _ONI_DRIVER_API_H_
