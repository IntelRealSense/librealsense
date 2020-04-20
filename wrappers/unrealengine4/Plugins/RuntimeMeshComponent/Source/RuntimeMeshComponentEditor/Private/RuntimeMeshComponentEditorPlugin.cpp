// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#include "IRuntimeMeshComponentEditorPlugin.h"
#include "PropertyEditorModule.h"
#include "RuntimeMeshComponent.h"
#include "RuntimeMeshComponentDetails.h"
#include "RuntimeMeshComponentEditorStyle.h"
#include "RuntimeMeshComponentEditorCommands.h"

#include "ModuleManager.h"
#include "LevelEditor.h"

class FToolBarBuilder;
class FMenuBuilder;

#define LOCTEXT_NAMESPACE "FRuntimeMeshComponentEditorModule"

class FRuntimeMeshComponentEditorPlugin : public IRuntimeMeshComponentEditorPlugin
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** This function will be bound to Command. */
	void DonateActionClicked();
	void HelpActionClicked();
	void ForumsActionClicked();
	void IssuesActionClicked();
	void DiscordActionClicked();
	void MarketplaceActionClicked();

private:

	void AddMenuBarExtension(FMenuBarBuilder& Builder);
	void AddMenuExtension(FMenuBuilder& Builder);

private:
	TSharedPtr<class FUICommandList> PluginCommands;
};

IMPLEMENT_MODULE(FRuntimeMeshComponentEditorPlugin, RuntimeMeshComponentEditor)



void FRuntimeMeshComponentEditorPlugin::StartupModule()
{
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomClassLayout(URuntimeMeshComponent::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FRuntimeMeshComponentDetails::MakeInstance));
	}

	FRuntimeMeshComponentEditorStyle::Initialize();
	FRuntimeMeshComponentEditorStyle::ReloadTextures();

	FRuntimeMeshComponentEditorCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FRuntimeMeshComponentEditorCommands::Get().DonateAction,
		FExecuteAction::CreateRaw(this, &FRuntimeMeshComponentEditorPlugin::DonateActionClicked),
		FCanExecuteAction());
	PluginCommands->MapAction(
		FRuntimeMeshComponentEditorCommands::Get().HelpAction,
		FExecuteAction::CreateRaw(this, &FRuntimeMeshComponentEditorPlugin::HelpActionClicked),
		FCanExecuteAction());
	PluginCommands->MapAction(
		FRuntimeMeshComponentEditorCommands::Get().ForumsAction,
		FExecuteAction::CreateRaw(this, &FRuntimeMeshComponentEditorPlugin::ForumsActionClicked),
		FCanExecuteAction());
	PluginCommands->MapAction(
		FRuntimeMeshComponentEditorCommands::Get().IssuesAction,
		FExecuteAction::CreateRaw(this, &FRuntimeMeshComponentEditorPlugin::IssuesActionClicked),
		FCanExecuteAction());
	PluginCommands->MapAction(
		FRuntimeMeshComponentEditorCommands::Get().DiscordAction,
		FExecuteAction::CreateRaw(this, &FRuntimeMeshComponentEditorPlugin::DiscordActionClicked),
		FCanExecuteAction());
	PluginCommands->MapAction(
		FRuntimeMeshComponentEditorCommands::Get().MarketplaceAction,
		FExecuteAction::CreateRaw(this, &FRuntimeMeshComponentEditorPlugin::MarketplaceActionClicked),
		FCanExecuteAction());

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	{
		TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
		MenuExtender->AddMenuBarExtension("Window", EExtensionHook::After, PluginCommands, FMenuBarExtensionDelegate::CreateRaw(this, &FRuntimeMeshComponentEditorPlugin::AddMenuBarExtension));
		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
	}
}

void FRuntimeMeshComponentEditorPlugin::ShutdownModule()
{
	FRuntimeMeshComponentEditorStyle::Shutdown();

	FRuntimeMeshComponentEditorCommands::Unregister();
}

void FRuntimeMeshComponentEditorPlugin::DonateActionClicked()
{
	FPlatformProcess::LaunchURL(TEXT("https://github.com/Koderz/RuntimeMeshComponent/wiki/Support-the-development!"), NULL, NULL);

}

void FRuntimeMeshComponentEditorPlugin::HelpActionClicked()
{
	FPlatformProcess::LaunchURL(TEXT("https://github.com/Koderz/UE4RuntimeMeshComponent/wiki"), NULL, NULL);
}

void FRuntimeMeshComponentEditorPlugin::ForumsActionClicked()
{
	FPlatformProcess::LaunchURL(TEXT("https://github.com/Koderz/UE4RuntimeMeshComponent/wiki"), NULL, NULL);

}

void FRuntimeMeshComponentEditorPlugin::IssuesActionClicked()
{
	FPlatformProcess::LaunchURL(TEXT("https://github.com/Koderz/UE4RuntimeMeshComponent/issues"), NULL, NULL);

}

void FRuntimeMeshComponentEditorPlugin::DiscordActionClicked()
{

	FPlatformProcess::LaunchURL(TEXT("https://discord.gg/KGvBBTv"), NULL, NULL);
}

void FRuntimeMeshComponentEditorPlugin::MarketplaceActionClicked()
{
	FPlatformProcess::LaunchURL(TEXT("https://unrealengine.com/Marketplace/RuntimeMeshComponent"), NULL, NULL);

}

void FRuntimeMeshComponentEditorPlugin::AddMenuBarExtension(FMenuBarBuilder& Builder)
{
	Builder.AddPullDownMenu(
		LOCTEXT("RuntimeMeshComponentMenu", "Runtime Mesh Component"),
		LOCTEXT("RuntimeMeshComponentMenu_ToolTip", "Open Runtime Mesh Component Help and Documentation"),
		FNewMenuDelegate::CreateRaw(this, &FRuntimeMeshComponentEditorPlugin::AddMenuExtension),
		"Runtime Mesh Component");
}

void FRuntimeMeshComponentEditorPlugin::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.BeginSection("Help", LOCTEXT("RuntimeMeshComponentMenu_Help", "Help"));
	Builder.AddMenuEntry(FRuntimeMeshComponentEditorCommands::Get().MarketplaceAction);
	Builder.AddMenuEntry(FRuntimeMeshComponentEditorCommands::Get().ForumsAction);
	Builder.AddMenuEntry(FRuntimeMeshComponentEditorCommands::Get().HelpAction);
	Builder.AddMenuEntry(FRuntimeMeshComponentEditorCommands::Get().IssuesAction);
	Builder.AddMenuEntry(FRuntimeMeshComponentEditorCommands::Get().DiscordAction);
	Builder.EndSection();
	
	Builder.BeginSection("Support", LOCTEXT("RuntimeMeshComponentMenu_Support", "Support"));
	Builder.AddMenuEntry(FRuntimeMeshComponentEditorCommands::Get().DonateAction);
	Builder.EndSection();
}


#undef LOCTEXT_NAMESPACE