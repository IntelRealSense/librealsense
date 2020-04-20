// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.RuntimeMeshComponentEditor

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "RuntimeMeshComponentEditorStyle.h"

class FRuntimeMeshComponentEditorCommands : public TCommands<FRuntimeMeshComponentEditorCommands>
{
public:

	FRuntimeMeshComponentEditorCommands()
		: TCommands<FRuntimeMeshComponentEditorCommands>(TEXT("RuntimeMeshComponentEditor"), NSLOCTEXT("Contexts", "RuntimeMeshComponentEditor", "RuntimeMeshComponentEditor Plugin"), NAME_None, FRuntimeMeshComponentEditorStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr<FUICommandInfo> DonateAction;
	TSharedPtr<FUICommandInfo> HelpAction;
	TSharedPtr<FUICommandInfo> ForumsAction;
	TSharedPtr<FUICommandInfo> IssuesAction;
	TSharedPtr<FUICommandInfo> DiscordAction;
	TSharedPtr<FUICommandInfo> MarketplaceAction;
};
