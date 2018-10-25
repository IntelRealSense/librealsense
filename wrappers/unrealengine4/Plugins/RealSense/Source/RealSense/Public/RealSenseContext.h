#pragma once

#include "RealSenseTypes.h"
#include "RealSenseContext.generated.h"

UCLASS(ClassGroup="RealSense", BlueprintType)
class REALSENSE_API URealSenseContext : public UObject
{
	GENERATED_UCLASS_BODY()
	friend class FRealSensePlugin;

public:

	virtual ~URealSenseContext();
	struct rs2_context* GetHandle();

	UFUNCTION(Category="RealSense", BlueprintCallable)
	static URealSenseContext* GetRealSense();

	UFUNCTION(Category="RealSense", BlueprintCallable)
	void QueryDevices();

	UFUNCTION(Category="RealSense", BlueprintCallable)
	class URealSenseDevice* GetDeviceById(int Id);

	UFUNCTION(Category="RealSense", BlueprintCallable)
	class URealSenseDevice* FindDeviceBySerial(FString Serial);

	UPROPERTY(Category="RealSense", BlueprintReadOnly, VisibleAnywhere)
	TArray<class URealSenseDevice*> Devices;

private:

	void SetHandle(struct rs2_context* Handle);
	class URealSenseDevice* NewDevice(struct rs2_device* Handle, const TCHAR* Name);

	struct rs2_context* RsContext = nullptr;
	FCriticalSection DevicesMx;
};
