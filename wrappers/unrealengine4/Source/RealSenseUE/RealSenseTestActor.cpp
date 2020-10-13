#include "RealSenseTestActor.h"
#include "RealSenseUE.h"

// wrapper headers
#include "RealSensePlugin.h"
#include "RealSenseContext.h"
#include "RealSenseDevice.h"

// native headers (optional)
#include "RealSenseNative.h"

ARealSenseTestActor::ARealSenseTestActor()
{
	PrimaryActorTick.bCanEverTick = true;

	auto Context = IRealSensePlugin::Get().GetContext();
	Context->QueryDevices();
	for (auto Device : Context->Devices)
	{
		UE_LOG(LogRealSenseDemo, Display, TEXT("Wrapper device %s"), *(Device->Name));
	}

	#if 0
	rs2::context_ref RsContext(Context->GetHandle());
	auto DeviceList = RsContext.query_devices();
	for (auto Device : DeviceList)
	{
		FString DevName(ANSI_TO_TCHAR(Device.get_info(RS2_CAMERA_INFO_NAME)));
		UE_LOG(LogRealSenseDemo, Display, TEXT("Native device %s"), *DevName);
	}
	#endif
}

void ARealSenseTestActor::BeginPlay()
{
	Super::BeginPlay();
}

void ARealSenseTestActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
