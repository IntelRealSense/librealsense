#include "RealSenseSensor.h"
#include "PCH.h"

URealSenseSensor::URealSenseSensor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	//REALSENSE_TRACE(TEXT("+URealSenseSensor_%p"), this);
}

URealSenseSensor::~URealSenseSensor()
{
	//REALSENSE_TRACE(TEXT("~URealSenseSensor_%p"), this);

	SetHandle(nullptr);
}

void URealSenseSensor::SetHandle(rs2_sensor* Handle)
{
	if (RsSensor) rs2_delete_sensor(RsSensor);
	RsSensor = Handle;
}

rs2_sensor* URealSenseSensor::GetHandle()
{
	return RsSensor;
}

URealSenseOption* URealSenseSensor::NewOption(rs2_options* Handle, ERealSenseOptionType Type, const TCHAR* ObjName)
{
	auto Option = NewNamedObject<URealSenseOption>(this, ObjName);
	Option->SetHandle(Handle);
	Option->SetType(Type);
	return Option;
}

void URealSenseSensor::QueryData()
{
	Options.Empty();
	StreamProfiles.Empty();

	rs2::error_ref e;
	Name = uestr(rs2_get_sensor_info(RsSensor, RS2_CAMERA_INFO_NAME, &e));

	for (int i = 0; i < RS2_OPTION_COUNT; ++i)
	{
		if (SupportsOption((ERealSenseOptionType)i))
		{
			Options.Add(NewOption((rs2_options*)RsSensor, (ERealSenseOptionType)i, TEXT("RealSenseOption")));
		}
	}

	rs2_stream_profile_list* ProfileList = rs2_get_stream_profiles(RsSensor, &e);
	if (ProfileList)
	{
		const int NumProfiles = rs2_get_stream_profiles_count(ProfileList, &e);
		for (int Id = 0; Id < NumProfiles; Id++)
		{
			const rs2_stream_profile* Profile = rs2_get_stream_profile(ProfileList, Id, &e);
			if (Profile)
			{
				rs2_stream Stream;
				rs2_format Format;
				int StreamId;
				int ProfileId;
				int Framerate;
				rs2_get_stream_profile_data(Profile, &Stream, &Format, &StreamId, &ProfileId, &Framerate, &e);

				if (e.success())
				{
					if ((Stream == RS2_STREAM_DEPTH && Format == RS2_FORMAT_Z16)
						|| (Stream == RS2_STREAM_COLOR && Format == RS2_FORMAT_RGBA8)
						|| (Stream == RS2_STREAM_INFRARED && Format == RS2_FORMAT_Y8))
					{
						FRealSenseStreamProfile spi;
						spi.StreamType = (ERealSenseStreamType)Stream;
						spi.Format = (ERealSenseFormatType)Format;
						spi.Rate = Framerate;
						rs2_get_video_stream_resolution(Profile, &spi.Width, &spi.Height, &e);

						if (e.success())
						{
							StreamProfiles.Add(spi);
						}
					}
				}
			}
		}
	}

	for (auto* Option : Options)
	{
		Option->QueryData();
	}
}

bool URealSenseSensor::SupportsOption(ERealSenseOptionType Option)
{
	rs2::error_ref e;
	const int Value = rs2_supports_option((const rs2_options*)RsSensor, (rs2_option)Option, &e);
	return (e.success() && Value == (int)true);
}

bool URealSenseSensor::SupportsProfile(FRealSenseStreamProfile Profile)
{
	for (const auto& Iter : StreamProfiles)
	{
		if (Iter.StreamType == Profile.StreamType 
			&& Iter.Format == Profile.Format 
			&& Iter.Width == Profile.Width 
			&& Iter.Height == Profile.Height 
			&& Iter.Rate == Profile.Rate)
		{
			return true;
		}
	}
	return false;
}

URealSenseOption* URealSenseSensor::GetOption(ERealSenseOptionType OptionType)
{
	URealSenseOption* Result = nullptr;

	for (auto* Option : Options)
	{
		if (Option->Type == OptionType)
		{
			Result = Option;
			break;
		}
	}

	return Result;
}