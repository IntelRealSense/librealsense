#include "RealSenseContext.h"
#include "PCH.h"

URealSenseContext::URealSenseContext(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	REALSENSE_TRACE(TEXT("+URealSenseContext_%p"), this);
}

URealSenseContext::~URealSenseContext()
{
	REALSENSE_TRACE(TEXT("~URealSenseContext_%p"), this);
}

void URealSenseContext::SetHandle(rs2_context* Handle)
{
	RsContext = Handle; // rs2_context is managed by FRealSensePlugin, just keep a pointer
}

rs2_context* URealSenseContext::GetHandle()
{
	return RsContext;
}

URealSenseContext* URealSenseContext::GetRealSense() // static
{
	return IRealSensePlugin::Get().GetContext();
}

URealSenseDevice* URealSenseContext::NewDevice(rs2_device* Handle, const TCHAR* Name)
{
	auto Device = NewNamedObject<URealSenseDevice>(this, Name);
	Device->SetHandle(Handle);
	return Device;
}

void URealSenseContext::QueryDevices()
{
	REALSENSE_TRACE(TEXT("URealSenseContext_%p::QueryDevices"), this);

	FScopeLock Lock(&DevicesMx);

	Devices.Empty();

	rs2::error_ref e;
	rs2_device_list* List = rs2_query_devices(RsContext, &e);
	if (List)
	{
		const int Count = rs2_get_device_count(List, &e);
		for (int i = 0; i < Count; i++)
		{
			rs2_device* RsDevice = rs2_create_device(List, i, &e);
			if (RsDevice)
			{
				Devices.Add(NewDevice(RsDevice, TEXT("RealSenseDevice")));
			}
		}

		rs2_delete_device_list(List);
	}

	for (auto* Device : Devices)
	{
		Device->QueryData();
	}
}

URealSenseDevice* URealSenseContext::GetDeviceById(int Id)
{
	FScopeLock Lock(&DevicesMx);

	return (Id < 0 || Id >= Devices.Num()) ? nullptr : Devices[Id];
}

URealSenseDevice* URealSenseContext::FindDeviceBySerial(FString Serial)
{
	FScopeLock Lock(&DevicesMx);

	for (auto* Device : Devices)
	{
		if (Device->Serial.Equals(Serial))
		{
			return Device;
		}
	}

	return nullptr;
}
