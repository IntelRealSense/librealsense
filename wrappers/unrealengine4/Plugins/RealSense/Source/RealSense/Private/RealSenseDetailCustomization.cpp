#include "RealSenseDetailCustomization.h"
#include "PCH.h"

#if WITH_EDITOR

#include "IDetailsView.h"
#include "IDetailCustomization.h"
#include "IDetailGroup.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "EditorDirectories.h"
#include "DesktopPlatformModule.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SNumericEntryBox.h"
#include "Widgets/Input/SFilePathPicker.h"

//
// FRealSenseDetailCustomization
//

void FRealSenseDetailCustomization::Register()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout("RealSenseInspector", FOnGetDetailCustomizationInstance::CreateStatic(&FRealSenseInspectorCustomization::MakeInstance));
	//PropertyModule.RegisterCustomPropertyTypeLayout("RealSenseStreamMode", FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FRealSenseStreamModeCustomization::MakeInstance));
	PropertyModule.NotifyCustomizationModuleChanged(); // LAST
}

void FRealSenseDetailCustomization::Unregister()
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.UnregisterCustomClassLayout("RealSenseInspector");
	//PropertyModule.UnregisterCustomPropertyTypeLayout("RealSenseStreamMode");
}

//
// FRealSenseInspectorCustomization
//

TSharedRef<IDetailCustomization> FRealSenseInspectorCustomization::MakeInstance() { return MakeShareable(new FRealSenseInspectorCustomization); }

FRealSenseInspectorCustomization::FRealSenseInspectorCustomization()
{
	DetailLayoutPtr = nullptr;
	RsDevices.Reset(new rs2::device_list());
}

void FRealSenseInspectorCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	SCOPED_PROFILER;
	REALSENSE_TRACE(TEXT("FRealSenseInspectorCustomization::CustomizeDetails"));

	if (!IRealSensePlugin::Get().GetContext()) return;

	DetailLayoutPtr = &DetailLayout;
	auto This = this;
	auto* Category = &DetailLayout.EditCategory("Hardware Inspector");
	auto ParentWindow = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);

	if (RsDevices->size() == 0)
	{
		UpdateDevices();
	}

	Category->AddCustomRow(FText::FromString(TEXT("Refresh")))
		.WholeRowContent()
		[
			SNew(SButton)
			.Text(FText::FromString(TEXT("Refresh")))
			.OnClicked_Lambda([This]()
			{
				This->UpdateDevices();
				This->DetailLayoutPtr->ForceRefreshDetails();
				return FReply::Handled();
			})
		]
	;

	for (auto Device : *RsDevices)
	{
		FString DevName = uestr(Device.get_info(RS2_CAMERA_INFO_NAME));
		FString DevSerial = uestr(Device.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
		auto* DeviceGroup = &Category->AddGroup(TEXT("Device"), FText::FromString(DevName));

		DeviceGroup->AddWidgetRow()
			.NameContent()
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("Serial Number")))
				.Font(IDetailLayoutBuilder::GetDetailFont())
			]
			.ValueContent()
			[
				SNew(STextBlock)
				.Text(FText::FromString(DevSerial))
				.Font(IDetailLayoutBuilder::GetDetailFont())
			]
		;

		DeviceGroup->AddWidgetRow()
			.NameContent()
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Load preset")))
				.OnClicked_Lambda([This, Device, ParentWindow]()
				{
					FString Path;
					if (This->PickFile(Path, ParentWindow, TEXT("Select preset"), TEXT("Preset file|*.json")))
					{
						REALSENSE_TRACE(TEXT("Load preset %s"), *Path);
						if (URealSenseDevice::LoadPreset(Device.get().get(), Path))
						{
							This->UpdateDevices();
						}
					}
					return FReply::Handled();
				})
			]
		;

		DeviceGroup->AddWidgetRow()
			.NameContent()
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("Save preset")))
				.OnClicked_Lambda([This, Device, ParentWindow]()
				{
					FString Path;
					if (This->PickFile(Path, ParentWindow, TEXT("Select preset"), TEXT("Preset file|*.json"), true))
					{
						REALSENSE_TRACE(TEXT("Save preset %s"), *Path);
						URealSenseDevice::SavePreset(Device.get().get(), Path);
					}
					return FReply::Handled();
				})
			]
		;

		for (auto Sensor : Device.query_sensors())
		{
			FString SensorName = uestr(Sensor.get_info(RS2_CAMERA_INFO_NAME));
			auto* SensorGroup = &DeviceGroup->AddGroup(TEXT("Sensor"), FText::FromString(SensorName));

			for (int i = 0; i < RS2_OPTION_COUNT; ++i)
			{
				const rs2_option Option = (rs2_option)i;

				if (Sensor.supports(Option))
				{
					auto OptionName = uestr(rs2_option_to_string(Option));
					auto Range = Sensor.get_option_range(Option);

					auto UniqId = DevName + "_" + SensorName + "_" + OptionName;
					if (!ValueCache.Contains(UniqId))
					{
						auto Value = Sensor.get_option(Option);
						ValueCache.Add(UniqId, Value);
					}

					SensorGroup->AddWidgetRow()
						.NameContent()
						[
							SNew(STextBlock)
							.Text(FText::FromString(OptionName))
							.Font(IDetailLayoutBuilder::GetDetailFont())
						]
						.ValueContent()
						[
							SNew(SNumericEntryBox<float>)
							.MinValue(Range.min)
							.MinSliderValue(Range.min)
							.MaxValue(Range.max)
							.MaxSliderValue(Range.max)
							.AllowSpin(true)
							.Value_Lambda([This, UniqId]()
							{
								return This->ValueCache.Contains(UniqId) ? This->ValueCache[UniqId] : 0;
							})
							.OnValueChanged_Lambda([This, Sensor, Option, UniqId, Range](float NewValue)
							{
								NewValue = FMath::RoundToFloat(NewValue / Range.step) * Range.step;
								Sensor.set_option(Option, NewValue);
								if (This->ValueCache.Contains(UniqId)) This->ValueCache[UniqId] = NewValue; else This->ValueCache.Add(UniqId, NewValue);
							})
						]
					;
				}
			}
		}
	}
}

void FRealSenseInspectorCustomization::UpdateDevices()
{
	SCOPED_PROFILER;
	REALSENSE_TRACE(TEXT("FRealSenseInspectorCustomization::UpdateDevices"));

	rs2::context_ref RsContext(IRealSensePlugin::Get().GetContext()->GetHandle());
	*RsDevices = RsContext.query_devices();
}

bool FRealSenseInspectorCustomization::PickFile(FString& OutPath, const void* ParentWindow, const FString& Title, const FString& Filter, bool SaveFlag)
{
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (DesktopPlatform)
	{
		TArray<FString> OutFiles;
		if (SaveFlag)
		{
			auto DefaultPath = FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_SAVE);
			if (DesktopPlatform->SaveFileDialog(ParentWindow, Title, DefaultPath, TEXT(""), Filter, EFileDialogFlags::None, OutFiles))
			{
				OutPath = OutFiles[0];
				return true;
			}
		}
		else
		{
			auto DefaultPath = FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_OPEN);
			if (DesktopPlatform->OpenFileDialog(ParentWindow, Title, DefaultPath, TEXT(""), Filter, EFileDialogFlags::None, OutFiles))
			{
				OutPath = OutFiles[0];
				return true;
			}
		}
	}
	return false;
}

#endif // WITH_EDITOR
