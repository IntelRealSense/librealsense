// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#include "IRuntimeMeshComponentEditorPlugin.h"
#include "PropertyEditorModule.h"
#include "RuntimeMeshComponent.h"
#include "RuntimeMeshComponentDetails.h"



class FRuntimeMeshComponentEditorPlugin : public IRuntimeMeshComponentEditorPlugin
{
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE(FRuntimeMeshComponentEditorPlugin, RuntimeMeshComponentEditor)



void FRuntimeMeshComponentEditorPlugin::StartupModule()
{
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomClassLayout(URuntimeMeshComponent::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FRuntimeMeshComponentDetails::MakeInstance));
	}
}


void FRuntimeMeshComponentEditorPlugin::ShutdownModule()
{

}
