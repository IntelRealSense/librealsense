#include "RealSensePluginImpl.h"
#include "PCH.h"

#if defined(PROF_ENABLED)
#include "AllowWindowsPlatformTypes.h"
#include <windows.h>
#include "HideWindowsPlatformTypes.h"
#include "Profiler.inl"
#endif

DEFINE_LOG_CATEGORY(LogRealSense);

FRealSensePlugin::FRealSensePlugin()
{
	REALSENSE_TRACE(TEXT("+FRealSensePlugin_%p"), this);
}

FRealSensePlugin::~FRealSensePlugin()
{
	REALSENSE_TRACE(TEXT("~FRealSensePlugin_%p"), this);

	ReleaseContext();
}

void FRealSensePlugin::StartupModule()
{
	REALSENSE_TRACE(TEXT("FRealSensePlugin_%p::StartupModule"), this);

	INIT_PROFILER;

	#if WITH_EDITOR
	FRealSenseDetailCustomization::Register();
	#endif
}

void FRealSensePlugin::ShutdownModule()
{
	REALSENSE_TRACE(TEXT("FRealSensePlugin_%p::ShutdownModule"), this);

	ReleaseContext();

	#if WITH_EDITOR
	FRealSenseDetailCustomization::Unregister();
	#endif

	SHUT_PROFILER;
}

int FRealSensePlugin::GenId()
{
	FScopeLock Lock(&UniqIdMx);
	return UniqId++;
}

URealSenseContext* FRealSensePlugin::GetContext()
{
	if (!Context)
	{
		FScopeLock Lock(&ContextMx);
		if (!Context)
		{
			//FPlatformMisc::SetEnvironmentVar(TEXT("OMP_NUM_THREADS"), TEXT("1"));

			rs2::error_ref e;
			REALSENSE_TRACE(TEXT("rs2_create_context"));
			RsContext = rs2_create_context(RS2_API_VERSION, &e);
			if (!e.success())
			{
				REALSENSE_ERR(TEXT("rs2_create_context failed: "), *uestr(e.get_message()));
				return nullptr;
			}

			Context = NewNamedObject<URealSenseContext>(nullptr, TEXT("RealSenseContext"));
			Context->AddToRoot();
			Context->SetHandle(RsContext);
			Context->QueryDevices();
		}
	}

	return Context;
}

void FRealSensePlugin::ReleaseContext()
{
	FScopeLock Lock(&ContextMx);

	if (RsContext)
	{
		REALSENSE_TRACE(TEXT("rs2_delete_context %X"), RsContext);
		rs2_delete_context(RsContext);
		RsContext = nullptr;
	}

	Context = nullptr;
}

IMPLEMENT_MODULE(FRealSensePlugin, RealSense)
