#pragma once

#include "ModuleManager.h"

class REALSENSE_API IRealSensePlugin : public IModuleInterface
{
public:

	static inline IRealSensePlugin& Get()
	{
		return FModuleManager::LoadModuleChecked<IRealSensePlugin>("RealSense");
	}

	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("RealSense");
	}

	virtual int GenId() = 0;
	virtual class URealSenseContext* GetContext() = 0;
};
