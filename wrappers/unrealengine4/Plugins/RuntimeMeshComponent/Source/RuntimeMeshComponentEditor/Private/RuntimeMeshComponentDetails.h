// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once
#include "IDetailCustomization.h"
#include "DetailLayoutBuilder.h"

class FRuntimeMeshComponentDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	/** Handle clicking the convert button */
	FReply ClickedOnConvertToStaticMesh();

	/** Is the convert button enabled */
	bool ConvertToStaticMeshEnabled() const;

	/** Util to get the RuntimeMeshComponent we want to convert */
	class URuntimeMeshComponent* GetFirstSelectedRuntimeMeshComp() const;

	/** Cached array of selected objects */
	TArray< TWeakObjectPtr<UObject> > SelectedObjectsList;
};