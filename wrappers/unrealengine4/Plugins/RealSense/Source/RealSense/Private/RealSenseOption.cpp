#include "RealSenseOption.h"
#include "PCH.h"

URealSenseOption::URealSenseOption(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	//REALSENSE_TRACE(TEXT("+URealSenseOption_%p"), this);

	Range.Min = 0.0f;
	Range.Max = 1.0f;
	Range.Step = 0.1f;
	Range.Default = 0.5f;
}

URealSenseOption::~URealSenseOption()
{
	//REALSENSE_TRACE(TEXT("~URealSenseOption_%p"), this);
}

void URealSenseOption::SetHandle(rs2_options* Handle)
{
	RsOptions = Handle; // rs2_options is managed by URealSenseSensor, just keep a pointer
}

rs2_options* URealSenseOption::GetHandle()
{
	return RsOptions;
}

void URealSenseOption::SetType(ERealSenseOptionType NewType)
{
	this->Type = NewType;
}

void URealSenseOption::QueryData()
{
	rs2::error_ref e;
	Name = uestr(rs2_option_to_string((rs2_option)Type));
	Description = uestr(rs2_get_option_description(RsOptions, (rs2_option)Type, &e));
	rs2_get_option_range(RsOptions, (rs2_option)Type, &Range.Min, &Range.Max, &Range.Step, &Range.Default, &e);
	QueryValue();
}

float URealSenseOption::QueryValue()
{
	rs2::error_ref e;
	Value = rs2_get_option(RsOptions, (rs2_option)Type, &e);
	if (!e.success())
	{
		REALSENSE_ERR(TEXT("rs2_get_option failed: %s"), *uestr(e.get_message()));
	}
	return Value;
}

void URealSenseOption::SetValue(float NewValue)
{
	NewValue = FMath::RoundToFloat(NewValue / Range.Step) * Range.Step;
	rs2::error_ref e;
	rs2_set_option(RsOptions, (rs2_option)Type, NewValue, &e);
	if (!e.success())
	{
		REALSENSE_ERR(TEXT("rs2_set_option failed: %s"), *uestr(e.get_message()));
	}
	this->Value = NewValue;
}
