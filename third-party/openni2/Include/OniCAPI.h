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
#ifndef _ONI_C_API_H_
#define _ONI_C_API_H_

#include "OniPlatform.h"
#include "OniCTypes.h"
#include "OniCProperties.h"
#include "OniVersion.h"

/******************************************** General APIs */

/**  Initialize OpenNI2. Use ONI_API_VERSION as the version. */
ONI_C_API OniStatus oniInitialize(int apiVersion);
/**  Shutdown OpenNI2 */
ONI_C_API void oniShutdown();

/**
 * Get the list of currently connected device.
 * Each device is represented by its OniDeviceInfo.
 * pDevices will be allocated inside.
 */
ONI_C_API OniStatus oniGetDeviceList(OniDeviceInfo** pDevices, int* pNumDevices);
/** Release previously allocated device list */
ONI_C_API OniStatus oniReleaseDeviceList(OniDeviceInfo* pDevices);

ONI_C_API OniStatus oniRegisterDeviceCallbacks(OniDeviceCallbacks* pCallbacks, void* pCookie, OniCallbackHandle* pHandle);
ONI_C_API void oniUnregisterDeviceCallbacks(OniCallbackHandle handle);

/** Wait for any of the streams to have a new frame */
ONI_C_API OniStatus oniWaitForAnyStream(OniStreamHandle* pStreams, int numStreams, int* pStreamIndex, int timeout);

/** Get the current version of OpenNI2 */
ONI_C_API OniVersion oniGetVersion();

/** Translate from format to number of bytes per pixel. Will return 0 for formats in which the number of bytes per pixel isn't fixed. */
ONI_C_API int oniFormatBytesPerPixel(OniPixelFormat format);

/** Get internal error */
ONI_C_API const char* oniGetExtendedError();

/******************************************** Device APIs */

/** Open a device. Uri can be taken from the matching OniDeviceInfo. */
ONI_C_API OniStatus oniDeviceOpen(const char* uri, OniDeviceHandle* pDevice);
/** Close a device */
ONI_C_API OniStatus oniDeviceClose(OniDeviceHandle device);

/** Get the possible configurations available for a specific source, or NULL if the source does not exist. */
ONI_C_API const OniSensorInfo* oniDeviceGetSensorInfo(OniDeviceHandle device, OniSensorType sensorType);

/** Get the OniDeviceInfo of a certain device. */
ONI_C_API OniStatus oniDeviceGetInfo(OniDeviceHandle device, OniDeviceInfo* pInfo);

/** Create a new stream in the device. The stream will originate from the source. */
ONI_C_API OniStatus oniDeviceCreateStream(OniDeviceHandle device, OniSensorType sensorType, OniStreamHandle* pStream);

ONI_C_API OniStatus oniDeviceEnableDepthColorSync(OniDeviceHandle device);
ONI_C_API void oniDeviceDisableDepthColorSync(OniDeviceHandle device);
ONI_C_API OniBool oniDeviceGetDepthColorSyncEnabled(OniDeviceHandle device);

/** Set property in the device. Use the properties listed in OniTypes.h: ONI_DEVICE_PROPERTY_..., or specific ones supplied by the device. */
ONI_C_API OniStatus oniDeviceSetProperty(OniDeviceHandle device, int propertyId, const void* data, int dataSize);
/** Get property in the device. Use the properties listed in OniTypes.h: ONI_DEVICE_PROPERTY_..., or specific ones supplied by the device. */
ONI_C_API OniStatus oniDeviceGetProperty(OniDeviceHandle device, int propertyId, void* data, int* pDataSize);
/** Check if the property is supported by the device. Use the properties listed in OniTypes.h: ONI_DEVICE_PROPERTY_..., or specific ones supplied by the device. */
ONI_C_API OniBool oniDeviceIsPropertySupported(OniDeviceHandle device, int propertyId);
/** Invoke an internal functionality of the device. */
ONI_C_API OniStatus oniDeviceInvoke(OniDeviceHandle device, int commandId, void* data, int dataSize);
/** Check if a command is supported, for invoke */
ONI_C_API OniBool oniDeviceIsCommandSupported(OniDeviceHandle device, int commandId);

ONI_C_API OniBool oniDeviceIsImageRegistrationModeSupported(OniDeviceHandle device, OniImageRegistrationMode mode);

/** @internal */
ONI_C_API OniStatus oniDeviceOpenEx(const char* uri, const char* mode, OniDeviceHandle* pDevice);

/******************************************** Stream APIs */

/** Destroy an existing stream */
ONI_C_API void oniStreamDestroy(OniStreamHandle stream);

/** Get the OniSensorInfo of the certain stream. */
ONI_C_API const OniSensorInfo* oniStreamGetSensorInfo(OniStreamHandle stream);

/** Start generating data from the stream. */
ONI_C_API OniStatus oniStreamStart(OniStreamHandle stream);
/** Stop generating data from the stream. */
ONI_C_API void oniStreamStop(OniStreamHandle stream);

/** Get the next frame from the stream. This function is blocking until there is a new frame from the stream. For timeout, use oniWaitForStreams() first */
ONI_C_API OniStatus oniStreamReadFrame(OniStreamHandle stream, OniFrame** pFrame);

/** Register a callback to when the stream has a new frame. */
ONI_C_API OniStatus oniStreamRegisterNewFrameCallback(OniStreamHandle stream, OniNewFrameCallback handler, void* pCookie, OniCallbackHandle* pHandle);
/** Unregister a previously registered callback to when the stream has a new frame. */
ONI_C_API void oniStreamUnregisterNewFrameCallback(OniStreamHandle stream, OniCallbackHandle handle);

/** Set property in the stream. Use the properties listed in OniTypes.h: ONI_STREAM_PROPERTY_..., or specific ones supplied by the device for its streams. */
ONI_C_API OniStatus oniStreamSetProperty(OniStreamHandle stream, int propertyId, const void* data, int dataSize);
/** Get property in the stream. Use the properties listed in OniTypes.h: ONI_STREAM_PROPERTY_..., or specific ones supplied by the device for its streams. */
ONI_C_API OniStatus oniStreamGetProperty(OniStreamHandle stream, int propertyId, void* data, int* pDataSize);
/** Check if the property is supported the stream. Use the properties listed in OniTypes.h: ONI_STREAM_PROPERTY_..., or specific ones supplied by the device for its streams. */
ONI_C_API OniBool oniStreamIsPropertySupported(OniStreamHandle stream, int propertyId);
/** Invoke an internal functionality of the stream. */
ONI_C_API OniStatus oniStreamInvoke(OniStreamHandle stream, int commandId, void* data, int dataSize);
/** Check if a command is supported, for invoke */
ONI_C_API OniBool oniStreamIsCommandSupported(OniStreamHandle stream, int commandId);
/** Sets the stream buffer allocation functions. Note that this function may only be called while stream is not started. */
ONI_C_API OniStatus oniStreamSetFrameBuffersAllocator(OniStreamHandle stream, OniFrameAllocBufferCallback alloc, OniFrameFreeBufferCallback free, void* pCookie);

////
/** Mark another user of the frame. */
ONI_C_API void oniFrameAddRef(OniFrame* pFrame);
/** Mark that the frame is no longer needed.  */
ONI_C_API void oniFrameRelease(OniFrame* pFrame);

// ONI_C_API OniStatus oniConvertRealWorldToProjective(OniStreamHandle stream, OniFloatPoint3D* pRealWorldPoint, OniFloatPoint3D* pProjectivePoint);
// ONI_C_API OniStatus oniConvertProjectiveToRealWorld(OniStreamHandle stream, OniFloatPoint3D* pProjectivePoint, OniFloatPoint3D* pRealWorldPoint);

/**
 * Creates a recorder that records to a file.
 * @param	[in]	fileName	The name of the file that will contain the recording.
 * @param	[out]	pRecorder	Points to the handle to the newly created recorder.
 * @retval ONI_STATUS_OK Upon successful completion.
 * @retval ONI_STATUS_ERROR Upon any kind of failure.
 */
ONI_C_API OniStatus oniCreateRecorder(const char* fileName, OniRecorderHandle* pRecorder);

/**
 * Attaches a stream to a recorder. The amount of attached streams is virtually
 * infinite. You cannot attach a stream after you have started a recording, if
 * you do: an error will be returned by oniRecorderAttachStream.
 * @param	[in]	recorder				The handle to the recorder.
 * @param	[in]	stream					The handle to the stream.
 * @param	[in]	allowLossyCompression	Allows/denies lossy compression
 * @retval ONI_STATUS_OK Upon successful completion.
 * @retval ONI_STATUS_ERROR Upon any kind of failure.
 */
ONI_C_API OniStatus oniRecorderAttachStream(
        OniRecorderHandle   recorder, 
        OniStreamHandle     stream, 
        OniBool             allowLossyCompression);

/**
 * Starts recording. There must be at least one stream attached to the recorder,
 * if not: oniRecorderStart will return an error.
 * @param[in] recorder The handle to the recorder.
 * @retval ONI_STATUS_OK Upon successful completion.
 * @retval ONI_STATUS_ERROR Upon any kind of failure.
 */
ONI_C_API OniStatus oniRecorderStart(OniRecorderHandle recorder);

/**
 * Stops recording. You can resume recording via oniRecorderStart.
 * @param[in] recorder The handle to the recorder.
 * @retval ONI_STATUS_OK Upon successful completion.
 * @retval ONI_STATUS_ERROR Upon any kind of failure.
 */
ONI_C_API void oniRecorderStop(OniRecorderHandle recorder);

/**
 * Stops recording if needed, and destroys a recorder.
 * @param	[in,out]	recorder	The handle to the recorder, the handle will be
 *									invalidated (nullified) when the function returns.
 * @retval ONI_STATUS_OK Upon successful completion.
 * @retval ONI_STATUS_ERROR Upon any kind of failure.
 */
ONI_C_API OniStatus oniRecorderDestroy(OniRecorderHandle* pRecorder);

ONI_C_API OniStatus oniCoordinateConverterDepthToWorld(OniStreamHandle depthStream, float depthX, float depthY, float depthZ, float* pWorldX, float* pWorldY, float* pWorldZ);

ONI_C_API OniStatus oniCoordinateConverterWorldToDepth(OniStreamHandle depthStream, float worldX, float worldY, float worldZ, float* pDepthX, float* pDepthY, float* pDepthZ);

ONI_C_API OniStatus oniCoordinateConverterDepthToColor(OniStreamHandle depthStream, OniStreamHandle colorStream, int depthX, int depthY, OniDepthPixel depthZ, int* pColorX, int* pColorY);

/******************************************** Log APIs */

/** 
 * Change the log output folder

 * @param const char * strOutputFolder	[in]	path to the desirebale folder
 *
 * @retval ONI_STATUS_OK Upon successful completion.
 * @retval ONI_STATUS_ERROR Upon any kind of failure.
 */
ONI_C_API OniStatus oniSetLogOutputFolder(const char* strOutputFolder);

/** 
 * Get the current log file name

 * @param	char * strFileName	[out]	hold the returned file name
 * @param	int nBufferSize	[in]	size of strFileName
 *
 * @retval ONI_STATUS_OK Upon successful completion.
 * @retval ONI_STATUS_ERROR Upon any kind of failure.
 */
ONI_C_API OniStatus oniGetLogFileName(char* strFileName, int nBufferSize);

/** 
 * Set the Minimum severity for log produce 

 * @param	const char * strMask	[in]	Name of the logger
 *
 * @retval ONI_STATUS_OK Upon successful completion.
 * @retval ONI_STATUS_ERROR Upon any kind of failure.
 */
ONI_C_API OniStatus oniSetLogMinSeverity(int nMinSeverity);

/** 
 * Configures if log entries will be printed to console.

 * @param	OniBool bConsoleOutput	[in]	TRUE to print log entries to console, FALSE otherwise.
 *
 * @retval ONI_STATUS_OK Upon successful completion.
 * @retval ONI_STATUS_ERROR Upon any kind of failure.
 */
ONI_C_API OniStatus oniSetLogConsoleOutput(OniBool bConsoleOutput);

/** 
 * Configures if log entries will be printed to a log file.

 * @param	OniBool bFileOutput	[in]	TRUE to print log entries to the file, FALSE otherwise.
 *
 * @retval ONI_STATUS_OK Upon successful completion.
 * @retval ONI_STATUS_ERROR Upon any kind of failure.
 */
ONI_C_API OniStatus oniSetLogFileOutput(OniBool bFileOutput);

#if ONI_PLATFORM == ONI_PLATFORM_ANDROID_ARM
/** 
 * Configures if log entries will be printed to the Android log.

 * @param	OniBool bAndroidOutput	[in]	TRUE to print log entries to the Android log, FALSE otherwise.
 *
 * @retval ONI_STATUS_OK Upon successful completion.
 * @retval ONI_STATUS_ERROR Upon any kind of failure.
 */
ONI_C_API OniStatus oniSetLogAndroidOutput(OniBool bAndroidOutput);
#endif
#endif // _ONI_C_API_H_
