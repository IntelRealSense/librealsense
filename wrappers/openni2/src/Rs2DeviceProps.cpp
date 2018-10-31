#include "Rs2Driver.h"
#include "PS1080.h"

namespace oni { namespace driver {

OniStatus Rs2Device::setProperty(int propertyId, const void* data, int dataSize)
{
	SCOPED_PROFILER;

	rsTraceFunc("propertyId=%d dataSize=%d", propertyId, dataSize);

	switch (propertyId)
	{
		case ONI_DEVICE_PROPERTY_IMAGE_REGISTRATION:
		{
			if (data && (dataSize == sizeof(OniImageRegistrationMode)))
			{
				m_registrationMode = *((OniImageRegistrationMode*)data);
				rsLogDebug("registrationMode=%d", (int)m_registrationMode);
				//updateConfiguration();
				return ONI_STATUS_OK;
			}
			break;
		}

		default:
		{
			#if defined(RS2_TRACE_NOT_SUPPORTED_PROPS)
				rsTraceError("Not supported: propertyId=%d", propertyId);
			#endif
			return ONI_STATUS_NOT_SUPPORTED;
		}
	}

	rsTraceError("propertyId=%d dataSize=%d", propertyId, dataSize);
	return ONI_STATUS_ERROR;
}

OniStatus Rs2Device::getProperty(int propertyId, void* data, int* dataSize)
{
	SCOPED_PROFILER;

	switch (propertyId)
	{
		case ONI_DEVICE_PROPERTY_SERIAL_NUMBER:
		{
			if (data && dataSize && *dataSize > 0)
			{
				Rs2Error e;
				const char* info = rs2_get_device_info(m_device, RS2_CAMERA_INFO_SERIAL_NUMBER, &e);
				if (e.success())
				{
					int n = snprintf((char*)data, *dataSize - 1, "%s", info);
					*dataSize = n + 1;
					return ONI_STATUS_OK;
				}
			}
			break;
		}

		case ONI_DEVICE_PROPERTY_IMAGE_REGISTRATION:
		{
			if (data && dataSize && *dataSize == sizeof(OniImageRegistrationMode))
			{
				*((OniImageRegistrationMode*)data) = m_registrationMode;
				return ONI_STATUS_OK;
			}
			break;
		}

		#ifdef RS2_EMULATE_PRIMESENSE_HARDWARE
		case XN_MODULE_PROPERTY_AHB:
		{
			if (data && dataSize && *dataSize == 12)
			{
				unsigned char hack[] = {0x40, 0x0, 0x0, 0x28, 0x6A, 0x26, 0x54, 0x4F, 0xFF, 0xFF, 0xFF, 0xFF};
				memcpy(data, hack, sizeof(hack));
				return ONI_STATUS_OK;
			}
			break;
		}
		#endif

		default:
		{
			#if defined(RS2_TRACE_NOT_SUPPORTED_PROPS)
				rsTraceError("Not supported: propertyId=%d", propertyId);
			#endif
			return ONI_STATUS_NOT_SUPPORTED;
		}
	}

	rsTraceError("propertyId=%d dataSize=%d", propertyId, *dataSize);
	return ONI_STATUS_ERROR;
}

OniBool Rs2Device::isPropertySupported(int propertyId)
{
	switch (propertyId)
	{
		case ONI_DEVICE_PROPERTY_SERIAL_NUMBER:
		case ONI_DEVICE_PROPERTY_IMAGE_REGISTRATION:
			return true;

		default:
			return false;
	}
}

}} // namespace
