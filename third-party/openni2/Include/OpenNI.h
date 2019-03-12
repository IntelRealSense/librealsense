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
#ifndef _OPENNI_H_
#define _OPENNI_H_

#include "OniPlatform.h"
#include "OniProperties.h"
#include "OniEnums.h"

#include "OniCAPI.h"
#include "OniCProperties.h"

/**
openni is the namespace of the entire C++ API of OpenNI
*/
namespace openni
{

/** Pixel type used to store depth images. */
typedef uint16_t				DepthPixel;

/** Pixel type used to store IR images. */
typedef uint16_t				Grayscale16Pixel;

// structs
/** Holds an OpenNI version number, which consists of four separate numbers in the format: @c major.minor.maintenance.build. For example: 2.0.0.20. */
typedef struct
{
	/** Major version number, incremented for major API restructuring. */
	int major;
	/** Minor version number, incremented when significant new features added. */
	int minor;
	/** Maintenance build number, incremented for new releases that primarily provide minor bug fixes. */
	int maintenance;
	/** Build number. Incremented for each new API build. Generally not shown on the installer and download site. */
	int build;
} Version;

/** Holds the value of a single color image pixel in 24-bit RGB format. */
typedef struct
{
	/* Red value of this pixel. */
	uint8_t r;
	/* Green value of this pixel. */
	uint8_t g;
	/* Blue value of this pixel. */
	uint8_t b;
} RGB888Pixel;

/**
 Holds the value of two pixels in YUV422 format (Luminance/Chrominance,16-bits/pixel).
 The first pixel has the values y1, u, v.
 The second pixel has the values y2, u, v.
*/
typedef struct
{
	/** First chrominance value for two pixels, stored as blue luminance difference signal. */
	uint8_t u;
	/** Overall luminance value of first pixel. */
	uint8_t y1;
	/** Second chrominance value for two pixels, stored as red luminance difference signal. */
	uint8_t v;
	/** Overall luminance value of second pixel. */
	uint8_t y2;
} YUV422DoublePixel;

/** This special URI can be passed to @ref Device::open() when the application has no concern for a specific device. */
#if ONI_PLATFORM != ONI_PLATFORM_WIN32
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic push
#endif
static const char* ANY_DEVICE = NULL;
#if ONI_PLATFORM != ONI_PLATFORM_WIN32
#pragma GCC diagnostic pop
#endif

/**
Provides a simple array class used throughout the API. Wraps a primitive array
of objects, holding the elements and their count.
*/
template<class T>
class Array
{
public:
	/**
	Default constructor.  Creates an empty Array and sets the element count to zero.
	*/
	Array() : m_data(NULL), m_count(0), m_owner(false) {}

	/**
	Constructor.  Creates new Array from an existing primitive array of known size.

	@tparam [in] T Object type this Array will contain.
	@param [in] data Pointer to a primitive array of objects of type T.
	@param [in] count Number of elements in the primitive array pointed to by data.
	*/
	Array(const T* data, int count) : m_owner(false) { _setData(data, count); }

	/**
	Destructor.  Destroys the Array object.
	*/
	~Array()
	{
		clear();
	}

	/**
	Getter function for the Array size.
	@returns Current number of elements in the Array.
	*/
	int getSize() const { return m_count; }

	/**
	Implements the array indexing operator for the Array class.
	*/
	const T& operator[](int index) const {return m_data[index];}

	/**
	@internal
	Setter function for data.  Causes this array to wrap an existing primitive array
	of specified type.  The optional data ownership flag controls whether the primitive
	array this Array wraps will be destroyed when this Array is deconstructed.
	@param [in] T Type of objects array will contain.
	@param [in] data Pointer to first object in list.
	@param [in] count Number of objects in list.
	@param [in] isOwner Optional flag to indicate data ownership
	*/
	void _setData(const T* data, int count, bool isOwner = false)
	{
		clear();
		m_count = count;
		m_owner = isOwner;
		if (!isOwner)
		{
			m_data = data;
		}
		else
		{
			m_data = new T[count];
			memcpy((void*)m_data, data, count*sizeof(T));
		}
	}

private:
	Array(const Array<T>&);
	Array<T>& operator=(const Array<T>&);

	void clear()
	{
		if (m_owner && m_data != NULL)
			delete []m_data;
		m_owner = false;
		m_data = NULL;
		m_count = 0;
	}

	const T* m_data;
	int m_count;
	bool m_owner;
};

// Forward declaration of all
class SensorInfo;
class VideoStream;
class VideoFrameRef;
class Device;
class OpenNI;
class CameraSettings;
class PlaybackControl;

/**
Encapsulates a group of settings for a @ref VideoStream.  Settings stored include
frame rate, resolution, and pixel format.

This class is used as an input for changing the settings of a @ref VideoStream,
as well as an output for reporting the current settings of that class.  It is also used
by @ref SensorInfo to report available video modes of a stream.

Recommended practice is to use @ref SensorInfo::getSupportedVideoModes()
to obtain a list of valid video modes, and then to use items from that list to pass
new settings to @ref VideoStream.  This is much less likely to produce an
invalid video mode than instantiating and manually changing objects of this
class.
*/
class VideoMode : private OniVideoMode
{
public:
	/**
	Default constructor, creates an empty VideoMode object.  Application programs should, in most
	cases, use the copy constructor to copy an existing valid video mode.  This is much less
	error prone that creating and attempting to configure a new VideoMode from scratch.
	*/
	VideoMode()
	{}

	/**
	Copy constructor, creates a new VideoMode identical to an existing VideoMode.

	@param [in] other Existing VideoMode to copy.
	*/
	VideoMode(const VideoMode& other)
	{
		*this = other;
	}

	/**
	Assignment operator.  Sets the pixel format, frame rate, and resolution of this
	VideoMode to equal that of a different VideoMode.

	@param [in] other Existing VideoMode to copy settings from.
	*/
	VideoMode& operator=(const VideoMode& other)
	{
		setPixelFormat(other.getPixelFormat());
		setResolution(other.getResolutionX(), other.getResolutionY());
		setFps(other.getFps());

		return *this;
	}

	/**
	Getter function for the pixel format of this VideoMode.
	@returns Current pixel format setting of this VideoMode.
	*/
	PixelFormat getPixelFormat() const { return (PixelFormat)pixelFormat; }

	/**
	Getter function for the X resolution of this VideoMode.
	@returns Current horizontal resolution of this VideoMode, in pixels.
	*/
	int getResolutionX() const { return resolutionX; }

	/**
	Getter function for the Y resolution of this VideoMode.
	@returns Current vertical resolution of this VideoMode, in pixels.
	*/
	int getResolutionY() const {return resolutionY;}

	/**
	Getter function for the frame rate of this VideoMode.
	@returns Current frame rate, measured in frames per second.
	*/
	int getFps() const { return fps; }

	/**
	Setter function for the pixel format of this VideoMode.  Application use of this
	function is not recommended.  Instead, use @ref SensorInfo::getSupportedVideoModes()
	to obtain a list of valid video modes.
	@param [in] format Desired new pixel format for this VideoMode.
	*/
	void setPixelFormat(PixelFormat format) { this->pixelFormat = (OniPixelFormat)format; }

	/**
	Setter function for the resolution of this VideoMode.  Application use of this
	function is not recommended.  Instead, use @ref SensorInfo::getSupportedVideoModes() to
	obtain a list of valid video modes.
	@param [in] resolutionX Desired new horizontal resolution in pixels.
	@param [in] resolutionY Desired new vertical resolution in pixels.
	*/
	void setResolution(int resolutionX, int resolutionY)
	{
		this->resolutionX = resolutionX;
		this->resolutionY = resolutionY;
	}

	/**
	Setter function for the frame rate.  Application use of this function is not recommended.
	Instead, use @ref SensorInfo::getSupportedVideoModes() to obtain a list of valid
	video modes.
	@param [in] fps Desired new frame rate, measured in frames per second.
	*/
	void setFps(int fps) { this->fps = fps; }

	friend class SensorInfo;
	friend class VideoStream;
	friend class VideoFrameRef;
};

/**
The SensorInfo class encapsulates all info related to a specific sensor in a specific
device.  
A @ref Device object holds a SensorInfo object for each sensor it contains.
A @ref VideoStream object holds one SensorInfo object, describing the sensor used to produce that stream.

A given SensorInfo object will contain the type of the sensor (Depth, IR or Color), and
a list of all video modes that the sensor can support.  Each available video mode will have a single
VideoMode object that can be queried to get the details of that mode.

SensorInfo objects should be the only source of VideoMode objects for the vast majority of
application programs.

Application programs will never directly instantiate objects of type SensorInfo.  In fact, no
public constructors are provided.  SensorInfo objects should be obtained either from a Device or @ref VideoStream,
and in turn be used to provide available video modes for that sensor.
*/
class SensorInfo
{
public:
	/**
	Provides the sensor type of the sensor this object is associated with.
	@returns Type of the sensor.
	*/
	SensorType getSensorType() const { return (SensorType)m_pInfo->sensorType; }

	/**
	Provides a list of video modes that this sensor can support. This function is the
	recommended method to be used by applications to obtain @ref VideoMode objects.

	@returns Reference to an array of @ref VideoMode objects, one for each supported
	video mode.
	*/
	const Array<VideoMode>& getSupportedVideoModes() const { return m_videoModes; }

private:
	SensorInfo(const SensorInfo&);
	SensorInfo& operator=(const SensorInfo&);

	SensorInfo() : m_pInfo(NULL), m_videoModes(NULL, 0) {}

	SensorInfo(const OniSensorInfo* pInfo) : m_pInfo(NULL), m_videoModes(NULL, 0)
	{
		_setInternal(pInfo);
	}

	void _setInternal(const OniSensorInfo* pInfo)
	{
		m_pInfo = pInfo;
		if (pInfo == NULL)
		{
			m_videoModes._setData(NULL, 0);
		}
		else
		{
			m_videoModes._setData(static_cast<VideoMode*>(pInfo->pSupportedVideoModes), pInfo->numSupportedVideoModes);
		}
	}

	const OniSensorInfo* m_pInfo;
	Array<VideoMode> m_videoModes;

	friend class VideoStream;
	friend class Device;
};

/**
The DeviceInfo class encapsulates info related to a specific device.

Applications will generally obtain objects of this type via calls to @ref OpenNI::enumerateDevices() or
@ref openni::Device::getDeviceInfo(), and then use the various accessor functions to obtain specific
information on that device.

There should be no reason for application code to instantiate this object directly.
*/
class DeviceInfo : private OniDeviceInfo
{
public:
	/** 
	Returns the device URI.  URI can be used by @ref Device::open to open a specific device. 
	The URI string format is determined by the driver.
	*/
	const char* getUri() const { return uri; }
	/** Returns a the vendor name for this device. */
	const char* getVendor() const { return vendor; }
	/** Returns the device name for this device. */
	const char* getName() const { return name; }
	/** Returns the USB VID code for this device. */
	uint16_t getUsbVendorId() const { return usbVendorId; }
	/** Returns the USB PID code for this device. */
	uint16_t getUsbProductId() const { return usbProductId; }

	friend class Device;
	friend class OpenNI;
};

/**
The @ref VideoFrameRef class encapsulates a single video frame - the output of a @ref VideoStream at a specific time.
The data contained will be a single frame of color, IR, or depth video, along with associated meta data.

An object of type @ref VideoFrameRef does not actually hold the data of the frame, but only a reference to it. The
reference can be released by destroying the @ref VideoFrameRef object, or by calling the @ref release() method. The
actual data of the frame is freed when the last reference to it is released.

The usual way to obtain @ref VideoFrameRef objects is by a call to @ref VideoStream.:readFrame().

All data references by a @ref VideoFrameRef is stored as a primitive array of pixels.  Each pixel will be
of a type according to the configured pixel format (see @ref VideoMode).
*/
class VideoFrameRef
{
public:
	/**
	Default constructor.  Creates a new empty @ref VideoFrameRef object.
	This object will be invalid until initialized by a call to @ref VideoStream::readFrame().
	*/
	VideoFrameRef()
	{
		m_pFrame = NULL;
	}

	/**
	Destroy this object and release the reference to the frame.
	*/
	~VideoFrameRef()
	{
		release();
	}

	/**
	Copy constructor.  Creates a new @ref VideoFrameRef object. The newly created
	object will reference the same frame current object references.
	@param [in] other Another @ref VideoFrameRef object.
	*/
	VideoFrameRef(const VideoFrameRef& other) : m_pFrame(NULL)
	{
		_setFrame(other.m_pFrame);
	}

	/**
	Make this @ref VideoFrameRef object reference the same frame that the @c other frame references.
	If this object referenced another frame before calling this method, the previous frame will be released.
	@param [in] other Another @ref VideoFrameRef object.
	*/
	VideoFrameRef& operator=(const VideoFrameRef& other)
	{
		_setFrame(other.m_pFrame);
		return *this;
	}

	/**
	Getter function for the size of the data contained by this object.  Useful primarily
	when allocating buffers.
	@returns Current size of data pointed to by this object, measured in bytes.
	*/
	inline int getDataSize() const
	{
		return m_pFrame->dataSize;
	}

	/**
	Getter function for the array of data pointed to by this object.
	@returns Pointer to the actual frame data array.  Type of data
	pointed to can be determined according to the pixel format (can be obtained by calling @ref getVideoMode()).
	*/
	inline const void* getData() const
	{
		return m_pFrame->data;
	}

	/**
	Getter function for the sensor type used to produce this frame.  Used to determine whether
	this is an IR, Color or Depth frame.  See the @ref SensorType enumeration for all possible return
	values from this function.
	@returns The type of sensor used to produce this frame.
	*/
	inline SensorType getSensorType() const
	{
		return (SensorType)m_pFrame->sensorType;
	}

	/**
	Returns a reference to the @ref VideoMode object assigned to this frame.  This object describes
	the video mode the sensor was configured to when the frame was produced and can be used
	to determine the pixel format and resolution of the data.  It will also provide the frame rate
	that the sensor was running at when it recorded this frame.
	@returns Reference to the @ref VideoMode assigned to this frame.
	*/
	inline const VideoMode& getVideoMode() const
	{
		return static_cast<const VideoMode&>(m_pFrame->videoMode);
	}

	/**
	Provides a timestamp for the frame.  The 'zero' point for this stamp
	is implementation specific, but all streams from the same device are guaranteed to use the same zero.
	This value can therefore be used to compute time deltas between frames from the same device,
	regardless of whether they are from the same stream.
	@returns Timestamp of frame, measured in microseconds from an arbitrary zero
	*/
	inline uint64_t getTimestamp() const
	{
		return m_pFrame->timestamp;
	}

	/**
	Frames are provided sequential frame ID numbers by the sensor that produced them.  If frame
	synchronization has been enabled for a device via @ref Device::setDepthColorSyncEnabled(), then frame
	numbers for corresponding frames of depth and color are guaranteed to match.

	If frame synchronization is not enabled, then there is no guarantee of matching frame indexes between
	@ref VideoStream "VideoStreams".  In the latter case, applications should use timestamps instead of frame indexes to
	align frames in time.
	@returns Index number for this frame.
	*/
	inline int getFrameIndex() const
	{
		return m_pFrame->frameIndex;
	}

	/**
	Gives the current width of this frame, measured in pixels.  If cropping is enabled, this will be
	the width of the cropping window.  If cropping is not enabled, then this will simply be equal to
	the X resolution of the @ref VideoMode used to produce this frame.
	@returns Width of this frame in pixels.
	*/
	inline int getWidth() const
	{
		return m_pFrame->width;
	}

	/**
	Gives the current height of this frame, measured in pixels.  If cropping is enabled, this will
	be the length of the cropping window.  If cropping is not enabled, then this will simply be equal
	to the Y resolution of the @ref VideoMode used to produce this frame.
	*/
	inline int getHeight() const
	{
		return m_pFrame->height;
	}

	/**
	Indicates whether cropping was enabled when the frame was produced.
	@return true if cropping is enabled, false otherwise
	*/
	inline bool getCroppingEnabled() const
	{
		return m_pFrame->croppingEnabled == TRUE;
	}

	/**
	Indicates the X coordinate of the upper left corner of the crop window.
	@return Distance of crop origin from left side of image, in pixels.
	*/
	inline int getCropOriginX() const
	{
		return m_pFrame->cropOriginX;
	}

	/**
	Indicates the Y coordinate of the upper left corner of the crop window.
	@return Distance of crop origin from top of image, in pixels.
	*/
	inline int getCropOriginY() const
	{
		return m_pFrame->cropOriginY;
	}

	/**
	Gives the length of one row of pixels, measured in bytes.  Primarily useful
	for indexing the array which contains the data.
	@returns Stride of the array which contains the image for this frame, in bytes
	*/
	inline int getStrideInBytes() const
	{
		return m_pFrame->stride;
	}

	/**
	Check if this object references an actual frame.
	*/
	inline bool isValid() const
	{
		return m_pFrame != NULL;
	}

	/** 
	Release the reference to the frame. Once this method is called, the object becomes invalid, and no method
	should be called other than the assignment operator, or passing this object to a @ref VideoStream::readFrame() call.
	*/
	void release()
	{
		if (m_pFrame != NULL)
		{
			oniFrameRelease(m_pFrame);
			m_pFrame = NULL;
		}
	}

	/** @internal */
	void _setFrame(OniFrame* pFrame)
	{
		setReference(pFrame);
		if (pFrame != NULL)
		{
			oniFrameAddRef(pFrame);
		}
	}

	/** @internal */
	OniFrame* _getFrame()
	{
		return m_pFrame;
	}

private:
	friend class VideoStream;
	inline void setReference(OniFrame* pFrame)
	{
		// Initial - don't addref. This is the reference from OpenNI
		release();
		m_pFrame = pFrame;
	}

	OniFrame* m_pFrame; // const!!?
};

/**
The @ref VideoStream object encapsulates a single video stream from a device.  Once created, it is used to start data flow
from the device, and to read individual frames of data.  This is the central class used to obtain data in OpenNI.  It
provides the ability to manually read data in a polling loop, as well as providing events and a Listener class that can be
used to implement event-driven data acquisition.

Aside from the video data frames themselves, the class offers a number of functions used for obtaining information about a
@ref VideoStream.  Field of view, available video modes, and minimum and maximum valid pixel values can all be obtained.

In addition to obtaining data, the @ref VideoStream object is used to set all configuration properties that apply to a specific
stream (rather than to an entire device).  In particular, it is used to control cropping, mirroring, and video modes.

A pointer to a valid, initialized device that provides the desired stream type is required to create a stream.

Several video streams can be created to stream data from the same sensor. This is useful if several components of an application
need to read frames separately.

While some device might allow different streams
from the same sensor to have different configurations, most devices will have a single configuration for the sensor, 
shared by all streams.
*/
class VideoStream
{
public:
	/**
	The @ref VideoStream::NewFrameListener class is provided to allow the implementation of event driven frame reading.  To use
	it, create a class that inherits from it and implement override the onNewFrame() method.  Then, register
	your created class with an active @ref VideoStream using the @ref VideoStream::addNewFrameListener() function.  Once this is done, the
	event handler function you implemented will be called whenever a new frame becomes available. You may call 
	@ref VideoStream::readFrame() from within the event handler.
	*/
	class NewFrameListener
	{
	public:
		/**
		Default constructor.
		*/
		NewFrameListener() : m_callbackHandle(NULL)
		{
		}

		virtual ~NewFrameListener()
		{
		}

		/**
		Derived classes should implement this function to handle new frames.
		*/
		virtual void onNewFrame(VideoStream&) = 0;

	private:
		friend class VideoStream;

		static void ONI_CALLBACK_TYPE callback(OniStreamHandle streamHandle, void* pCookie)
		{
			NewFrameListener* pListener = (NewFrameListener*)pCookie;
			VideoStream stream;
			stream._setHandle(streamHandle);
			pListener->onNewFrame(stream);
			stream._setHandle(NULL);
		}
		OniCallbackHandle m_callbackHandle;
	};

	class FrameAllocator
	{
	public:
		virtual ~FrameAllocator() {}
		virtual void* allocateFrameBuffer(int size) = 0;
		virtual void freeFrameBuffer(void* data) = 0;

	private:
		friend class VideoStream;

		static void* ONI_CALLBACK_TYPE allocateFrameBufferCallback(int size, void* pCookie)
		{
			FrameAllocator* pThis = (FrameAllocator*)pCookie;
			return pThis->allocateFrameBuffer(size);
		}

		static void ONI_CALLBACK_TYPE freeFrameBufferCallback(void* data, void* pCookie)
		{
			FrameAllocator* pThis = (FrameAllocator*)pCookie;
			pThis->freeFrameBuffer(data);
		}
	};

	/**
	Default constructor.  Creates a new, non-valid @ref VideoStream object.  The object created will be invalid until its create() function
	is called with a valid Device.
	*/
	VideoStream() : m_stream(NULL), m_sensorInfo(), m_pCameraSettings(NULL), m_isOwner(true)
	{}

	/**
	Handle constructor. Creates a VideoStream object based on the given initialized handle.
	This object will not destroy the underlying handle when  @ref destroy() or destructor is called
	*/
	explicit VideoStream(OniStreamHandle handle) : m_stream(NULL), m_sensorInfo(), m_pCameraSettings(NULL), m_isOwner(false)
	{
		_setHandle(handle);
	}

	/**
	Destructor.  The destructor calls the destroy() function, but it is considered a best practice for applications to
	call destroy() manually on any @ref VideoStream that they run create() on.
	*/
	~VideoStream()
	{
		destroy();
	}

	/**
	Checks to see if this object has been properly initialized and currently points to a valid stream.
	@returns true if this object has been previously initialized, false otherwise.
	*/
	bool isValid() const
	{
		return m_stream != NULL;
	}

	/**
	Creates a stream of frames from a specific sensor type of a specific device.  You must supply a reference to a
	Device that supplies the sensor type requested.  You can use @ref Device::hasSensor() to check whether a
	given sensor is available on your target device before calling create().

	@param [in] device A reference to the @ref Device you want to create the stream on.
	@param [in] sensorType The type of sensor the stream should produce data from.
	@returns Status code indicating success or failure for this operation.
	*/
	inline Status create(const Device& device, SensorType sensorType);

	/**
	Destroy this stream.  This function is currently called automatically by the destructor, but it is
	considered a best practice for applications to manually call this function on any @ref VideoStream that they
	call create() for.
	*/
	inline void destroy();

	/**
	Provides the @ref SensorInfo object associated with the sensor that is producing this @ref VideoStream.  Note that
	this function will return NULL if the stream has not yet been initialized with the create() function.

	@ref SensorInfo is useful primarily as a means of learning which video modes are valid for this VideoStream.

	@returns Reference to the SensorInfo object associated with the sensor providing this stream.
	*/
	const SensorInfo& getSensorInfo() const
	{
		return m_sensorInfo;
	}

	/**
	Starts data generation from this video stream.
	*/
	Status start()
	{
		if (!isValid())
		{
			return STATUS_ERROR;
		}

		return (Status)oniStreamStart(m_stream);
	}

	/**
	Stops data generation from this video stream.
	*/
	void stop()
	{
		if (!isValid())
		{
			return;
		}

		oniStreamStop(m_stream);
	}

	/**
	Read the next frame from this video stream, delivered as a @ref VideoFrameRef.  This is the primary
	method for manually obtaining frames of video data.  
	If no new frame is available, the call will block until one is available.
	To avoid blocking, use @ref VideoStream::Listener to implement an event driven architecture.  Another
	alternative is to use @ref OpenNI::waitForAnyStream() to wait for new frames from several streams.

	@param [out] pFrame Pointer to a @ref VideoFrameRef object to hold the reference to the new frame.
	@returns Status code to indicated success or failure of this function.
	*/
	Status readFrame(VideoFrameRef* pFrame)
	{
		if (!isValid())
		{
			return STATUS_ERROR;
		}

		OniFrame* pOniFrame;
		Status rc = (Status)oniStreamReadFrame(m_stream, &pOniFrame);

		pFrame->setReference(pOniFrame);
		return rc;
	}

	/**
	Adds a new Listener to receive this VideoStream onNewFrame event.  See @ref VideoStream::NewFrameListener for
	more information on implementing an event driven frame reading architecture. An instance of a listener can be added to only one source.

	@param [in] pListener Pointer to a @ref VideoStream::NewFrameListener object (or a derivative) that will respond to this event.
	@returns Status code indicating success or failure of the operation.
	*/
	Status addNewFrameListener(NewFrameListener* pListener)
	{
		if (!isValid())
		{
			return STATUS_ERROR;
		}

		return (Status)oniStreamRegisterNewFrameCallback(m_stream, pListener->callback, pListener, &pListener->m_callbackHandle);
	}

	/**
	Removes a Listener from this video stream list.  The listener removed will no longer receive new frame events from this stream.
	@param [in] pListener Pointer to the listener object to be removed.
	*/
	void removeNewFrameListener(NewFrameListener* pListener)
	{
		if (!isValid())
		{
			return;
		}

		oniStreamUnregisterNewFrameCallback(m_stream, pListener->m_callbackHandle);
		pListener->m_callbackHandle = NULL;
	}

	/**
	Sets the frame buffers allocator for this video stream.
	@param [in] pAllocator Pointer to the frame buffers allocator object. Pass NULL to return to default frame allocator.
	@returns ONI_STATUS_OUT_OF_FLOW The frame buffers allocator cannot be set while stream is streaming.
	*/
	Status setFrameBuffersAllocator(FrameAllocator* pAllocator)
	{
		if (!isValid())
		{
			return STATUS_ERROR;
		}

		if (pAllocator == NULL)
		{
			return (Status)oniStreamSetFrameBuffersAllocator(m_stream, NULL, NULL, NULL);
		}
		else
		{
			return (Status)oniStreamSetFrameBuffersAllocator(m_stream, pAllocator->allocateFrameBufferCallback, pAllocator->freeFrameBufferCallback, pAllocator);
		}
	}

	/**
	@internal
	Get an internal handle. This handle can be used via the C API.
	*/
	OniStreamHandle _getHandle() const
	{
		return m_stream;
	}

	/**
	Gets an object through which several camera settings can be configured.
	@returns NULL if the stream doesn't support camera settings.
	*/
	CameraSettings* getCameraSettings() {return m_pCameraSettings;}

	/**
	General function for obtaining the value of stream specific properties.
	There are convenience functions available for all commonly used properties, so it is not
	expected that applications will make direct use of the getProperty function very often.

	@param [in] propertyId The numerical ID of the property to be queried.
	@param [out] data Place to store the value of the property.
	@param [in,out] dataSize IN: Size of the buffer passed in the @c data argument. OUT: the actual written size.
	@returns Status code indicating success or failure of this operation.
	*/
	Status getProperty(int propertyId, void* data, int* dataSize) const
	{
		if (!isValid())
		{
			return STATUS_ERROR;
		}

		return (Status)oniStreamGetProperty(m_stream, propertyId, data, dataSize);
	}

	/**
	General function for setting the value of stream specific properties.
	There are convenience functions available for all commonly used properties, so it is not
	expected that applications will make direct use of the setProperty function very often.

	@param [in] propertyId The numerical ID of the property to be set.
	@param [in] data Place to store the data to be written to the property.
	@param [in] dataSize Size of the data to be written to the property.
	@returns Status code indicating success or failure of this operation.
	*/
	Status setProperty(int propertyId, const void* data, int dataSize)
	{
		if (!isValid())
		{
			return STATUS_ERROR;
		}

		return (Status)oniStreamSetProperty(m_stream, propertyId, data, dataSize);
	}

	/**
	Get the current video mode information for this video stream.
	This includes its resolution, fps and stream format.

	@returns Current video mode information for this video stream.
	*/
	VideoMode getVideoMode() const
	{
		VideoMode videoMode;
		getProperty<OniVideoMode>(STREAM_PROPERTY_VIDEO_MODE, static_cast<OniVideoMode*>(&videoMode));
		return videoMode;
	}

	/**
	Changes the current video mode of this stream.  Recommended practice is to use @ref Device::getSensorInfo(), and
	then @ref SensorInfo::getSupportedVideoModes() to obtain a list of valid video mode settings for this stream.  Then,
	pass a valid @ref VideoMode to @ref setVideoMode to ensure correct operation.

	@param [in] videoMode Desired new video mode for this stream.
	returns Status code indicating success or failure of this operation.
	*/
	Status setVideoMode(const VideoMode& videoMode)
	{
		return setProperty<OniVideoMode>(STREAM_PROPERTY_VIDEO_MODE, static_cast<const OniVideoMode&>(videoMode));
	}

	/**
	Provides the maximum possible value for pixels obtained by this stream.  This is most useful for
	getting the maximum possible value of depth streams.
	@returns Maximum possible pixel value.
	*/
	int getMaxPixelValue() const
	{
		int maxValue;
		Status rc = getProperty<int>(STREAM_PROPERTY_MAX_VALUE, &maxValue);
		if (rc != STATUS_OK)
		{
			return 0;
		}
		return maxValue;
	}

	/**
	Provides the smallest possible value for pixels obtains by this VideoStream.  This is most useful
	for getting the minimum possible value that will be reported by a depth stream.
	@returns Minimum possible pixel value that can come from this stream.
	*/
	int getMinPixelValue() const
	{
		int minValue;
		Status rc = getProperty<int>(STREAM_PROPERTY_MIN_VALUE, &minValue);
		if (rc != STATUS_OK)
		{
			return 0;
		}
		return minValue;
	}

	/**
	Checks whether this stream supports cropping.
	@returns true if the stream supports cropping, false if it does not.
	*/
	bool isCroppingSupported() const
	{
		return isPropertySupported(STREAM_PROPERTY_CROPPING);
	}

	/**
	Obtains the current cropping settings for this stream.
	@param [out] pOriginX X coordinate of the upper left corner of the cropping window
	@param [out] pOriginY Y coordinate of the upper left corner of the cropping window
	@param [out] pWidth Horizontal width of the cropping window, in pixels
	@param [out] pHeight Vertical width of the cropping window, in pixels
	returns true if cropping is currently enabled, false if it is not.
	*/
	bool getCropping(int* pOriginX, int* pOriginY, int* pWidth, int* pHeight) const
	{
		OniCropping cropping;
		bool enabled = false;

		Status rc = getProperty<OniCropping>(STREAM_PROPERTY_CROPPING, &cropping);

		if (rc == STATUS_OK)
		{
			*pOriginX = cropping.originX;
			*pOriginY = cropping.originY;
			*pWidth = cropping.width;
			*pHeight = cropping.height;
			enabled = (cropping.enabled == TRUE);
		}

		return enabled;
	}

	/**
	Changes the cropping settings for this stream.  You can use the @ref isCroppingSupported()
	function to make sure cropping is supported before calling this function.
	@param [in] originX New X coordinate of the upper left corner of the cropping window.
	@param [in] originY New Y coordinate of the upper left corner of the cropping window.
	@param [in] width New horizontal width for the cropping window, in pixels.
	@param [in] height New vertical height for the cropping window, in pixels.
	@returns Status code indicating success or failure of this operation.
	*/
	Status setCropping(int originX, int originY, int width, int height)
	{
		OniCropping cropping;
		cropping.enabled = true;
		cropping.originX = originX;
		cropping.originY = originY;
		cropping.width = width;
		cropping.height = height;
		return setProperty<OniCropping>(STREAM_PROPERTY_CROPPING, cropping);
	}

	/**
	Disables cropping.
	@returns Status code indicating success or failure of this operation.
	*/
	Status resetCropping()
	{
		OniCropping cropping;
		cropping.enabled = false;
		return setProperty<OniCropping>(STREAM_PROPERTY_CROPPING, cropping);
	}

	/**
	Check whether mirroring is currently turned on for this stream.
	@returns true if mirroring is currently enabled, false otherwise.
	*/
	bool getMirroringEnabled() const
	{
		OniBool enabled;
		Status rc = getProperty<OniBool>(STREAM_PROPERTY_MIRRORING, &enabled);
		if (rc != STATUS_OK)
		{
			return false;
		}
		return enabled == TRUE;
	}

	/**
	Enable or disable mirroring for this stream.
	@param [in] isEnabled true to enable mirroring, false to disable it.
	@returns Status code indicating the success or failure of this operation.
	*/
	Status setMirroringEnabled(bool isEnabled)
	{
		return setProperty<OniBool>(STREAM_PROPERTY_MIRRORING, isEnabled ? TRUE : FALSE);
	}

	/**
	Gets the horizontal field of view of frames received from this stream.
	@returns Horizontal field of view, in radians.
	*/
	float getHorizontalFieldOfView() const
	{
		float horizontal = 0;
		getProperty<float>(STREAM_PROPERTY_HORIZONTAL_FOV, &horizontal);
		return horizontal;
	}

	/**
	Gets the vertical field of view of frames received from this stream.
	@returns Vertical field of view, in radians.
	*/
	float getVerticalFieldOfView() const
	{
		float vertical = 0;
		getProperty<float>(STREAM_PROPERTY_VERTICAL_FOV, &vertical);
		return vertical;
	}

	/**
	Function for setting a value of a stream property using an arbitrary input type.
	There are convenience functions available for all commonly used properties, so it is not
	expected that applications will make direct use of this function very often.
	@tparam [in] T Data type of the value to be passed to the property.
	@param [in] propertyId The numerical ID of the property to be set.
	@param [in] value Data to be sent to the property.
	@returns Status code indicating success or failure of this operation.
	*/
	template <class T>
	Status setProperty(int propertyId, const T& value)
	{
		return setProperty(propertyId, &value, sizeof(T));
	}

	/**
	Function for getting the value from a property using an arbitrary output type.
	There are convenience functions available for all commonly used properties, so it is not
	expected that applications will make direct use of this function very often.
	@tparam [in] T Data type of the value to be read.
	@param [in] propertyId The numerical ID of the property to be read.
	@param [in, out] value Pointer to a place to store the value read from the property.
	@returns Status code indicating success or failure of this operation.
	*/
	template <class T>
	Status getProperty(int propertyId, T* value) const
	{
		int size = sizeof(T);
		return getProperty(propertyId, value, &size);
	}

	/**
	Checks if a specific property is supported by the video stream.
	@param [in] propertyId Property to be checked.
	@returns true if the property is supported, false otherwise.
	*/
	bool isPropertySupported(int propertyId) const
	{
		if (!isValid())
		{
			return false;
		}

		return oniStreamIsPropertySupported(m_stream, propertyId) == TRUE;
	}

	/**
	Invokes a command that takes an arbitrary data type as its input.  It is not expected that
	application code will need this function frequently, as all commonly used properties have
	higher level functions provided.
	@param [in] commandId Numerical code of the property to be invoked.
	@param [in] data Data to be passed to the property.
	@param [in] dataSize size of the buffer passed in @c data.
	@returns Status code indicating success or failure of this operation.
	*/
	Status invoke(int commandId, void* data, int dataSize)
	{
		if (!isValid())
		{
			return STATUS_ERROR;
		}

		return (Status)oniStreamInvoke(m_stream, commandId, data, dataSize);
	}

	/**
	Invokes a command that takes an arbitrary data type as its input.  It is not expected that
	application code will need this function frequently, as all commonly used properties have
	higher level functions provided.
	@tparam [in] T Type of data to be passed to the property.
	@param [in] commandId Numerical code of the property to be invoked.
	@param [in] value Data to be passed to the property.
	@returns Status code indicating success or failure of this operation.
	*/
	template <class T>
	Status invoke(int commandId, T& value)
	{
		return invoke(commandId, &value, sizeof(T));
	}

	/**
	Checks if a specific command is supported by the video stream.
	@param [in] commandId Command to be checked.
	@returns true if the command is supported, false otherwise.
	*/
	bool isCommandSupported(int commandId) const
	{
		if (!isValid())
		{
			return false;
		}

		return (Status)oniStreamIsCommandSupported(m_stream, commandId) == TRUE;
	}

private:
	friend class Device;

	void _setHandle(OniStreamHandle stream)
	{
		m_sensorInfo._setInternal(NULL);
		m_stream = stream;

		if (stream != NULL)
		{
			m_sensorInfo._setInternal(oniStreamGetSensorInfo(m_stream));
		}
	}

private:
	VideoStream(const VideoStream& other);
	VideoStream& operator=(const VideoStream& other);

	OniStreamHandle m_stream;
	SensorInfo m_sensorInfo;
	CameraSettings* m_pCameraSettings;
	bool m_isOwner;
};

/**
The Device object abstracts a specific device; either a single hardware device, or a file
device holding a recording from a hardware device.  It offers the ability to connect to
the device, and obtain information about its configuration and the data streams it can offer.

It provides the means to query and change all configuration parameters that apply to the
device as a whole.  This includes enabling depth/color image registration and frame
synchronization.

Devices are used when creating and initializing @ref VideoStream "VideoStreams" -- you will need a valid pointer to
a Device in order to use the VideoStream.create() function.  This, along with configuration, is
the primary use of this class for application developers.

Before devices can be created, @ref OpenNI::initialize() must have been run to make the device drivers
on the system available to the API.
*/
class Device
{
public:
	/**
	Default constructor. Creates a new empty Device object. This object will be invalid until it is initialized by
	calling its open() function.
	*/
	Device() : m_pPlaybackControl(NULL), m_device(NULL), m_isOwner(true)
	{
		clearSensors();
	}

	/**
	Handle constructor. Creates a Device object based on the given initialized handle.
	This object will not destroy the underlying handle when  @ref close() or destructor is called
	*/
	explicit Device(OniDeviceHandle handle) : m_pPlaybackControl(NULL), m_device(NULL), m_isOwner(false)
	{
		_setHandle(handle);
	}

	/**
	The destructor calls the @ref close() function, but it is considered a best practice for applications to
	call @ref close() manually on any @ref Device that they run @ref open() on.
	*/
	~Device()
	{
		if (m_device != NULL)
		{
			close();
		}
	}

	/**
	Opens a device.  This can either open a device chosen arbitrarily from all devices
	on the system, or open a specific device selected by passing this function the device URI.

	To open any device, simply pass the constant@ref ANY_DEVICE to this function.  If multiple
	devices are connected to the system, then one of them will be opened.  This procedure is most
	useful when it is known that exactly one device is (or can be) connected to the system.  In that case,
	requesting a list of all devices and iterating through it would be a waste of effort.

	If multiple devices are (or may be) connected to a system, then a URI will be required to select
	a specific device to open.  There are two ways to obtain a URI: from a DeviceConnected event, or
	by calling @ref OpenNI::enumerateDevices().

	In the case of a DeviceConnected event, the @ref OpenNI::Listener will be provided with a DeviceInfo object
	as an argument to its @ref OpenNI::Listener::onDeviceConnected "onDeviceConnected()" function.  
	The DeviceInfo.getUri() function can then be used to obtain the URI.

	If the application is not using event handlers, then it can also call the static function
	@ref OpenNI::enumerateDevices().  This will return an array of @ref DeviceInfo objects, one for each device
	currently available to the system.  The application can then iterate through this list and
	select the desired device.  The URI is again obtained via the @ref DeviceInfo::getUri() function.

	Standard codes of type Status are returned indicating whether opening was successful.

	@param [in] uri String containing the URI of the device to be opened, or @ref ANY_DEVICE.
	@returns Status code with the outcome of the open operation.

	@remark For opening a recording file, pass the file path as a uri.
	*/
	inline Status open(const char* uri);

	/**
	Closes the device.  This properly closes any files or shuts down hardware, as appropriate.  This
	function is currently called by the destructor if not called manually by application code, but it
	is considered a best practice to manually close any device that was opened.
	*/
	inline void close();

	/**
	Provides information about this device in the form of a DeviceInfo object.  This object can
	be used to access the URI of the device, as well as various USB descriptor strings that might
	be useful to an application.

	Note that valid device info will not be available if this device has not yet been opened.  If you are
	trying to obtain a URI to open a device, use OpenNI::enumerateDevices() instead.
	@returns DeviceInfo object for this Device
	*/
	const DeviceInfo& getDeviceInfo() const
	{
		return m_deviceInfo;
	}

	/**
	This function checks to see if one of the specific sensor types defined in @ref SensorType is
	available on this device.  This allows an application to, for example, query for the presence
	of a depth sensor, or color sensor.
	@param [in] sensorType of sensor to query for
	@returns true if the Device supports the sensor queried, false otherwise.
	*/
	bool hasSensor(SensorType sensorType)
	{
		int i;
		for (i = 0; (i < ONI_MAX_SENSORS) && (m_aSensorInfo[i].m_pInfo != NULL); ++i)
		{
			if (m_aSensorInfo[i].getSensorType() == sensorType)
			{
				return true;
			}
		}

		if (i == ONI_MAX_SENSORS)
		{
			return false;
		}

		const OniSensorInfo* pInfo = oniDeviceGetSensorInfo(m_device, (OniSensorType)sensorType);

		if (pInfo == NULL)
		{
			return false;
		}

		m_aSensorInfo[i]._setInternal(pInfo);

		return true;
	}

	/**
	Get the @ref SensorInfo for a specific sensor type on this device.  The @ref SensorInfo
	is useful primarily for determining which video modes are supported by the sensor.
	@param [in] sensorType of sensor to get information about.
	@returns SensorInfo object corresponding to the sensor type specified, or NULL if such a sensor
			is not available from this device.
	*/
	const SensorInfo* getSensorInfo(SensorType sensorType)
	{
		int i;
		for (i = 0; (i < ONI_MAX_SENSORS) && (m_aSensorInfo[i].m_pInfo != NULL); ++i)
		{
			if (m_aSensorInfo[i].getSensorType() == sensorType)
			{
				return &m_aSensorInfo[i];
			}
		}

		// not found. check to see we have additional space
		if (i == ONI_MAX_SENSORS)
		{
			return NULL;
		}

		const OniSensorInfo* pInfo = oniDeviceGetSensorInfo(m_device, (OniSensorType)sensorType);
		if (pInfo == NULL)
		{
			return NULL;
		}

		m_aSensorInfo[i]._setInternal(pInfo);
		return &m_aSensorInfo[i];
	}

	/**
	@internal
	Get an internal handle. This handle can be used via the C API.
	*/
	OniDeviceHandle _getHandle() const
	{
		return m_device;
	}

	/**
	Gets an object through which playback of a file device can be controlled.
	@returns NULL if this device is not a file device.
	*/
	PlaybackControl* getPlaybackControl() {return m_pPlaybackControl;}

	/**
	Get the value of a general property of the device.
	There are convenience functions for all the commonly used properties, such as
	image registration and frame synchronization.  It is expected for this reason
	that this function will rarely be directly used by applications.

	@param [in] propertyId Numerical ID of the property you would like to check.
	@param [out] data Place to store the value of the property.
	@param [in,out] dataSize IN: Size of the buffer passed in the @c data argument. OUT: the actual written size.
	@returns Status code indicating results of this operation.
	*/
	Status getProperty(int propertyId, void* data, int* dataSize) const
	{
		return (Status)oniDeviceGetProperty(m_device, propertyId, data, dataSize);
	}

	/**
	Sets the value of a general property of the device.
	There are convenience functions for all the commonly used properties, such as
	image registration and frame synchronization.  It is expected for this reason
	that this function will rarely be directly used by applications.

	@param [in] propertyId The numerical ID of the property to be set.
	@param [in] data Place to store the data to be written to the property.
	@param [in] dataSize Size of the data to be written to the property.
	@returns Status code indicating results of this operation.
	*/
	Status setProperty(int propertyId, const void* data, int dataSize)
	{
		return (Status)oniDeviceSetProperty(m_device, propertyId, data, dataSize);
	}

	/**
	Checks to see if this device can support registration of color video and depth video.
	Image registration is used to properly superimpose two images from cameras located at different
	points in space.  Please see the OpenNi 2.0 Programmer's Guide for more information about
	registration.
	@returns true if image registration is supported by this device, false otherwise.
	*/
	bool isImageRegistrationModeSupported(ImageRegistrationMode mode) const
	{
		return (oniDeviceIsImageRegistrationModeSupported(m_device, (OniImageRegistrationMode)mode) == TRUE);
	}

	/**
	Gets the current image registration mode of this device.
	Image registration is used to properly superimpose two images from cameras located at different
	points in space.  Please see the OpenNi 2.0 Programmer's Guide for more information about
	registration.
	@returns Current image registration mode.  See @ref ImageRegistrationMode for possible return values.
	*/
	ImageRegistrationMode getImageRegistrationMode() const
	{
		ImageRegistrationMode mode;
		Status rc = getProperty<ImageRegistrationMode>(DEVICE_PROPERTY_IMAGE_REGISTRATION, &mode);
		if (rc != STATUS_OK)
		{
			return IMAGE_REGISTRATION_OFF;
		}
		return mode;
	}

	/**
	Sets the image registration on this device.
	Image registration is used to properly superimpose two images from cameras located at different
	points in space.  Please see the OpenNi 2.0 Programmer's Guide for more information about
	registration.

	See @ref ImageRegistrationMode for a list of valid settings to pass to this function. 
	
	It is a good practice to first check if the mode is supported by calling @ref isImageRegistrationModeSupported().

	@param [in] mode Desired new value for the image registration mode.
	@returns Status code for the operation.
	*/
	Status setImageRegistrationMode(ImageRegistrationMode mode)
	{
		return setProperty<ImageRegistrationMode>(DEVICE_PROPERTY_IMAGE_REGISTRATION, mode);
	}

	/**
	Checks whether this Device object is currently connected to an actual file or hardware device.
	@returns true if the Device is connected, false otherwise.
	*/
	bool isValid() const
	{
		return m_device != NULL;
	}

	/**
	Checks whether this device is a file device (i.e. a recording).
	@returns true if this is a file device, false otherwise.
	*/
	bool isFile() const
	{
		return isPropertySupported(DEVICE_PROPERTY_PLAYBACK_SPEED) &&
			isPropertySupported(DEVICE_PROPERTY_PLAYBACK_REPEAT_ENABLED) &&
			isCommandSupported(DEVICE_COMMAND_SEEK);
	}

	/**
	Used to turn the depth/color frame synchronization feature on and off.  When frame synchronization
	is enabled, the device will deliver depth and image frames that are separated in time
	by some maximum value.  When disabled, the phase difference between depth and image frame
	generation cannot be guaranteed.
	@param [in] isEnabled Set to TRUE to enable synchronization, FALSE to disable it
	@returns Status code indicating success or failure of this operation
	*/
	Status setDepthColorSyncEnabled(bool isEnabled)
	{
		Status rc = STATUS_OK;

		if (isEnabled)
		{
			rc = (Status)oniDeviceEnableDepthColorSync(m_device);
		}
		else
		{
			oniDeviceDisableDepthColorSync(m_device);
		}

		return rc;
	}

	bool getDepthColorSyncEnabled()
	{
		return oniDeviceGetDepthColorSyncEnabled(m_device) == TRUE;
	}

	/**
	Sets a property that takes an arbitrary data type as its input.  It is not expected that
	application code will need this function frequently, as all commonly used properties have
	higher level functions provided.

	@tparam T Type of data to be passed to the property.
	@param [in] propertyId The numerical ID of the property to be set.
	@param [in] value Place to store the data to be written to the property.
	@returns Status code indicating success or failure of this operation.
	*/
	template <class T>
	Status setProperty(int propertyId, const T& value)
	{
		return setProperty(propertyId, &value, sizeof(T));
	}

	/**
	Checks a property that provides an arbitrary data type as its output.  It is not expected that
	application code will need this function frequently, as all commonly used properties have
	higher level functions provided.
	@tparam [in] T Data type of the value to be read.
	@param [in] propertyId The numerical ID of the property to be read.
	@param [in, out] value Pointer to a place to store the value read from the property.
	@returns Status code indicating success or failure of this operation.
	*/
	template <class T>
	Status getProperty(int propertyId, T* value) const
	{
		int size = sizeof(T);
		return getProperty(propertyId, value, &size);
	}

	/**
	Checks if a specific property is supported by the device.
	@param [in] propertyId Property to be checked.
	@returns true if the property is supported, false otherwise.
	*/
	bool isPropertySupported(int propertyId) const
	{
		return oniDeviceIsPropertySupported(m_device, propertyId) == TRUE;
	}

	/**
	Invokes a command that takes an arbitrary data type as its input.  It is not expected that
	application code will need this function frequently, as all commonly used properties have
	higher level functions provided.
	@param [in] commandId Numerical code of the property to be invoked.
	@param [in] data Data to be passed to the property.
	@param [in] dataSize size of the buffer passed in @c data.
	@returns Status code indicating success or failure of this operation.
	*/
	Status invoke(int commandId, void* data, int dataSize)
	{
		return (Status)oniDeviceInvoke(m_device, commandId, data, dataSize);
	}

	/**
	Invokes a command that takes an arbitrary data type as its input.  It is not expected that
	application code will need this function frequently, as all commonly used properties have
	higher level functions provided.
	@tparam [in] T Type of data to be passed to the property.
	@param [in] propertyId Numerical code of the property to be invoked.
	@param [in] value Data to be passed to the property.
	@returns Status code indicating success or failure of this operation.
	*/
	template <class T>
	Status invoke(int propertyId, T& value)
	{
		return invoke(propertyId, &value, sizeof(T));
	}

	/**
	Checks if a specific command is supported by the device.
	@param [in] commandId Command to be checked.
	@returns true if the command is supported, false otherwise.
	*/
	bool isCommandSupported(int commandId) const
	{
		return oniDeviceIsCommandSupported(m_device, commandId) == TRUE;
	}

	/** @internal **/
	inline Status _openEx(const char* uri, const char* mode);

private:
	Device(const Device&);
	Device& operator=(const Device&);

	void clearSensors()
	{
		for (int i = 0; i < ONI_MAX_SENSORS; ++i)
		{
			m_aSensorInfo[i]._setInternal(NULL);
		}
	}

	inline Status _setHandle(OniDeviceHandle deviceHandle);

private:
	PlaybackControl* m_pPlaybackControl;

	OniDeviceHandle m_device;
	DeviceInfo m_deviceInfo;
	SensorInfo m_aSensorInfo[ONI_MAX_SENSORS];

	bool m_isOwner;
};

/**
 * The PlaybackControl class provides access to a series of specific to playing back
 * a recording from a file device.
 *
 * When playing a stream back from a recording instead of playing from a live device,
 * it is possible to vary playback speed, change the current time location (ie
 * fast forward / rewind / seek), specify whether the playback should be repeated at the end
 * of the recording, and query the total size of the recording.
 *
 * Since none of these functions make sense in the context of a physical device, they are
 * split out into a seperate playback control class.  To use, simply create your file device, 
 * create a PlaybackControl, and then attach the PlaybackControl to the file device.
 */
class PlaybackControl
{
public:

	/**
	 * Deconstructor.  Destroys a PlaybackControl class.  The deconstructor presently detaches
	 * from its recording automatically, but it is considered a best practice for applications to 
	 * manually detach from any stream that was attached to.
	 */
	~PlaybackControl()
	{
		detach();
	}

	/**
	 * Getter function for the current playback speed of this device.
	 *
	 * This value is expressed as a multiple of the speed the original
	 * recording was taken at.  For example, if the original recording was at 30fps, and 
	 * playback speed is set to 0.5, then the recording will play at 15fps.  If playback speed
	 * is set to 2.0, then the recording would playback at 60fps.
	 *
	 * In addition, there are two "special" values.  A playback speed of 0.0 indicates that the
	 * playback should occur as fast as the system is capable of returning frames.  This is 
	 * most useful when testing algorithms on large datasets, as it enables playback to be
	 * done at a much higher rate than would otherwise be possible.
	 *
	 * A value of -1 indicates that speed is "manual".  In this mode, new frames will only
	 * become available when an application manually reads them.  If used in a polling loop,
	 * this setting also enables systems to read and process frames limited only by 
	 * available processing speeds.
	 *
	 * @returns Current playback speed of the device, measured as ratio of recording speed.
	*/
	float getSpeed() const
	{
		if (!isValid())
		{
			return 0.0f;
		}
		float speed;
		Status rc = m_pDevice->getProperty<float>(DEVICE_PROPERTY_PLAYBACK_SPEED, &speed);
		if (rc != STATUS_OK)
		{
			return 1.0f;
		}
		return speed;
	}
	/**
	* Setter function for the playback speed of the device.  For a full explaination of 
	* what this value means @see PlaybackControl::getSpeed().
	*
	* @param [in] speed Desired new value of playback speed, as ratio of original recording.
	* @returns Status code indicating success or failure of this operation.
	*/
	Status setSpeed(float speed)
	{
		if (!isValid())
		{
			return STATUS_NO_DEVICE;
		}
		return m_pDevice->setProperty<float>(DEVICE_PROPERTY_PLAYBACK_SPEED, speed);
	}

	/**
	* Gets the current repeat setting of the file device.
	*
	* @returns true if repeat is enabled, false if not enabled.
	*/
	bool getRepeatEnabled() const
	{
		if (!isValid())
		{
			return false;
		}

		OniBool repeat;
		Status rc = m_pDevice->getProperty<OniBool>(DEVICE_PROPERTY_PLAYBACK_REPEAT_ENABLED, &repeat);
		if (rc != STATUS_OK)
		{
			return false;
		}

		return repeat == TRUE;
	}

	/**
	* Changes the current repeat mode of the device.  If repeat mode is turned on, then the recording will
	* begin playback again at the beginning after the last frame is read.  If turned off, no more frames 
	* will become available after last frame is read.
	*
	* @param [in] repeat New value for repeat -- true to enable, false to disable
	* @returns Status code indicating success or failure of this operations.
	*/
	Status setRepeatEnabled(bool repeat)
	{
		if (!isValid())
		{
			return STATUS_NO_DEVICE;
		}

		return m_pDevice->setProperty<OniBool>(DEVICE_PROPERTY_PLAYBACK_REPEAT_ENABLED, repeat ? TRUE : FALSE);
	}

	/**
	* Seeks within a VideoStream to a given FrameID.  Note that when this function is called on one 
	* stream, all other streams will also be changed to the corresponding place in the recording.  The FrameIDs
	* of different streams may not match, since FrameIDs may differ for streams that are not synchronized, but
	* the recording will set all streams to the same moment in time.
	*
	* @param [in] stream Stream for which the frameIndex value is valid.  
	* @param [in] frameIndex Frame index to move playback to
	* @returns Status code indicating success or failure of this operation
	*/
	Status seek(const VideoStream& stream, int frameIndex)
	{
		if (!isValid())
		{
			return STATUS_NO_DEVICE;
		}
		OniSeek seek;
		seek.frameIndex = frameIndex;
		seek.stream = stream._getHandle();
		return m_pDevice->invoke(DEVICE_COMMAND_SEEK, seek);
	}

	/**
	 * Provides the a count of frames that this recording contains for a given stream.  This is useful
	 * both to determine the length of the recording, and to ensure that a valid Frame Index is set when using
	 * the @ref PlaybackControl::seek() function.
	 *
	 * @param [in] stream The video stream to count frames for
	 * @returns Number of frames in provided @ref VideoStream, or 0 if the stream is not part of the recording
	*/
	int getNumberOfFrames(const VideoStream& stream) const
	{
		int numOfFrames = -1;
		Status rc = stream.getProperty<int>(STREAM_PROPERTY_NUMBER_OF_FRAMES, &numOfFrames);
		if (rc != STATUS_OK)
		{
			return 0;
		}
		return numOfFrames;
	}

	bool isValid() const
	{
		return m_pDevice != NULL;
	}
private:
	Status attach(Device* device)
	{
		if (!device->isValid() || !device->isFile())
		{
			return STATUS_ERROR;
		}

		detach();
		m_pDevice = device;

		return STATUS_OK;
	}
	void detach()
	{
		m_pDevice = NULL;
	}

	friend class Device;
	PlaybackControl(Device* pDevice) : m_pDevice(NULL)
	{
		if (pDevice != NULL)
		{
			attach(pDevice);
		}
	}

	Device* m_pDevice;
};

class CameraSettings
{
public:
	// setters
	Status setAutoExposureEnabled(bool enabled)
	{
		return setProperty(STREAM_PROPERTY_AUTO_EXPOSURE, enabled ? TRUE : FALSE);
	}
	Status setAutoWhiteBalanceEnabled(bool enabled)
	{
		return setProperty(STREAM_PROPERTY_AUTO_WHITE_BALANCE, enabled ? TRUE : FALSE);
	}

	bool getAutoExposureEnabled() const
	{
		OniBool enabled = FALSE;

		Status rc = getProperty(STREAM_PROPERTY_AUTO_EXPOSURE, &enabled);
		return rc == STATUS_OK && enabled == TRUE;
	}
	bool getAutoWhiteBalanceEnabled() const
	{
		OniBool enabled = FALSE;

		Status rc = getProperty(STREAM_PROPERTY_AUTO_WHITE_BALANCE, &enabled);
		return rc == STATUS_OK && enabled == TRUE;
	}

	Status setGain(int gain)
	{
		return setProperty(STREAM_PROPERTY_GAIN, gain);
	}
	Status setExposure(int exposure)
	{
		return setProperty(STREAM_PROPERTY_EXPOSURE, exposure);
	}
	int getGain()
	{
		int gain;
		Status rc = getProperty(STREAM_PROPERTY_GAIN, &gain);
		if (rc != STATUS_OK)
		{
			return 100;
		}
		return gain;
	}
	int getExposure()
	{
		int exposure;
		Status rc = getProperty(STREAM_PROPERTY_EXPOSURE, &exposure);
		if (rc != STATUS_OK)
		{
			return 0;
		}
		return exposure;
	}

	bool isValid() const {return m_pStream != NULL;}
private:
	template <class T>
	Status getProperty(int propertyId, T* value) const
	{
		if (!isValid()) return STATUS_NOT_SUPPORTED;

		return m_pStream->getProperty<T>(propertyId, value);
	}
	template <class T>
	Status setProperty(int propertyId, const T& value)
	{
		if (!isValid()) return STATUS_NOT_SUPPORTED;

		return m_pStream->setProperty<T>(propertyId, value);
	}

	friend class VideoStream;
	CameraSettings(VideoStream* pStream)
	{
		m_pStream = pStream;
	}

	VideoStream* m_pStream;
};


/**
 * The OpenNI class is a static entry point to the library.  It is used by every OpenNI 2.0 
 * application to initialize the SDK and drivers to enable creation of valid device objects.  
 *
 * It also defines a listener class and events that enable for event driven notification of 
 * device connection, device disconnection, and device configuration changes.  
 *
 * In addition, it gives access to SDK version information and provides a function that allows 
 * you to wait for data to become available on any one of a list of streams (as opposed to 
 * waiting for data on one specific stream with functions provided by the VideoStream class)
 *
*/
class OpenNI
{
public:

	/**
	 * The OpenNI::DeviceConnectedListener class provides a means of registering for, and responding to  
	 * when a device is connected.
	 *
	 * onDeviceConnected is called whenever a new device is connected to the system (ie this event
	 * would be triggered when a new sensor is manually plugged into the host system running the 
	 * application)
	 *
	 * To use this class, you should write a new class that inherits from it, and override the
	 * onDeviceConnected method.  Once you instantiate your class, use the
	 * OpenNI::addDeviceConnectedListener() function to add your listener object to OpenNI's list of listeners.  Your
	 * handler function will then be called whenever the event occurs.  A OpenNI::removeDeviceConnectedListener()
	 * function is also provided, if you want to have your class stop listening to these events for any
	 * reason.
	*/
	class DeviceConnectedListener
	{
	public:
		DeviceConnectedListener()
		{
			m_deviceConnectedCallbacks.deviceConnected = deviceConnectedCallback;
			m_deviceConnectedCallbacks.deviceDisconnected = NULL;
			m_deviceConnectedCallbacks.deviceStateChanged = NULL;
			m_deviceConnectedCallbacksHandle = NULL;
		}
		
		virtual ~DeviceConnectedListener()
		{
		}
		
		/**
		* Callback function for the onDeviceConnected event.  This function will be 
		* called whenever this event occurs.  When this happens, a pointer to the @ref DeviceInfo
		* object for the newly connected device will be supplied.  Note that once a 
		* device is removed, if it was opened by a @ref Device object, that object can no longer be
	 	* used to access the device, even if it was reconnected. Once a device was reconnected, 
	 	* @ref Device::open() should be called again in order to use this device.
		*
		* If you wish to open the new device as it is connected, simply query the provided DeviceInfo
		* object to obtain the URI of the device, and pass this URI to the Device.Open() function.
		*/
		virtual void onDeviceConnected(const DeviceInfo*) = 0;
	private:
		static void ONI_CALLBACK_TYPE deviceConnectedCallback(const OniDeviceInfo* pInfo, void* pCookie)
		{
			DeviceConnectedListener* pListener = (DeviceConnectedListener*)pCookie;
			pListener->onDeviceConnected(static_cast<const DeviceInfo*>(pInfo));
		}

 		friend class OpenNI;
		OniDeviceCallbacks m_deviceConnectedCallbacks;
		OniCallbackHandle m_deviceConnectedCallbacksHandle;

	};
	/**
	 * The OpenNI::DeviceDisconnectedListener class provides a means of registering for, and responding to  
	 * when a device is disconnected.
	 *
	 * onDeviceDisconnected is called when a device is removed from the system.  Note that once a 
	 * device is removed, if it was opened by a @ref Device object, that object can no longer be
	 * used to access the device, even if it was reconnected. Once a device was reconnected, 
	 * @ref Device::open() should be called again in order to use this device.
	 *
	 * To use this class, you should write a new class that inherits from it, and override the
	 * onDeviceDisconnected method.  Once you instantiate your class, use the
	 * OpenNI::addDeviceDisconnectedListener() function to add your listener object to OpenNI's list of listeners.  Your
	 * handler function will then be called whenever the event occurs.  A OpenNI::removeDeviceDisconnectedListener()
	 * function is also provided, if you want to have your class stop listening to these events for any
	 * reason.
	*/
	class DeviceDisconnectedListener
	{
	public:
		DeviceDisconnectedListener()
		{
			m_deviceDisconnectedCallbacks.deviceConnected = NULL;
			m_deviceDisconnectedCallbacks.deviceDisconnected = deviceDisconnectedCallback;
			m_deviceDisconnectedCallbacks.deviceStateChanged = NULL;
			m_deviceDisconnectedCallbacksHandle = NULL;
		}
		
		virtual ~DeviceDisconnectedListener()
		{
		}
		
		/**
		 * Callback function for the onDeviceDisconnected event. This function will be
		 * called whenever this event occurs.  When this happens, a pointer to the DeviceInfo
		 * object for the newly disconnected device will be supplied.  Note that once a 
		 * device is removed, if it was opened by a @ref Device object, that object can no longer be
	 	 * used to access the device, even if it was reconnected. Once a device was reconnected, 
	 	 * @ref Device::open() should be called again in order to use this device.
		*/
		virtual void onDeviceDisconnected(const DeviceInfo*) = 0;
	private:
		static void ONI_CALLBACK_TYPE deviceDisconnectedCallback(const OniDeviceInfo* pInfo, void* pCookie)
		{
			DeviceDisconnectedListener* pListener = (DeviceDisconnectedListener*)pCookie;
			pListener->onDeviceDisconnected(static_cast<const DeviceInfo*>(pInfo));
		}

		friend class OpenNI;
		OniDeviceCallbacks m_deviceDisconnectedCallbacks;
		OniCallbackHandle m_deviceDisconnectedCallbacksHandle;
	};
	/**
	 * The OpenNI::DeviceStateChangedListener class provides a means of registering for, and responding to  
	 * when a device's state is changed.
	 *
	 * onDeviceStateChanged is triggered whenever the state of a connected device is changed.
	 *
	 * To use this class, you should write a new class that inherits from it, and override the
	 * onDeviceStateChanged method.  Once you instantiate your class, use the
	 * OpenNI::addDeviceStateChangedListener() function to add your listener object to OpenNI's list of listeners.  Your
	 * handler function will then be called whenever the event occurs.  A OpenNI::removeDeviceStateChangedListener()
	 * function is also provided, if you want to have your class stop listening to these events for any
	 * reason.
	*/
	class DeviceStateChangedListener
	{
	public:
		DeviceStateChangedListener()
		{
			m_deviceStateChangedCallbacks.deviceConnected = NULL;
			m_deviceStateChangedCallbacks.deviceDisconnected = NULL;
			m_deviceStateChangedCallbacks.deviceStateChanged = deviceStateChangedCallback;
			m_deviceStateChangedCallbacksHandle = NULL;
		}
		
		virtual ~DeviceStateChangedListener()
		{
		}
		
		/**
		* Callback function for the onDeviceStateChanged event.  This function will be 
		* called whenever this event occurs.  When this happens, a pointer to a DeviceInfo
		* object for the affected device will be supplied, as well as the new DeviceState 
		* value of that device.
		*/
		virtual void onDeviceStateChanged(const DeviceInfo*, DeviceState) = 0;
	private:
		static void ONI_CALLBACK_TYPE deviceStateChangedCallback(const OniDeviceInfo* pInfo, OniDeviceState state, void* pCookie)
		{
			DeviceStateChangedListener* pListener = (DeviceStateChangedListener*)pCookie;
			pListener->onDeviceStateChanged(static_cast<const DeviceInfo*>(pInfo), DeviceState(state));
		}

		friend class OpenNI;
		OniDeviceCallbacks m_deviceStateChangedCallbacks;
		OniCallbackHandle m_deviceStateChangedCallbacksHandle;
	};

	/**
	Initialize the library.
	This will load all available drivers, and see which devices are available
	It is forbidden to call any other method in OpenNI before calling @ref initialize().
	*/
	static Status initialize()
	{
		return (Status)oniInitialize(ONI_API_VERSION); // provide version of API, to make sure proper struct sizes are used
	}

	/**
	Stop using the library. Unload all drivers, close all streams and devices.
	Once @ref shutdown was called, no other calls to OpenNI is allowed.
	*/
	static void shutdown()
	{
		oniShutdown();
	}

	/**
	* Returns the version of OpenNI
	*/
	static Version getVersion()
	{
		OniVersion oniVersion = oniGetVersion();
		Version version;
		version.major = oniVersion.major;
		version.minor = oniVersion.minor;
		version.maintenance = oniVersion.maintenance;
		version.build = oniVersion.build;
		return version;
	}

	/**
	* Retrieves the calling thread's last extended error information. The last extended error information is maintained 
	* on a per-thread basis. Multiple threads do not overwrite each other's last extended error information.
	*
	* The extended error information is cleared on every call to an OpenNI method, so you should call this method
	* immediately after a call to an OpenNI method which have failed.
	*/
	static const char* getExtendedError()
	{
		return oniGetExtendedError();
	}

	/**
	Fills up an array of @ref DeviceInfo objects with devices that are available.
	@param [in,out] deviceInfoList An array to be filled with devices.
	*/
	static void enumerateDevices(Array<DeviceInfo>* deviceInfoList)
	{
		OniDeviceInfo* m_pDeviceInfos;
		int m_deviceInfoCount;
		oniGetDeviceList(&m_pDeviceInfos, &m_deviceInfoCount);
		deviceInfoList->_setData((DeviceInfo*)m_pDeviceInfos, m_deviceInfoCount, true);
		oniReleaseDeviceList(m_pDeviceInfos);
	}

	/**
	Wait for a new frame from any of the streams provided. The function blocks until any of the streams
	has a new frame available, or the timeout has passed.
	@param [in] pStreams An array of streams to wait for.
	@param [in] streamCount The number of streams in @c pStreams
	@param [out] pReadyStreamIndex The index of the first stream that has new frame available.
	@param [in] timeout [Optional] A timeout before returning if no stream has new data. Default value is @ref TIMEOUT_FOREVER.
	*/
	static Status waitForAnyStream(VideoStream** pStreams, int streamCount, int* pReadyStreamIndex, int timeout = TIMEOUT_FOREVER)
	{
		static const int ONI_MAX_STREAMS = 50;
		OniStreamHandle streams[ONI_MAX_STREAMS];

		if (streamCount > ONI_MAX_STREAMS)
		{
			printf("Too many streams for wait: %d > %d\n", streamCount, ONI_MAX_STREAMS);
			return STATUS_BAD_PARAMETER;
		}

		*pReadyStreamIndex = -1;
		for (int i = 0; i < streamCount; ++i)
		{
			if (pStreams[i] != NULL)
			{
				streams[i] = pStreams[i]->_getHandle();
			}
			else
			{
				streams[i] = NULL;
			}
		}
		Status rc = (Status)oniWaitForAnyStream(streams, streamCount, pReadyStreamIndex, timeout);

		return rc;
	}

	/**
	* Add a listener to the list of objects that receive the event when a device is connected.  See the 
	* @ref OpenNI::DeviceConnectedListener class for details on utilizing the events provided by OpenNI.
	* 
	* @param pListener Pointer to the Listener to be added to the list
	* @returns Status code indicating success or failure of this operation.
	*/
	static Status addDeviceConnectedListener(DeviceConnectedListener* pListener)
	{
		if (pListener->m_deviceConnectedCallbacksHandle != NULL)
		{
			return STATUS_ERROR;
		}
		return (Status)oniRegisterDeviceCallbacks(&pListener->m_deviceConnectedCallbacks, pListener, &pListener->m_deviceConnectedCallbacksHandle);
	}
	/**
	* Add a listener to the list of objects that receive the event when a device is disconnected.  See the 
	* @ref OpenNI::DeviceDisconnectedListener class for details on utilizing the events provided by OpenNI.
	* 
	* @param pListener Pointer to the Listener to be added to the list
	* @returns Status code indicating success or failure of this operation.
	*/
	static Status addDeviceDisconnectedListener(DeviceDisconnectedListener* pListener)
	{
		if (pListener->m_deviceDisconnectedCallbacksHandle != NULL)
		{
			return STATUS_ERROR;
		}
		return (Status)oniRegisterDeviceCallbacks(&pListener->m_deviceDisconnectedCallbacks, pListener, &pListener->m_deviceDisconnectedCallbacksHandle);
	}
	/**
	* Add a listener to the list of objects that receive the event when a device's state changes.  See the 
	* @ref OpenNI::DeviceStateChangedListener class for details on utilizing the events provided by OpenNI.
	* 
	* @param pListener Pointer to the Listener to be added to the list
	* @returns Status code indicating success or failure of this operation.
	*/
	static Status addDeviceStateChangedListener(DeviceStateChangedListener* pListener)
	{
		if (pListener->m_deviceStateChangedCallbacksHandle != NULL)
		{
			return STATUS_ERROR;
		}
		return (Status)oniRegisterDeviceCallbacks(&pListener->m_deviceStateChangedCallbacks, pListener, &pListener->m_deviceStateChangedCallbacksHandle);
	}
	/**
	* Remove a listener from the list of objects that receive the event when a device is connected.  See
	* the @ref OpenNI::DeviceConnectedListener class for details on utilizing the events provided by OpenNI.
	*
	* @param pListener Pointer to the Listener to be removed from the list
	* @returns Status code indicating the success or failure of this operation.
	*/
	static void removeDeviceConnectedListener(DeviceConnectedListener* pListener)
	{
		oniUnregisterDeviceCallbacks(pListener->m_deviceConnectedCallbacksHandle);
		pListener->m_deviceConnectedCallbacksHandle = NULL;
	}
	/**
	* Remove a listener from the list of objects that receive the event when a device is disconnected.  See
	* the @ref OpenNI::DeviceDisconnectedListener class for details on utilizing the events provided by OpenNI.
	*
	* @param pListener Pointer to the Listener to be removed from the list
	* @returns Status code indicating the success or failure of this operation.
	*/
	static void removeDeviceDisconnectedListener(DeviceDisconnectedListener* pListener)
	{
		oniUnregisterDeviceCallbacks(pListener->m_deviceDisconnectedCallbacksHandle);
		pListener->m_deviceDisconnectedCallbacksHandle = NULL;
	}
	/**
	* Remove a listener from the list of objects that receive the event when a device's state changes.  See
	* the @ref OpenNI::DeviceStateChangedListener class for details on utilizing the events provided by OpenNI.
	*
	* @param pListener Pointer to the Listener to be removed from the list
	* @returns Status code indicating the success or failure of this operation.
	*/
	static void removeDeviceStateChangedListener(DeviceStateChangedListener* pListener)
	{
		oniUnregisterDeviceCallbacks(pListener->m_deviceStateChangedCallbacksHandle);
		pListener->m_deviceStateChangedCallbacksHandle = NULL;
	}

	/** 
	 * Change the log output folder
	
	 * @param	const char * strLogOutputFolder	[in]	log required folder
	 *
	 * @retval STATUS_OK Upon successful completion.
	 * @retval STATUS_ERROR Upon any kind of failure.
	 */
	static Status setLogOutputFolder(const char *strLogOutputFolder)
	{
		return (Status)oniSetLogOutputFolder(strLogOutputFolder);
	}

	/** 
	 * Get current log file name
	
	 * @param	char * strFileName	[out]	returned file name buffer
	 * @param	int	nBufferSize	[in]	Buffer size
	 *
	 * @retval STATUS_OK Upon successful completion.
	 * @retval STATUS_ERROR Upon any kind of failure.
	 */
	static Status getLogFileName(char *strFileName, int nBufferSize)
	{
		return (Status)oniGetLogFileName(strFileName, nBufferSize);
	}

	/** 
	 * Set minimum severity for log produce
	
	 * @param	const char * strMask	[in]	Logger name
	 * @param	int nMinSeverity	[in]	Logger severity
	 *
	 * @retval STATUS_OK Upon successful completion.
	 * @retval STATUS_ERROR Upon any kind of failure.
	 */
	static Status setLogMinSeverity(int nMinSeverity)
	{
		return(Status) oniSetLogMinSeverity(nMinSeverity);
	}
	
	/** 
	* Configures if log entries will be printed to console.

	* @param	const OniBool bConsoleOutput	[in]	TRUE to print log entries to console, FALSE otherwise.
	*
	* @retval STATUS_OK Upon successful completion.
	* @retval STATUS_ERROR Upon any kind of failure.
	 */
	static Status setLogConsoleOutput(bool bConsoleOutput)
	{
		return (Status)oniSetLogConsoleOutput(bConsoleOutput);
	}

	/** 
	* Configures if log entries will be printed to file.

	* @param	const OniBool bConsoleOutput	[in]	TRUE to print log entries to file, FALSE otherwise.
	*
	* @retval STATUS_OK Upon successful completion.
	* @retval STATUS_ERROR Upon any kind of failure.
	 */
	static Status setLogFileOutput(bool bFileOutput)
	{
		return (Status)oniSetLogFileOutput(bFileOutput);
	}

	#if ONI_PLATFORM == ONI_PLATFORM_ANDROID_ARM
	/** 
	 * Configures if log entries will be printed to the Android log.

	 * @param	OniBool bAndroidOutput bAndroidOutput	[in]	TRUE to print log entries to the Android log, FALSE otherwise.
	 *
	 * @retval STATUS_OK Upon successful completion.
	 * @retval STATUS_ERROR Upon any kind of failure.
	 */
	
	static Status setLogAndroidOutput(bool bAndroidOutput)
	{
		return (Status)oniSetLogAndroidOutput(bAndroidOutput);
	}
	#endif
	
private:
	OpenNI()
	{
	}
};

/**
The CoordinateConverter class converts points between the different coordinate systems.

<b>Depth and World coordinate systems</b>

OpenNI applications commonly use two different coordinate systems to represent depth.  These two systems are referred to as Depth
and World representation.

Depth coordinates are the native data representation.  In this system, the frame is a map (two dimensional array), and each pixel is
assigned a depth value.  This depth value represents the distance between the camera plane and whatever object is in the given
pixel. The X and Y coordinates are simply the location in the map, where the origin is the top-left corner of the field of view.

World coordinates superimpose a more familiar 3D Cartesian coordinate system on the world, with the camera lens at the origin.
In this system, every point is specified by 3 points -- x, y and z.  The x axis of this system is along a line that passes
through the infrared projector and CMOS imager of the camera.  The y axis is parallel to the front face of the camera, and
perpendicular to the x axis (it will also be perpendicular to the ground if the camera is upright and level).  The z axis
runs into the scene, perpendicular to both the x and y axis.  From the perspective of the camera, an object moving from
left to right is moving along the increasing x axis.  An object moving up is moving along the increasing y axis, and an object
moving away from the camera is moving along the increasing z axis.

Mathematically, the Depth coordinate system is the projection of the scene on the CMOS. If the sensor's angular field of view and
resolution are known, then an angular size can be calculated for each pixel.  This is how the conversion algorithms work.  The
dependence of this calculation on FoV and resolution is the reason that a @ref VideoStream pointer must be provided to these
functions.  The @ref VideoStream pointer is used to determine parameters for the specific points to be converted.

Since Depth coordinates are a projective, the apparent size of objects in depth coordinates (measured in pixels)
will increase as an object moves closer to the sensor.  The size of objects in the World coordinate system is independent of
distance from the sensor.

Note that converting from Depth to World coordinates is relatively expensive computationally.  It is generally not practical to convert
the entire raw depth map to World coordinates.  A better approach is to have your computer vision algorithm work in Depth
coordinates for as long as possible, and only converting a few specific points to World coordinates right before output.

Note that when converting from Depth to World or vice versa, the Z value remains the same.
*/
class CoordinateConverter
{
public:
	/**
	Converts a single point from the World coordinate system to the Depth coordinate system.
	@param [in] depthStream Reference to an openni::VideoStream that will be used to determine the format of the Depth coordinates
	@param [in] worldX The X coordinate of the point to be converted, measured in millimeters in World coordinates
	@param [in] worldY The Y coordinate of the point to be converted, measured in millimeters in World coordinates
	@param [in] worldZ The Z coordinate of the point to be converted, measured in millimeters in World coordinates
	@param [out] pDepthX Pointer to a place to store the X coordinate of the output value, measured in pixels with 0 at far left of image
	@param [out] pDepthY Pointer to a place to store the Y coordinate of the output value, measured in pixels with 0 at top of image
	@param [out] pDepthZ Pointer to a place to store the Z(depth) coordinate of the output value, measured in the @ref PixelFormat of depthStream
	*/
	static Status convertWorldToDepth(const VideoStream& depthStream, float worldX, float worldY, float worldZ, int* pDepthX, int* pDepthY, DepthPixel* pDepthZ)
	{
		float depthX, depthY, depthZ;
		Status rc = (Status)oniCoordinateConverterWorldToDepth(depthStream._getHandle(), worldX, worldY, worldZ, &depthX, &depthY, &depthZ);
		*pDepthX = (int)depthX;
		*pDepthY = (int)depthY;
		*pDepthZ = (DepthPixel)depthZ;
		return rc;
	}

	/**
	Converts a single point from the World coordinate system to a floating point representation of the Depth coordinate system
	@param [in] depthStream Reference to an openni::VideoStream that will be used to determine the format of the Depth coordinates
	@param [in] worldX The X coordinate of the point to be converted, measured in millimeters in World coordinates
	@param [in] worldY The Y coordinate of the point to be converted, measured in millimeters in World coordinates
	@param [in] worldZ The Z coordinate of the point to be converted, measured in millimeters in World coordinates
	@param [out] pDepthX Pointer to a place to store the X coordinate of the output value, measured in pixels with 0.0 at far left of the image
	@param [out] pDepthY Pointer to a place to store the Y coordinate of the output value, measured in pixels with 0.0 at the top of the image
	@param [out] pDepthZ Pointer to a place to store the Z(depth) coordinate of the output value, measured in millimeters with 0.0 at the camera lens
	*/
	static Status convertWorldToDepth(const VideoStream& depthStream, float worldX, float worldY, float worldZ, float* pDepthX, float* pDepthY, float* pDepthZ)
	{
		return (Status)oniCoordinateConverterWorldToDepth(depthStream._getHandle(), worldX, worldY, worldZ, pDepthX, pDepthY, pDepthZ);
	}

	/**
	Converts a single point from the Depth coordinate system to the World coordinate system.
	@param [in] depthStream Reference to an openi::VideoStream that will be used to determine the format of the Depth coordinates
	@param [in] depthX The X coordinate of the point to be converted, measured in pixels with 0 at the far left of the image
	@param [in] depthY The Y coordinate of the point to be converted, measured in pixels with 0 at the top of the image
	@param [in] depthZ the Z(depth) coordinate of the point to be converted, measured in the @ref PixelFormat of depthStream
	@param [out] pWorldX Pointer to a place to store the X coordinate of the output value, measured in millimeters in World coordinates
	@param [out] pWorldY Pointer to a place to store the Y coordinate of the output value, measured in millimeters in World coordinates
	@param [out] pWorldZ Pointer to a place to store the Z coordinate of the output value, measured in millimeters in World coordinates
	*/
	static Status convertDepthToWorld(const VideoStream& depthStream, int depthX, int depthY, DepthPixel depthZ, float* pWorldX, float* pWorldY, float* pWorldZ)
	{
		return (Status)oniCoordinateConverterDepthToWorld(depthStream._getHandle(), float(depthX), float(depthY), float(depthZ), pWorldX, pWorldY, pWorldZ);
	}

	/**
	Converts a single point from a floating point representation of the Depth coordinate system to the World coordinate system.
	@param [in] depthStream Reference to an openi::VideoStream that will be used to determine the format of the Depth coordinates
	@param [in] depthX The X coordinate of the point to be converted, measured in pixels with 0.0 at the far left of the image
	@param [in] depthY The Y coordinate of the point to be converted, measured in pixels with 0.0 at the top of the image
	@param [in] depthZ Z(depth) coordinate of the point to be converted, measured in the @ref PixelFormat of depthStream
	@param [out] pWorldX Pointer to a place to store the X coordinate of the output value, measured in millimeters in World coordinates
	@param [out] pWorldY Pointer to a place to store the Y coordinate of the output value, measured in millimeters in World coordinates
	@param [out] pWorldZ Pointer to a place to store the Z coordinate of the output value, measured in millimeters in World coordinates
	*/
	static Status convertDepthToWorld(const VideoStream& depthStream, float depthX, float depthY, float depthZ, float* pWorldX, float* pWorldY, float* pWorldZ)
	{
		return (Status)oniCoordinateConverterDepthToWorld(depthStream._getHandle(), depthX, depthY, depthZ, pWorldX, pWorldY, pWorldZ);
	}

	/**
	For a given depth point, provides the coordinates of the corresponding color value.  Useful for superimposing the depth and color images.
	This operation is the same as turning on registration, but is performed on a single pixel rather than the whole image.
	@param [in] depthStream Reference to a openni::VideoStream that produced the depth value
	@param [in] colorStream Reference to a openni::VideoStream that we want to find the appropriate color pixel in
	@param [in] depthX X value of the depth point, given in Depth coordinates and measured in pixels
	@param [in] depthY Y value of the depth point, given in Depth coordinates and measured in pixels
	@param [in] depthZ Z(depth) value of the depth point, given in the @ref PixelFormat of depthStream
	@param [out] pColorX The X coordinate of the color pixel that overlaps the given depth pixel, measured in pixels
	@param [out] pColorY The Y coordinate of the color pixel that overlaps the given depth pixel, measured in pixels
	*/
	static Status convertDepthToColor(const VideoStream& depthStream, const VideoStream& colorStream, int depthX, int depthY, DepthPixel depthZ, int* pColorX, int* pColorY)
	{
		return (Status)oniCoordinateConverterDepthToColor(depthStream._getHandle(), colorStream._getHandle(), depthX, depthY, depthZ, pColorX, pColorY);
	}
};

/**
 * The Recorder class is used to record streams to an ONI file.
 *
 * After a recorder is instantiated, it must be initialized with a specific filename where
 * the recording will be stored.  The recorder is then attached to one or more streams.  Once 
 * this is complete, the recorder can be told to start recording.  The recorder will store
 * every frame from every stream to the specified file.  Later, this file can be used to 
 * initialize a file Device, and used to play back the same data that was recorded.
 *
 * Opening a file device is done by passing its path as the uri to the @ref Device::open() method.
 * 
 * @see PlaybackControl for options available to play a reorded file.
 * 
 */
class Recorder
{
public:
    /**
     * Creates a recorder. The recorder is not valid, i.e. @ref isValid() returns
     * false. You must initialize the recorder before use with @ref create().
     */
    Recorder() : m_recorder(NULL)
    {
    }

    /**
     * Destroys a recorder. This will also stop recording.
     */
    ~Recorder()
    {
		destroy();
    }

    /**
     * Initializes a recorder. You can initialize the recorder only once.  Attempts
	 * to intialize more than once will result in an error code being returned.
	 *
	 * Initialization assigns the recorder to an output file that will be used for 
	 * recording.  Before use, the @ref attach() function must also be used to assign input
	 * data to the Recorder.
	 *
     * @param	[in]	fileName	The name of a file which will contain the recording.
	 * @returns Status code which indicates success or failure of the operation.
     */
    Status create(const char* fileName)
    {
        if (!isValid())
        {
            return (Status)oniCreateRecorder(fileName, &m_recorder);
        }
        return STATUS_ERROR;
    }

    /**
     * Verifies if the recorder is valid, i.e. if one can record with this recorder.  A
	 * recorder object is not valid until the @ref create() method is called.
	 *
     * @returns true if the recorder has been intialized, false otherwise.
     */
    bool isValid() const
    {
        return NULL != getHandle();
    }

    /**
     * Attaches a stream to the recorder. Note, this won't start recording, you
     * should explicitly start it using @ref start() method. As soon as the recording
     * process has been started, no more streams can be attached to the recorder.
	 *
	 * @param [in] stream	The stream to be recorded.
	 * @param [in] allowLossyCompression [Optional] If this value is true, the recorder might use
	 *	a lossy compression, which means that when the recording will be played-back, there might
	 *  be small differences from the original frame. Default value is false.
     */
    Status attach(VideoStream& stream, bool allowLossyCompression = false)
    {
        if (!isValid() || !stream.isValid())
        {
            return STATUS_ERROR;
        }
        return (Status)oniRecorderAttachStream(
                m_recorder,
                stream._getHandle(),
                allowLossyCompression);
    }

    /**
     * Starts recording. 
	 * Once this method is called, the recorder will take all subsequent frames from the attached streams
	 * and store them in the file.
	 * You may not attach additional streams once recording was started.
     */
    Status start()
    {
		if (!isValid())
		{
			return STATUS_ERROR;
		}
        return (Status)oniRecorderStart(m_recorder);
    }

    /**
     * Stops recording. You may use @ref start() to resume the recording.
     */
    void stop()
    {
		if (isValid())
		{
			oniRecorderStop(m_recorder);
		}
    }

	/**
	Destroys the recorder object.
	*/
	void destroy()
	{
		if (isValid())
		{
			oniRecorderDestroy(&m_recorder);
		}
	}

private:
	Recorder(const Recorder&);
	Recorder& operator=(const Recorder&);

    /**
     * Returns a handle of this recorder.
     */
    OniRecorderHandle getHandle() const
    {
        return m_recorder;
    }


    OniRecorderHandle m_recorder;
};

// Implemetation
Status VideoStream::create(const Device& device, SensorType sensorType)
{
	OniStreamHandle streamHandle;
	Status rc = (Status)oniDeviceCreateStream(device._getHandle(), (OniSensorType)sensorType, &streamHandle);
	if (rc != STATUS_OK)
	{
		return rc;
	}

	m_isOwner = true;
	_setHandle(streamHandle);

	if (isPropertySupported(STREAM_PROPERTY_AUTO_WHITE_BALANCE) && isPropertySupported(STREAM_PROPERTY_AUTO_EXPOSURE))
	{
		m_pCameraSettings = new CameraSettings(this);
	}

	return STATUS_OK;
}

void VideoStream::destroy()
{
	if (!isValid())
	{
		return;
	}

	if (m_pCameraSettings != NULL)
	{
		delete m_pCameraSettings;
		m_pCameraSettings = NULL;
	}

	if (m_stream != NULL)
	{
		if(m_isOwner)
			oniStreamDestroy(m_stream);
		m_stream = NULL;
	}
}

Status Device::open(const char* uri)
{
	//If we are not the owners, we stick with our own device
	if(!m_isOwner)
	{
		if(isValid()){
			return STATUS_OK;
		}else{
			return STATUS_OUT_OF_FLOW;
		}
	}

	OniDeviceHandle deviceHandle;
	Status rc = (Status)oniDeviceOpen(uri, &deviceHandle);
	if (rc != STATUS_OK)
	{
		return rc;
	}

	_setHandle(deviceHandle);

	return STATUS_OK;
}

Status Device::_openEx(const char* uri, const char* mode)
{
	//If we are not the owners, we stick with our own device
	if(!m_isOwner)
	{
		if(isValid()){
			return STATUS_OK;
		}else{
			return STATUS_OUT_OF_FLOW;
		}
	}

	OniDeviceHandle deviceHandle;
	Status rc = (Status)oniDeviceOpenEx(uri, mode, &deviceHandle);
	if (rc != STATUS_OK)
	{
		return rc;
	}

	_setHandle(deviceHandle);

	return STATUS_OK;
}

Status Device::_setHandle(OniDeviceHandle deviceHandle)
{
	if (m_device == NULL)
	{
		m_device = deviceHandle;

		clearSensors();

		oniDeviceGetInfo(m_device, &m_deviceInfo);

		if (isFile())
		{
			m_pPlaybackControl = new PlaybackControl(this);
		}

		// Read deviceInfo
		return STATUS_OK;
	}

	return STATUS_OUT_OF_FLOW;
}

void Device::close()
{
	if (m_pPlaybackControl != NULL)
	{
		delete m_pPlaybackControl;
		m_pPlaybackControl = NULL;
	}

	if (m_device != NULL)
	{
		if(m_isOwner)
		{
			oniDeviceClose(m_device);
		}

		m_device = NULL;
	}
}


}

#endif // _OPEN_NI_HPP_
