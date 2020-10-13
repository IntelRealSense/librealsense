#pragma once

#include "RealSenseTypes.h"
#include "RealSenseDevice.generated.h"

UCLASS(ClassGroup="RealSense", BlueprintType)
class REALSENSE_API URealSenseDevice : public UObject
{
	GENERATED_UCLASS_BODY()
	friend class URealSenseContext;
	friend class ARealSenseInspector;
	friend class FRealSenseInspectorCustomization;

public:

	virtual ~URealSenseDevice();
	struct rs2_device* GetHandle();

	UFUNCTION(Category = "RealSense", BlueprintCallable)
	class URealSenseSensor* GetSensor(ERealSenseStreamType StreamType);

	UFUNCTION(Category="RealSense", BlueprintCallable)
	TArray<FRealSenseStreamProfile> GetStreamProfiles(ERealSenseStreamType StreamType);

	UFUNCTION(Category="RealSense", BlueprintCallable)
	bool SupportsProfile(FRealSenseStreamProfile Profile);

	UFUNCTION(Category="RealSense", BlueprintCallable)
	bool LoadPreset(const FString& FileName);

	UFUNCTION(Category="RealSense", BlueprintCallable)
	bool SavePreset(const FString& FileName);

	UPROPERTY(Category="RealSense", BlueprintReadOnly, VisibleAnywhere)
	FString Name;

	UPROPERTY(Category="RealSense", BlueprintReadOnly, VisibleAnywhere)
	FString Serial;

	UPROPERTY(Category="RealSense", BlueprintReadOnly, VisibleAnywhere)
	TArray<class URealSenseSensor*> Sensors;

	static bool LoadPreset(struct rs2_device* Handle, const FString& FileName);
	static bool SavePreset(struct rs2_device* Handle, const FString& FileName);

private:

	void SetHandle(struct rs2_device* Handle);
	class URealSenseSensor* NewSensor(struct rs2_sensor* Handle, const TCHAR* Name);
	void QueryData();

	struct rs2_device* RsDevice = nullptr;
};
