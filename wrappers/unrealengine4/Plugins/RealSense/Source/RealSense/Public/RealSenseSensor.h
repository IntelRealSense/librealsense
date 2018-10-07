#pragma once

#include "RealSenseTypes.h"
#include "RealSenseSensor.generated.h"

UCLASS(ClassGroup="RealSense", BlueprintType)
class REALSENSE_API URealSenseSensor : public UObject
{
	GENERATED_UCLASS_BODY()
	friend class URealSenseDevice;

public:

	virtual ~URealSenseSensor();
	struct rs2_sensor* GetHandle();

	UFUNCTION(Category="RealSense", BlueprintCallable)
	bool SupportsOption(ERealSenseOptionType Option);

	UFUNCTION(Category="RealSense", BlueprintCallable)
	bool SupportsProfile(FRealSenseStreamProfile Profile);

	UFUNCTION(Category = "RealSense", BlueprintCallable)
	class URealSenseOption* GetOption(ERealSenseOptionType OptionType);

	UPROPERTY(Category="RealSense", BlueprintReadOnly, VisibleAnywhere)
	FString Name;

	UPROPERTY(Category="RealSense", BlueprintReadOnly, VisibleAnywhere)
	TArray<class URealSenseOption*> Options;

	UPROPERTY(Category="RealSense", BlueprintReadOnly, VisibleAnywhere)
	TArray<FRealSenseStreamProfile> StreamProfiles;

private:

	void SetHandle(struct rs2_sensor* Handle);
	class URealSenseOption* NewOption(struct rs2_options* Handle, ERealSenseOptionType Type, const TCHAR* Name);
	void QueryData();

	struct rs2_sensor* RsSensor = nullptr;
};
