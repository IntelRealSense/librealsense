// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ModuleManager.h"

/**
* The public interface to this module
*/
class IRuntimeMeshComponentPlugin : public IModuleInterface
{

public:

	/**
	* Singleton-like access to this module's interface.  This is just for convenience!
	* Beware of calling this during the shutdown phase, though.  Your module might have been unloaded already.
	*
	* @return Returns singleton instance, loading the module on demand if needed
	*/
	static inline IRuntimeMeshComponentPlugin& Get()
	{
		return FModuleManager::LoadModuleChecked<IRuntimeMeshComponentPlugin>("RuntimeMeshComponent");
	}

	/**
	* Checks to see if this module is loaded and ready.  It is only valid to call Get() if IsAvailable() returns true.
	*
	* @return True if the module is loaded and ready to use
	*/
	static inline bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("RuntimeMeshComponent");
	}
};

DECLARE_LOG_CATEGORY_EXTERN(RuntimeMeshLog, Log, All);
