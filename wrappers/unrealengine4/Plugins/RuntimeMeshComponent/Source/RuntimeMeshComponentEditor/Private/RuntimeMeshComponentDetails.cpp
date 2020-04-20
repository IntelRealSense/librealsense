// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshComponentDetails.h"
#include "RuntimeMeshComponent.h"
#include "DlgPickAssetPath.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "AssetRegistryModule.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "PhysicsEngine/BodySetup.h"
#include "IDetailCustomization.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "RawMesh.h"

#include "Input/SCheckBox.h"
#include "Input/SComboBox.h"

#define LOCTEXT_NAMESPACE "RuntimeMeshComponentDetails"

TSharedRef<IDetailCustomization> FRuntimeMeshComponentDetails::MakeInstance()
{
	return MakeShareable(new FRuntimeMeshComponentDetails);
}

void FRuntimeMeshComponentDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& RuntimeMeshCategory = DetailBuilder.EditCategory("RuntimeMesh");

	const FText ConvertToStaticMeshText = LOCTEXT("ConvertToStaticMesh", "Create StaticMesh");

	CookingModes = TArray<TSharedPtr<ERuntimeMeshCollisionCookingMode>>
	{
		MakeShared<ERuntimeMeshCollisionCookingMode>(ERuntimeMeshCollisionCookingMode::CollisionPerformance),
		MakeShared<ERuntimeMeshCollisionCookingMode>(ERuntimeMeshCollisionCookingMode::CookingPerformance)
	};

	// Cache set of selected things
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 18
	SelectedObjectsList = DetailBuilder.GetDetailsView()->GetSelectedObjects();
#else
	SelectedObjectsList = DetailBuilder.GetDetailsView().GetSelectedObjects();
#endif

	// Add the Create Static Mesh button
	RuntimeMeshCategory.AddCustomRow(ConvertToStaticMeshText, false)
		.NameContent()
		[
			SNullWidget::NullWidget
		]
	.ValueContent()
		.VAlign(VAlign_Center)
		.MaxDesiredWidth(250)
		[
			SNew(SButton)
			.VAlign(VAlign_Center)
		.ToolTipText(LOCTEXT("ConvertToStaticMeshTooltip", "Create a new StaticMesh asset using current geometry from this RuntimeMeshComponent. Does not modify instance."))
		.OnClicked(this, &FRuntimeMeshComponentDetails::ClickedOnConvertToStaticMesh)
		.IsEnabled(this, &FRuntimeMeshComponentDetails::ConvertToStaticMeshEnabled)
		.Content()
		[
			SNew(STextBlock)
			.Text(ConvertToStaticMeshText)
		]
		];

	{
		// Add all the default properties
		TArray<TSharedRef<IPropertyHandle>> AllProperties;
		bool bSimpleProperties = true;
		bool bAdvancedProperties = false;
		// Add all properties in the category in order
		RuntimeMeshCategory.GetDefaultProperties(AllProperties, bSimpleProperties, bAdvancedProperties);
		for (auto& Property : AllProperties)
		{
			RuntimeMeshCategory.AddProperty(Property);
		}
	}

	TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
	DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);

	RuntimeMeshesReferenced.Empty();
	for (TWeakObjectPtr<UObject>& Object : ObjectsBeingCustomized)
	{
		URuntimeMeshComponent* Component = Cast<URuntimeMeshComponent>(Object.Get());
		if (ensure(Component))
		{
			if (Component->GetRuntimeMesh())
			{
				RuntimeMeshesReferenced.AddUnique(Component->GetRuntimeMesh());
			}
		}
	}

	FText UseComplexAsSimpleName = LOCTEXT("RuntimeMeshProperty_UseComplexAsSimpleCollision", "Use Complex as Simple Collision");
	RuntimeMeshCategory.AddCustomRow(UseComplexAsSimpleName, false)
		.NameContent()
		[
			SNew(STextBlock)
			.Text(UseComplexAsSimpleName)
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
	.ValueContent()
		.VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
			.IsChecked(this, &FRuntimeMeshComponentDetails::UseComplexAsSimple)
			.OnCheckStateChanged(this, &FRuntimeMeshComponentDetails::UseComplexAsSimpleCheckedStateChanged)
		];

	FText UseAsyncCollisionName = LOCTEXT("RuntimeMeshProperty_AsyncCollision", "Use Async Cooking");
	RuntimeMeshCategory.AddCustomRow(UseAsyncCollisionName, false)
		.NameContent()
		[
			SNew(STextBlock)
			.Text(UseAsyncCollisionName)
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
	.ValueContent()
		.VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
			.IsChecked(this, &FRuntimeMeshComponentDetails::IsAsyncCollisionEnabled)
			.OnCheckStateChanged(this, &FRuntimeMeshComponentDetails::AsyncCollisionCheckedStateChanged)
		];

	FText ShouldSerializeMeshDataName = LOCTEXT("RuntimeMeshProperty_ShouldSerialize", "Should Serialize Mesh Data");
	RuntimeMeshCategory.AddCustomRow(ShouldSerializeMeshDataName, false)
		.NameContent()
		[
			SNew(STextBlock)
			.Text(ShouldSerializeMeshDataName)
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
	.ValueContent()
		.VAlign(VAlign_Center)
		[
			SNew(SCheckBox)
			.IsChecked(this, &FRuntimeMeshComponentDetails::ShouldSerializeMeshData)
			.OnCheckStateChanged(this, &FRuntimeMeshComponentDetails::ShouldSerializeMeshDataCheckedStateChanged)
		];

	FText CollisionCookingModeName = LOCTEXT("RuntimeMeshProperty_CollisionCookingModeName", "Collision Cooking Mode");
	RuntimeMeshCategory.AddCustomRow(CollisionCookingModeName, false)
		.NameContent()
		[
			SNew(STextBlock)
			.Text(CollisionCookingModeName)
			.Font(IDetailLayoutBuilder::GetDetailFont())
		]
		.ValueContent()
		.VAlign(VAlign_Center)
		[
			SNew(SComboBox<TSharedPtr<ERuntimeMeshCollisionCookingMode>>)
			.OptionsSource(&CookingModes)
			.OnGenerateWidget(this, &FRuntimeMeshComponentDetails::MakeCollisionModeComboItemWidget)
			.OnSelectionChanged(this, &FRuntimeMeshComponentDetails::CollisionCookingModeSelectionChanged)
			.InitiallySelectedItem(GetCurrentCollisionCookingMode())
			[
				SNew(STextBlock).Text(this, &FRuntimeMeshComponentDetails::GetSelectedModeText)
			]
		];

	
}

URuntimeMeshComponent* FRuntimeMeshComponentDetails::GetFirstSelectedRuntimeMeshComp() const
{
	// Find first selected valid RuntimeMeshComp
	URuntimeMeshComponent* RuntimeMeshComp = nullptr;
	for (const TWeakObjectPtr<UObject>& Object : SelectedObjectsList)
	{
		URuntimeMeshComponent* TestRuntimeComp = Cast<URuntimeMeshComponent>(Object.Get());
		// See if this one is good
		if (TestRuntimeComp != nullptr && !TestRuntimeComp->IsTemplate())
		{
			RuntimeMeshComp = TestRuntimeComp;
			break;
		}
	}

	return RuntimeMeshComp;
}


ECheckBoxState FRuntimeMeshComponentDetails::UseComplexAsSimple() const
{
	ECheckBoxState State = ECheckBoxState::Undetermined;

	for (URuntimeMesh* Mesh : RuntimeMeshesReferenced)
	{
		if (Mesh && Mesh->IsValidLowLevel())
		{
			ECheckBoxState NewState = Mesh->IsCollisionUsingComplexAsSimple() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;

			if (State == ECheckBoxState::Undetermined || State == NewState)
			{
				State = NewState;
			}
			else
			{
				State = ECheckBoxState::Undetermined;
				break;
			}
		}
	}

	return State;
}

void FRuntimeMeshComponentDetails::UseComplexAsSimpleCheckedStateChanged(ECheckBoxState InCheckboxState)
{
	for (URuntimeMesh* Mesh : RuntimeMeshesReferenced)
	{
		if (Mesh && Mesh->IsValidLowLevel())
		{
			Mesh->SetCollisionUseComplexAsSimple(InCheckboxState == ECheckBoxState::Checked);
		}
	}
}

ECheckBoxState FRuntimeMeshComponentDetails::IsAsyncCollisionEnabled() const
{
	ECheckBoxState State = ECheckBoxState::Undetermined;

	for (URuntimeMesh* Mesh : RuntimeMeshesReferenced)
	{
		if (Mesh && Mesh->IsValidLowLevel())
		{
			ECheckBoxState NewState = Mesh->IsCollisionUsingAsyncCooking() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;

			if (State == ECheckBoxState::Undetermined || State == NewState)
			{
				State = NewState;
			}
			else
			{
				State = ECheckBoxState::Undetermined;
				break;
			}
		}
	}

	return State;
}

void FRuntimeMeshComponentDetails::AsyncCollisionCheckedStateChanged(ECheckBoxState InCheckboxState)
{
	for (URuntimeMesh* Mesh : RuntimeMeshesReferenced)
	{
		if (Mesh && Mesh->IsValidLowLevel())
		{
			Mesh->SetCollisionUseAsyncCooking(InCheckboxState == ECheckBoxState::Checked);
		}
	}
}

ECheckBoxState FRuntimeMeshComponentDetails::ShouldSerializeMeshData() const
{
	ECheckBoxState State = ECheckBoxState::Undetermined;

	for (URuntimeMesh* Mesh : RuntimeMeshesReferenced)
	{
		if (Mesh && Mesh->IsValidLowLevel())
		{
			ECheckBoxState NewState = Mesh->ShouldSerializeMeshData() ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;

			if (State == ECheckBoxState::Undetermined || State == NewState)
			{
				State = NewState;
			}
			else
			{
				State = ECheckBoxState::Undetermined;
				break;
			}
		}
	}

	return State;
}

void FRuntimeMeshComponentDetails::ShouldSerializeMeshDataCheckedStateChanged(ECheckBoxState InCheckboxState)
{
	for (URuntimeMesh* Mesh : RuntimeMeshesReferenced)
	{
		if (Mesh && Mesh->IsValidLowLevel())
		{
			Mesh->SetShouldSerializeMeshData(InCheckboxState == ECheckBoxState::Checked);
		}
	}
}

FText FRuntimeMeshComponentDetails::GetModeText(const TSharedPtr<ERuntimeMeshCollisionCookingMode>& Mode) const
{
	if (Mode.IsValid())
	{
		switch (*Mode)
		{
		case ERuntimeMeshCollisionCookingMode::CollisionPerformance:
			return LOCTEXT("ERuntimeMeshCollisionCookingMode_CollisionPerformance", "Collision Performance");
		case ERuntimeMeshCollisionCookingMode::CookingPerformance:
			return LOCTEXT("ERuntimeMeshCollisionCookingMode_CookingPerformance", "Cooking Performance");
		}
	}
	return LOCTEXT("ERuntimeMeshCollisionCookingMode_Unknown", "Unknown/Multiple Values");
}

TSharedRef<SWidget> FRuntimeMeshComponentDetails::MakeCollisionModeComboItemWidget(TSharedPtr<ERuntimeMeshCollisionCookingMode> Mode)
{
	return SNew(STextBlock).Text(GetModeText(Mode)).ToolTipText(GetModeText(Mode));
}

TSharedPtr<ERuntimeMeshCollisionCookingMode> FRuntimeMeshComponentDetails::GetCurrentCollisionCookingMode() const
{
	ERuntimeMeshCollisionCookingMode CurrentMode = ERuntimeMeshCollisionCookingMode::CollisionPerformance;
	bool bIsFirst = true;
	bool bIsAllSame = true;

	for (URuntimeMesh* Mesh : RuntimeMeshesReferenced)
	{
		if (Mesh && Mesh->IsValidLowLevel())
		{
			ERuntimeMeshCollisionCookingMode NewMode = Mesh->GetCollisionMode();
			if (bIsFirst || CurrentMode == NewMode)
			{
				CurrentMode = NewMode;
				bIsFirst = false;
			}
			else
			{
				bIsAllSame = false;
				break;
			}
		}
	}

	return bIsAllSame ? *CookingModes.FindByPredicate([CurrentMode](const TSharedPtr<ERuntimeMeshCollisionCookingMode>& Mode) -> bool
	{
		return *Mode == CurrentMode;
	}) : nullptr;
}

void FRuntimeMeshComponentDetails::CollisionCookingModeSelectionChanged(TSharedPtr<ERuntimeMeshCollisionCookingMode> NewMode, ESelectInfo::Type SelectionType)
{
	if (NewMode.IsValid())
	{
		for (URuntimeMesh* Mesh : RuntimeMeshesReferenced)
		{
			if (Mesh && Mesh->IsValidLowLevel())
			{
				Mesh->SetCollisionMode(*NewMode);
			}
		}
	}
}

bool FRuntimeMeshComponentDetails::ConvertToStaticMeshEnabled() const
{
	return GetFirstSelectedRuntimeMeshComp() != nullptr;
}


FReply FRuntimeMeshComponentDetails::ClickedOnConvertToStaticMesh()
{
	// Find first selected RuntimeMeshComp
	URuntimeMeshComponent* RuntimeMeshComp = GetFirstSelectedRuntimeMeshComp();
	if (RuntimeMeshComp != nullptr)
	{
		FString NewNameSuggestion = FString(TEXT("RuntimeMeshComp"));
		FString PackageName = FString(TEXT("/Game/Meshes/")) + NewNameSuggestion;
		FString Name;
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		AssetToolsModule.Get().CreateUniqueAssetName(PackageName, TEXT(""), PackageName, Name);

		TSharedPtr<SDlgPickAssetPath> PickAssetPathWidget =
			SNew(SDlgPickAssetPath)
			.Title(LOCTEXT("ConvertToStaticMeshPickName", "Choose New StaticMesh Location"))
			.DefaultAssetPath(FText::FromString(PackageName));

		if (PickAssetPathWidget->ShowModal() == EAppReturnType::Ok)
		{
			// Get the full name of where we want to create the physics asset.
			FString UserPackageName = PickAssetPathWidget->GetFullAssetPath().ToString();
			FName MeshName(*FPackageName::GetLongPackageAssetName(UserPackageName));

			// Check if the user inputed a valid asset name, if they did not, give it the generated default name
			if (MeshName == NAME_None)
			{
				// Use the defaults that were already generated.
				UserPackageName = PackageName;
				MeshName = *Name;
			}

			// Raw mesh data we are filling in
			FRawMesh RawMesh;

			// Unique Materials to apply to new mesh
			TArray<FStaticMaterial> Materials;

			bool bUseHighPrecisionTangents = false;
			bool bUseFullPrecisionUVs = false;

			const int32 NumSections = RuntimeMeshComp->GetNumSections();
			int32 VertexBase = 0;
			int32 MaxUVs = 0;
			for (int32 SectionIdx = 0; SectionIdx < NumSections; SectionIdx++)
			{
				auto MeshData = RuntimeMeshComp->GetSectionReadonly(SectionIdx);
				check(MeshData.IsValid());

				int32 NumUVs = MeshData->NumUVChannels();
				MaxUVs = FMath::Max(NumUVs, MaxUVs);

				// Fill out existing UV channels to start of this one
				for (int32 Index = 0; Index < MaxUVs; Index++)
				{
					RawMesh.WedgeTexCoords[Index].SetNumZeroed(RawMesh.WedgeIndices.Num());
				}

				// Copy the vertex positions
				int32 NumVertices = MeshData->NumVertices();
				for (int32 Index = 0; Index < NumVertices; Index++)
				{
					RawMesh.VertexPositions.Add(MeshData->GetPosition(Index));
				}

				// Copy wedges
				int32 NumTris = MeshData->NumIndices();
				for (int32 Index = 0; Index < NumTris; Index++)
				{
					int32 VertexIndex = MeshData->GetIndex(Index);
					RawMesh.WedgeIndices.Add(VertexIndex + VertexBase);

					FVector TangentX = MeshData->GetTangent(VertexIndex);
					FVector TangentZ = MeshData->GetNormal(VertexIndex);

					FVector TangentY = (TangentX ^ TangentZ).GetSafeNormal() * MeshData->GetNormal(VertexIndex).W;
					RawMesh.WedgeTangentX.Add(TangentX);
					RawMesh.WedgeTangentY.Add(TangentY);
					RawMesh.WedgeTangentZ.Add(TangentZ);


					for (int32 UVIndex = 0; UVIndex < NumUVs; UVIndex++)
					{
						RawMesh.WedgeTexCoords[UVIndex].Add(MeshData->GetUV(VertexIndex, UVIndex));
					}

					RawMesh.WedgeColors.Add(MeshData->GetColor(VertexIndex));
				}

				// Find a material index for this section.
				UMaterialInterface* Material = RuntimeMeshComp->GetSectionMaterial(SectionIdx);
				int32 MaterialIndex = Materials.AddUnique(FStaticMaterial(Material));

				// copy face info
				for (int32 TriIdx = 0; TriIdx < NumTris / 3; TriIdx++)
				{
					// Set the face material
					RawMesh.FaceMaterialIndices.Add(MaterialIndex);

					RawMesh.FaceSmoothingMasks.Add(0); // Assume this is ignored as bRecomputeNormals is false
				}

				// Update offset for creating one big index/vertex buffer
				VertexBase += NumVertices;
			}

			// Fill out existing UV channels to start of this one
			for (int32 Index = 0; Index < MaxUVs; Index++)
			{
				RawMesh.WedgeTexCoords[Index].SetNumZeroed(RawMesh.WedgeIndices.Num());
			}

			// If we got some valid data.
			if (RawMesh.VertexPositions.Num() >= 3 && RawMesh.WedgeIndices.Num() >= 3)
			{
				// Then find/create it.
				UPackage* Package = CreatePackage(NULL, *UserPackageName);
				check(Package);

				// Create StaticMesh object
				UStaticMesh* StaticMesh = NewObject<UStaticMesh>(Package, MeshName, RF_Public | RF_Standalone);
				StaticMesh->InitResources();

				StaticMesh->LightingGuid = FGuid::NewGuid();

				// Add source to new StaticMesh
				FStaticMeshSourceModel* SrcModel = new (StaticMesh->SourceModels) FStaticMeshSourceModel();
				SrcModel->BuildSettings.bRecomputeNormals = false;
				SrcModel->BuildSettings.bRecomputeTangents = false;
				SrcModel->BuildSettings.bRemoveDegenerates = false;
				SrcModel->BuildSettings.bUseHighPrecisionTangentBasis = bUseHighPrecisionTangents;
				SrcModel->BuildSettings.bUseFullPrecisionUVs = bUseFullPrecisionUVs;
				SrcModel->BuildSettings.bGenerateLightmapUVs = true;
				SrcModel->BuildSettings.SrcLightmapIndex = 0;
				SrcModel->BuildSettings.DstLightmapIndex = 1;
				SrcModel->RawMeshBulkData->SaveRawMesh(RawMesh);

				// Set the materials used for this static mesh
				StaticMesh->StaticMaterials = Materials;
				int32 NumMaterials = StaticMesh->StaticMaterials.Num();

				// Set up the SectionInfoMap to enable collision
				for (int32 SectionIdx = 0; SectionIdx < NumMaterials; SectionIdx++)
				{
					FMeshSectionInfo Info = StaticMesh->SectionInfoMap.Get(0, SectionIdx);
					Info.MaterialIndex = SectionIdx;
					Info.bEnableCollision = true;
					StaticMesh->SectionInfoMap.Set(0, SectionIdx, Info);
				}

				// Configure body setup for working collision.
				StaticMesh->CreateBodySetup();
				StaticMesh->BodySetup->CollisionTraceFlag = CTF_UseComplexAsSimple;

				// Build mesh from source
				StaticMesh->Build(false);

				// Make package dirty.
				StaticMesh->MarkPackageDirty();

				StaticMesh->PostEditChange();

				// Notify asset registry of new asset
				FAssetRegistryModule::AssetCreated(StaticMesh);
			}
		}
	}

	return FReply::Handled();
}


#undef LOCTEXT_NAMESPACE