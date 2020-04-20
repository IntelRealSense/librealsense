// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "RuntimeMeshComponentEditorCommands.h"

#define LOCTEXT_NAMESPACE "FRuntimeMeshComponentEditorModule"

void FRuntimeMeshComponentEditorCommands::RegisterCommands()
{
	UI_COMMAND(DonateAction, "Donate to Support the RMC!", "Support the RMC by donating", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(HelpAction, "Help Documentation", "RMC Help Documentation", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(ForumsAction, "Forums", "RMC Forums", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(IssuesAction, "Issues", "RMC Issue Tracker", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(DiscordAction, "Discord Chat", "RMC Discord Chat", EUserInterfaceActionType::Button, FInputGesture());
	UI_COMMAND(MarketplaceAction, "Marketplace Page", "RMC Marketplace Page", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE
