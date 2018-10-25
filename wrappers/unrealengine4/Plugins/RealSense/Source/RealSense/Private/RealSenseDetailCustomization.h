#pragma once

#if WITH_EDITOR

#include "IDetailCustomization.h"
#include "IPropertyTypeCustomization.h"
#include "PropertyHandle.h"

namespace rs2 { class device_list; }

class FRealSenseDetailCustomization
{
public:
	static void Register();
	static void Unregister();
};

class FRealSenseInspectorCustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	FRealSenseInspectorCustomization();
	virtual void CustomizeDetails(class IDetailLayoutBuilder& DetailLayout) override;

private:
	void UpdateDevices();
	bool PickFile(FString& OutPath, const void* ParentWindow, const FString& Title, const FString& Filter, bool SaveFlag = false);

	IDetailLayoutBuilder* DetailLayoutPtr;
	TUniquePtr<class rs2::device_list> RsDevices;
	TMap<FString, float> ValueCache;
};

#endif // WITH_EDITOR
