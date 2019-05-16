#include "RealSenseDevice.h"
#include "PCH.h"
#include <librealsense2/rs_advanced_mode.hpp>

URealSenseDevice::URealSenseDevice(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	REALSENSE_TRACE(TEXT("+URealSenseDevice_%p"), this);
}

URealSenseDevice::~URealSenseDevice()
{
	REALSENSE_TRACE(TEXT("~URealSenseDevice_%p"), this);

	SetHandle(nullptr);
}

void URealSenseDevice::SetHandle(rs2_device* Handle)
{
	if (RsDevice) { rs2_delete_device(RsDevice); }
	RsDevice = Handle;
}

rs2_device* URealSenseDevice::GetHandle()
{
	return RsDevice;
}

URealSenseSensor* URealSenseDevice::NewSensor(rs2_sensor* Handle, const TCHAR* ObjName)
{
	auto Sensor = NewNamedObject<URealSenseSensor>(this, ObjName);
	Sensor->SetHandle(Handle);
	return Sensor;
}

void URealSenseDevice::QueryData()
{
	Sensors.Empty();

	rs2::error_ref e;
	Name = uestr(rs2_get_device_info(RsDevice, RS2_CAMERA_INFO_NAME, &e));
	Serial = uestr(rs2_get_device_info(RsDevice, RS2_CAMERA_INFO_SERIAL_NUMBER, &e));

	rs2_sensor_list* List = rs2_query_sensors(RsDevice, &e);
	if (List)
	{
		const int Count = rs2_get_sensors_count(List, &e);
		for (int i = 0; i < Count; i++)
		{
			rs2_sensor* RsSensor = rs2_create_sensor(List, i, &e);
			if (RsSensor)
			{
				Sensors.Add(NewSensor(RsSensor, TEXT("RealSenseSensor")));
			}
		}

		rs2_delete_sensor_list(List);
	}

	for (auto* Sensor : Sensors)
	{
		Sensor->QueryData();
	}
}

URealSenseSensor* URealSenseDevice::GetSensor(ERealSenseStreamType StreamType)
{
	URealSenseSensor* Result = nullptr;

	for (auto* Sensor : Sensors)
	{
		for (const auto& Profile : Sensor->StreamProfiles)
		{
			if (Profile.StreamType == StreamType || StreamType == ERealSenseStreamType::STREAM_ANY)
			{
				Result = Sensor;
				break;
			}
		}
	}

	return Result;
}

TArray<FRealSenseStreamProfile> URealSenseDevice::GetStreamProfiles(ERealSenseStreamType StreamType)
{
	TArray<FRealSenseStreamProfile> Result;

	for (auto* Sensor : Sensors)
	{
		for (const auto& Profile : Sensor->StreamProfiles)
		{
			if (Profile.StreamType == StreamType || StreamType == ERealSenseStreamType::STREAM_ANY)
			{
				Result.Add(Profile);
			}
		}
	}

	return Result;
}

bool URealSenseDevice::SupportsProfile(FRealSenseStreamProfile Profile)
{
	for (auto* Sensor : Sensors)
	{
		if (Sensor->SupportsProfile(Profile))
		{
			return true;
		}
	}

	return false;
}

bool URealSenseDevice::LoadPreset(const FString& FileName)
{
	return LoadPreset(RsDevice, FileName);
}

bool URealSenseDevice::SavePreset(const FString& FileName)
{
	return SavePreset(RsDevice, FileName);
}

bool URealSenseDevice::LoadPreset(rs2_device* Handle, const FString& FileName)
{
	bool Result = false;
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	IFileHandle* FileHandle = PlatformFile.OpenRead(*FileName);
	if (!FileHandle)
	{
		REALSENSE_ERR(TEXT("OpenRead failed: %s"), *FileName);
	}
	else
	{
		auto Size = FileHandle->Size();
		std::vector<uint8> Data;
		Data.resize(Size);

		if (!FileHandle->Read(&Data[0], Size))
		{
			REALSENSE_ERR(TEXT("Read failed: %s Size=%u"), *FileName, (unsigned int)Size);
		}
		else
		{
			rs2::error_ref e;
			rs2_load_json(Handle, &Data[0], Size, &e);
			if (!e.success())
			{
				REALSENSE_ERR(TEXT("rs2_load_json failed: %s"), ANSI_TO_TCHAR(e.get_message()));
			}
			else
			{
				Result = true;
			}
		}

		delete FileHandle;
	}
	return Result;
}

bool URealSenseDevice::SavePreset(rs2_device* Handle, const FString& FileName)
{
	bool Result = false;
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	IFileHandle* FileHandle = PlatformFile.OpenWrite(*FileName);
	if (!FileHandle)
	{
		REALSENSE_ERR(TEXT("OpenWrite failed: %s"), *FileName);
	}
	else
	{
		rs2::error_ref e;
		auto Buffer = rs2_serialize_json(Handle, &e);
		if (!e.success())
		{
			REALSENSE_ERR(TEXT("rs2_serialize_json failed: %s"), ANSI_TO_TCHAR(e.get_message()));
		}
		else
		{
			auto Size = rs2_get_raw_data_size(Buffer, &e);
			auto Data = rs2_get_raw_data(Buffer, &e);

			if (!FileHandle->Write(Data, Size))
			{
				REALSENSE_ERR(TEXT("Write failed: %s"), *FileName);
			}
			else
			{
				Result = true;
			}

			rs2_delete_raw_data(Buffer);
		}

		delete FileHandle;
	}
	return Result;
}
