// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once
#include "IDetailCustomization.h"
#include "DetailLayoutBuilder.h"
#include "RuntimeMesh.h"
#include "SlateEnums.h"

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
	TArray<TWeakObjectPtr<UObject>> SelectedObjectsList;
	TArray<URuntimeMesh*> RuntimeMeshesReferenced;

	TArray<TSharedPtr<ERuntimeMeshCollisionCookingMode>> CookingModes;

	ECheckBoxState UseComplexAsSimple() const;
	void UseComplexAsSimpleCheckedStateChanged(ECheckBoxState InCheckboxState);

	ECheckBoxState IsAsyncCollisionEnabled() const;
	void AsyncCollisionCheckedStateChanged(ECheckBoxState InCheckboxState);

	ECheckBoxState ShouldSerializeMeshData() const;
	void ShouldSerializeMeshDataCheckedStateChanged(ECheckBoxState InCheckboxState);

	FText GetModeText(const TSharedPtr<ERuntimeMeshCollisionCookingMode>& Mode) const;

	FText GetSelectedModeText() const 
	{
		return GetModeText(GetCurrentCollisionCookingMode());
	}

	TSharedRef<SWidget> MakeCollisionModeComboItemWidget(TSharedPtr<ERuntimeMeshCollisionCookingMode> Mode);

	TSharedPtr<ERuntimeMeshCollisionCookingMode> GetCurrentCollisionCookingMode() const;

	void CollisionCookingModeSelectionChanged(TSharedPtr<ERuntimeMeshCollisionCookingMode> NewMode, ESelectInfo::Type SelectionType);

};