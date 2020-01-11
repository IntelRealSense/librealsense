// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshData.h"
#include "RuntimeMeshComponentPlugin.h"
#include "RuntimeMesh.h"
#include "RuntimeMeshLibrary.h"
#include "RuntimeMeshCollision.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "RuntimeMeshProxy.h"


DECLARE_CYCLE_STAT(TEXT("RM - Validation - Create"), STAT_RuntimeMesh_CheckCreate, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Validation - Update"), STAT_RuntimeMesh_CheckUpdate, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Validation - BoundingBox"), STAT_RuntimeMesh_CheckBoundingBox, STATGROUP_RuntimeMesh);

DECLARE_CYCLE_STAT(TEXT("RM - Create Mesh Section - MeshBuilder"), STAT_RuntimeMesh_CreateMeshSection_MeshData, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Create Mesh Section - MeshBuilder - Move"), STAT_RuntimeMesh_CreateMeshSection_MeshData_Move, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Update Mesh Section - MeshBuilder"), STAT_RuntimeMesh_UpdateMeshSection_MeshData, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Update Mesh Section - MeshBuilder - Move"), STAT_RuntimeMesh_UpdateMeshSection_MeshData_Move, STATGROUP_RuntimeMesh);

DECLARE_CYCLE_STAT(TEXT("RM - Create Mesh Section - Component Buffers"), STAT_RuntimeMesh_CreateMeshSectionFromComponents, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Update Mesh Section - Component Buffers"), STAT_RuntimeMesh_UpdateMeshSectionFromComponents, STATGROUP_RuntimeMesh);

DECLARE_CYCLE_STAT(TEXT("RM - Create Mesh Section - Blueprint Packed Buffer"), STAT_RuntimeMesh_CreateMeshSectionPacked_Blueprint, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Update Mesh Section - Blueprint Packed Buffer"), STAT_RuntimeMesh_UpdateMeshSectionPacked_Blueprint, STATGROUP_RuntimeMesh);

DECLARE_CYCLE_STAT(TEXT("RM - Get Readonly Section Accessor"), STAT_RuntimeMesh_GetReadonlyMeshAccessor, STATGROUP_RuntimeMesh);

DECLARE_CYCLE_STAT(TEXT("RM - Clear Mesh Section"), STAT_RuntimeMesh_ClearMeshSection, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Clear All Mesh Sections"), STAT_RuntimeMesh_ClearAllMeshSections, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Get Section Bounding Box"), STAT_RuntimeMesh_GetSectionBoundingBox, STATGROUP_RuntimeMesh);

DECLARE_CYCLE_STAT(TEXT("RM - Set Mesh Section Visible"), STAT_RuntimeMesh_SetMeshSectionVisible, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Is Mesh Section Visible"), STAT_RuntimeMesh_IsMeshSectionVisible, STATGROUP_RuntimeMesh);

DECLARE_CYCLE_STAT(TEXT("RM - Set Mesh Section Casts Shadow"), STAT_RuntimeMesh_SetMeshSectionCastsShadow, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Is Mesh Section Casting Shadows"), STAT_RuntimeMesh_IsMeshSectionCastingShadows, STATGROUP_RuntimeMesh);

DECLARE_CYCLE_STAT(TEXT("RM - Set Mesh Section Collision Enabled"), STAT_RuntimeMesh_SetMeshSectionCollisionEnabled, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Is Mesh Section Collision Enabled"), STAT_RuntimeMesh_IsMeshSectionCollisionEnabled, STATGROUP_RuntimeMesh);

DECLARE_CYCLE_STAT(TEXT("RM - Get Available Section Index"), STAT_RuntimeMesh_GetAvailableSectionIndex, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Get Section Ids"), STAT_RuntimeMesh_GetSectionIds, STATGROUP_RuntimeMesh);

DECLARE_CYCLE_STAT(TEXT("RM - Set Mesh Collision Section"), STAT_RuntimeMesh_SetMeshCollisionSection, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Clear Mesh Collision Section"), STAT_RuntimeMesh_ClearMeshCollisionSection, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Clear All Mesh Collision Sections"), STAT_RuntimeMesh_ClearAllMeshCollisionSections, STATGROUP_RuntimeMesh);

DECLARE_CYCLE_STAT(TEXT("RM - Add Convex Collision Section"), STAT_RuntimeMesh_AddConvexCollisionSection, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Set Convex Collision Section"), STAT_RuntimeMesh_SetConvexCollisionSection, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Clear Convex Collision Section"), STAT_RuntimeMesh_ClearConvexCollisionSection, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Clear All Convex Collision Sections"), STAT_RuntimeMesh_ClearAllConvexCollisionSections, STATGROUP_RuntimeMesh);

DECLARE_CYCLE_STAT(TEXT("RM - Create Mesh Section - Internal"), STAT_RuntimeMesh_CreateSectionInternal, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Update Mesh Section - Internal"), STAT_RuntimeMesh_UpdateSectionInternal, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Handle Common Section Update Flags"), STAT_RuntimeMesh_HandleCommonSectionUpdateFlags, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Handle Common Section Update Flags - Calculate Tangents"), STAT_RuntimeMesh_HandleCommonSectionUpdateFlags_CalculateTangents, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Handle Common Section Update Flags - Calculate Tessellation Indices"), STAT_RuntimeMesh_HandleCommonSectionUpdateFlags_CalculateTessellationIndices, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Update Section Properties Internal"), STAT_RuntimeMesh_UpdateSectionPropertiesInternal, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Update Local Bounds"), STAT_RuntimeMesh_UpdateLocalBounds, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Initialize"), STAT_RuntimeMesh_Initialize, STATGROUP_RuntimeMesh);

DECLARE_CYCLE_STAT(TEXT("RM - Contains Physics Triangle Mesh Data"), STAT_RuntimeMesh_ContainsPhysicsTriMeshData, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Get Physics Triangle Mesh Data"), STAT_RuntimeMesh_GetPhysicsTriMeshData, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Copy Collision Elements to Body Setup"), STAT_RuntimeMesh_CopyCollisionElementsToBodySetup, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Get Section From Collision Face Index"), STAT_RuntimeMesh_GetSectionFromCollisionFaceIndex, STATGROUP_RuntimeMesh);

FRuntimeMeshData::FRuntimeMeshData()
	: SyncRoot(new FRuntimeMeshNullLockProvider())
{
}

FRuntimeMeshData::~FRuntimeMeshData()
{
}

void FRuntimeMeshData::Setup(TWeakObjectPtr<URuntimeMesh> InParentMeshObject)
{
	ParentMeshObject = InParentMeshObject;
}

void FRuntimeMeshData::CheckCreate(int32 NumUVs, bool bIndexIsValid) const
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_CheckCreate);
#if DO_CHECK

	if (NumUVs < 1 || NumUVs > 8)
	{
		UE_LOG(RuntimeMeshLog, Fatal, TEXT("UV Channel Count must be between 1 and 8 inclusive"));
	}

	// Check indices
	if (!bIndexIsValid)
	{
		UE_LOG(RuntimeMeshLog, Fatal, TEXT("Indices can only be of type uint16, int32, or uint32."));
	}
#endif
}

void FRuntimeMeshData::CheckCreateLegacyInternal(const FRuntimeMeshVertexStreamStructure& Stream0Structure, const FRuntimeMeshVertexStreamStructure& Stream1Structure, const FRuntimeMeshVertexStreamStructure& Stream2Structure, bool bIsIndexValid) const
{
#if DO_CHECK

	// Check stream 0 contains the position element
	if (!Stream0Structure.Position.IsValid())
	{

		UE_LOG(RuntimeMeshLog, Fatal, TEXT("Position element must always be in stream 0."));
	}

	// Check the 3 streams are valid when combined
	if (!FRuntimeMeshVertexStreamStructure::ValidTripleStream(Stream0Structure, Stream1Structure, Stream2Structure))
	{
		UE_LOG(RuntimeMeshLog, Fatal, TEXT("Streams cannot have overlapping elements, and all elements must be present."));
	}

	// Check indices
	if (!bIsIndexValid)
	{
		UE_LOG(RuntimeMeshLog, Fatal, TEXT("Indices can only be of type uint16, int32, or uint32."));
	}
#endif
}

void FRuntimeMeshData::CheckUpdate(bool bUseHighPrecisionTangents, bool bUseHighPrecisionUVs, int32 NumUVs, bool b32BitIndices, int32 SectionIndex, bool bShouldCheckIndexType,
	bool bCheckTangentVertexStream, bool bCheckUVVertexStream) const
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_CheckUpdate);
#if DO_CHECK
	if (!MeshSections.IsValidIndex(SectionIndex) || !MeshSections[SectionIndex].IsValid())
	{
		UE_LOG(RuntimeMeshLog, Fatal, TEXT("Mesh Section %d does not exist in RMC."), SectionIndex);
	}

	FRuntimeMeshSectionPtr Section = MeshSections[SectionIndex];
	
	if (bCheckTangentVertexStream && !Section->CheckTangentBuffer(bUseHighPrecisionTangents))
	{
		UE_LOG(RuntimeMeshLog, Fatal, TEXT("Supplied vertex type does not match stream 1 for mesh section %d."), SectionIndex);
	}

	if (bCheckUVVertexStream && !Section->CheckUVBuffer(bUseHighPrecisionUVs, NumUVs))
	{
		UE_LOG(RuntimeMeshLog, Fatal, TEXT("Supplied vertex type does not match stream 2 for mesh section %d."), SectionIndex);
	}

	if (bShouldCheckIndexType && !Section->CheckIndexBufferSize(b32BitIndices))
	{
		UE_LOG(RuntimeMeshLog, Fatal, TEXT("Supplied index type do not match mesh section %d."), SectionIndex);
	}
#endif
}

void FRuntimeMeshData::CheckBoundingBox(const FBox& Box) const
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_CheckBoundingBox);
#if DO_CHECK
	if (!Box.IsValid || Box.GetVolume() <= 0)
	{
		UE_LOG(RuntimeMeshLog, Fatal, TEXT("Supplied bounding invalid was invalid."));
	}
#endif
}



void FRuntimeMeshData::EnterSerializedMode()
{
	if (!SyncRoot->IsThreadSafe())
	{
		SyncRoot = MakeUnique<FRuntimeMeshMutexLockProvider>();
	}
}

void FRuntimeMeshData::CreateMeshSection(int32 SectionIndex, bool bWantsHighPrecisionTangents, bool bWantsHighPrecisionUVs, int32 NumUVs, bool bWants32BitIndices, bool bCreateCollision, EUpdateFrequency UpdateFrequency /*= EUpdateFrequency::Average*/)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_CreateMeshSection_NoData);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	CheckCreate(NumUVs, true);
	
	auto NewSection = CreateOrResetSection(SectionIndex, bWantsHighPrecisionTangents, bWantsHighPrecisionUVs, NumUVs, bWants32BitIndices, UpdateFrequency);
	
	// Track collision status and update collision information if necessary
	NewSection->SetCollisionEnabled(bCreateCollision);

	// Finalize section.
	CreateSectionInternal(SectionIndex, ESectionUpdateFlags::None);
}

void FRuntimeMeshData::CreateMeshSection(int32 SectionId, const TSharedPtr<FRuntimeMeshBuilder>& MeshData, bool bCreateCollision /*= false*/, EUpdateFrequency UpdateFrequency /*= EUpdateFrequency::Average*/, ESectionUpdateFlags UpdateFlags /*= ESectionUpdateFlags::None*/)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_CreateMeshSection_MeshData);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	CheckCreate(MeshData->NumUVChannels(), true);


	auto NewSection = CreateOrResetSection(SectionId, MeshData->IsUsingHighPrecisionTangents(), MeshData->IsUsingHighPrecisionUVs(), MeshData->NumUVChannels(), MeshData->IsUsing32BitIndices(), UpdateFrequency);

	NewSection->UpdatePositionBuffer(MeshData->GetPositionStream(), false);
	NewSection->UpdateTangentsBuffer(MeshData->GetTangentStream(), false);
	NewSection->UpdateUVsBuffer(MeshData->GetUVStream(), false);
	NewSection->UpdateColorBuffer(MeshData->GetColorStream(), false);
	NewSection->UpdateIndexBuffer(MeshData->GetIndexStream(), false);

	// Track collision status and update collision information if necessary
	NewSection->SetCollisionEnabled(bCreateCollision);

	// Finalize section.
	CreateSectionInternal(SectionId, UpdateFlags);
}

void FRuntimeMeshData::CreateMeshSectionByMove(int32 SectionId, const TSharedPtr<FRuntimeMeshBuilder>& MeshData, bool bCreateCollision /*= false*/, EUpdateFrequency UpdateFrequency /*= EUpdateFrequency::Average*/, ESectionUpdateFlags UpdateFlags /*= ESectionUpdateFlags::None*/)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_CreateMeshSection_MeshData_Move);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	CheckCreate(MeshData->NumUVChannels(), true);

	auto NewSection = CreateOrResetSection(SectionId, MeshData->IsUsingHighPrecisionTangents(), MeshData->IsUsingHighPrecisionUVs(), MeshData->NumUVChannels(), MeshData->IsUsing32BitIndices(), UpdateFrequency);

	NewSection->UpdatePositionBuffer(MeshData->GetPositionStream(), true);
	NewSection->UpdateTangentsBuffer(MeshData->GetTangentStream(), true);
	NewSection->UpdateUVsBuffer(MeshData->GetUVStream(), true);
	NewSection->UpdateColorBuffer(MeshData->GetColorStream(), true);
	NewSection->UpdateIndexBuffer(MeshData->GetIndexStream(), true);

	// Track collision status and update collision information if necessary
	NewSection->SetCollisionEnabled(bCreateCollision);

	// Finalize section.
	CreateSectionInternal(SectionId, UpdateFlags);
}

void FRuntimeMeshData::UpdateMeshSection(int32 SectionId, const TSharedPtr<FRuntimeMeshBuilder>& MeshData, ESectionUpdateFlags UpdateFlags /*= ESectionUpdateFlags::None*/)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSection_MeshData);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	CheckUpdate(MeshData->IsUsingHighPrecisionTangents(), MeshData->IsUsingHighPrecisionUVs(), MeshData->NumUVChannels(), MeshData->IsUsing32BitIndices(), SectionId, true, true, true);

	FRuntimeMeshSectionPtr Section = MeshSections[SectionId];

	ERuntimeMeshBuffersToUpdate BuffersToUpdate = ERuntimeMeshBuffersToUpdate::AllVertexBuffers | ERuntimeMeshBuffersToUpdate::IndexBuffer;

	Section->UpdatePositionBuffer(MeshData->GetPositionStream(), false);
	Section->UpdateTangentsBuffer(MeshData->GetTangentStream(), false);
	Section->UpdateUVsBuffer(MeshData->GetUVStream(), false);
	Section->UpdateColorBuffer(MeshData->GetColorStream(), false);
	Section->UpdateIndexBuffer(MeshData->GetIndexStream(), false);

	UpdateSectionInternal(SectionId, BuffersToUpdate, UpdateFlags);
}

void FRuntimeMeshData::UpdateMeshSectionByMove(int32 SectionId, const TSharedPtr<FRuntimeMeshBuilder>& MeshData, ESectionUpdateFlags UpdateFlags /*= ESectionUpdateFlags::None*/)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSection_MeshData_Move);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	CheckUpdate(MeshData->IsUsingHighPrecisionTangents(), MeshData->IsUsingHighPrecisionUVs(), MeshData->NumUVChannels(), MeshData->IsUsing32BitIndices(), SectionId, true, true, true);

	FRuntimeMeshSectionPtr Section = MeshSections[SectionId];

	ERuntimeMeshBuffersToUpdate BuffersToUpdate = ERuntimeMeshBuffersToUpdate::AllVertexBuffers | ERuntimeMeshBuffersToUpdate::IndexBuffer;

	Section->UpdatePositionBuffer(MeshData->GetPositionStream(), true);
	Section->UpdateTangentsBuffer(MeshData->GetTangentStream(), true);
	Section->UpdateUVsBuffer(MeshData->GetUVStream(), true);
	Section->UpdateColorBuffer(MeshData->GetColorStream(), true);
	Section->UpdateIndexBuffer(MeshData->GetIndexStream(), true);

	UpdateSectionInternal(SectionId, BuffersToUpdate, UpdateFlags);
}




TUniquePtr<FRuntimeMeshScopedUpdater> FRuntimeMeshData::BeginSectionUpdate(int32 SectionId, ESectionUpdateFlags UpdateFlags /*= ESectionUpdateFlags::None*/)
{
	check(DoesSectionExist(SectionId));

	// Enter the lock and then hand this lock to the updater
	SyncRoot->Lock();
	
	FRuntimeMeshSectionPtr Section = MeshSections[SectionId];

	return Section->GetSectionMeshUpdater(this->AsShared(), SectionId, UpdateFlags, SyncRoot.Get(), false);
}

TUniquePtr<FRuntimeMeshScopedUpdater> FRuntimeMeshData::GetSectionReadonly(int32 SectionId)
{
	check(DoesSectionExist(SectionId));

	// Enter the lock and then hand this lock to the updater
	SyncRoot->Lock();

	FRuntimeMeshSectionPtr Section = MeshSections[SectionId];

	return Section->GetSectionMeshUpdater(this->AsShared(), SectionId, ESectionUpdateFlags::None, SyncRoot.Get(), true);
}

void FRuntimeMeshData::EndSectionUpdate(FRuntimeMeshScopedUpdater* Updater, ERuntimeMeshBuffersToUpdate BuffersToUpdate, const FBox* BoundingBox /*= nullptr*/)
{
	check(DoesSectionExist(Updater->SectionIndex));

	FRuntimeMeshSectionPtr Section = MeshSections[Updater->SectionIndex];

	if (BoundingBox)
	{
		Section->SetBoundingBox(*BoundingBox);
	}
	else
	{
		Section->UpdateBoundingBox();
	}

	UpdateSectionInternal(Updater->SectionIndex, BuffersToUpdate, Updater->UpdateFlags);
}

void FRuntimeMeshData::CreateMeshSectionFromComponents(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, TFunction<FColor(int32 Index)> ColorAccessor, int32 NumColors,
	const TArray<FRuntimeMeshTangent>& Tangents, bool bCreateCollision, EUpdateFrequency UpdateFrequency, ESectionUpdateFlags UpdateFlags,
	bool bUseHighPrecisionTangents, bool bUseHighPrecisionUVs, bool bWantsSecondUV)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_CreateMeshSectionFromComponents);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	// Create the section
	auto NewSection = CreateOrResetSectionForBlueprint(SectionIndex, bWantsSecondUV, bUseHighPrecisionTangents, bUseHighPrecisionUVs, UpdateFrequency);

	TSharedPtr<FRuntimeMeshAccessor> MeshData = NewSection->GetSectionMeshAccessor();

	// We base the size of the mesh data off the vertices/positions
	MeshData->SetNumVertices(Vertices.Num());

	for (int32 Index = 0; Index < Vertices.Num(); Index++)
	{
		MeshData->SetPosition(Index, Vertices[Index]);
		MeshData->SetNormalTangent(Index, 
			Normals.Num() > Index ? Normals[Index] : FVector(0.0f, 0.0f, 1.0f), 
			Tangents.Num() > Index ? Tangents[Index] : FRuntimeMeshTangent(0, 0, 1.0f));
		MeshData->SetColor(Index, NumColors > Index ? ColorAccessor(Index) : FColor::White);
		MeshData->SetUV(Index, 0, UV0.Num() > Index ? UV0[Index] : FVector2D::ZeroVector);
		if (bWantsSecondUV)
		{
			MeshData->SetUV(Index, 1, UV1.Num() > Index ? UV1[Index] : FVector2D::ZeroVector);
		}
	}

	NewSection->UpdateIndexBuffer(Triangles);

	NewSection->UpdateBoundingBox();

	// Track collision status and update collision information if necessary
	NewSection->SetCollisionEnabled(bCreateCollision);

	// Finalize section.
	CreateSectionInternal(SectionIndex, UpdateFlags);
}

void FRuntimeMeshData::UpdateMeshSectionFromComponents(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, TFunction<FColor(int32 Index)> ColorAccessor, int32 NumColors, const TArray<FRuntimeMeshTangent>& Tangents, ESectionUpdateFlags UpdateFlags)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSectionFromComponents);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	// We only check stream 0 and 2 since stream 1 can change based on config, this is potentially dangerous to assume but probably not in practice.
//	CheckUpdate(GetStreamStructure<FVector>(), GetStreamStructure<FRuntimeMeshNullVertex>(), GetStreamStructure<FColor>(), true, SectionIndex, true, true, false, true);

	FRuntimeMeshSectionPtr& Section = MeshSections[SectionIndex];

	ERuntimeMeshBuffersToUpdate BuffersToUpdate = ERuntimeMeshBuffersToUpdate::None;
	if (Vertices.Num() > 0)
	{
		BuffersToUpdate |= ERuntimeMeshBuffersToUpdate::PositionBuffer;
	}
	if (Normals.Num() > 0 || Tangents.Num() > 0)
	{
		BuffersToUpdate |= ERuntimeMeshBuffersToUpdate::TangentBuffer;
	}
	if (UV0.Num() > 0 || UV1.Num() > 0)
	{
		BuffersToUpdate |= ERuntimeMeshBuffersToUpdate::UVBuffer;
	}
	if (NumColors > 0)
	{
		BuffersToUpdate |= ERuntimeMeshBuffersToUpdate::ColorBuffer;
	}

	if (BuffersToUpdate != ERuntimeMeshBuffersToUpdate::None)
	{
		TSharedPtr<FRuntimeMeshAccessor> MeshData = Section->GetSectionMeshAccessor();

		int32 OldVertexCount = FMath::Min(MeshData->NumVertices(), Vertices.Num());

		// We base the size of the mesh data off the vertices/positions
		MeshData->SetNumVertices(Vertices.Num());

		bool bHasSecondUV = MeshData->NumUVChannels() > 1;

		// Overwrite existing data
		for (int32 Index = 0; Index < OldVertexCount; Index++)
		{
			if (Vertices.Num() > Index) MeshData->SetPosition(Index, Vertices[Index]);
			if (Normals.Num() > Index) MeshData->SetNormal(Index, Normals[Index]);
			if (Tangents.Num() > Index) MeshData->SetTangent(Index, FVector4(Tangents[Index].TangentX, Tangents[Index].bFlipTangentY ? -1.0f : 1.0f));
			if (NumColors > Index) MeshData->SetColor(Index, ColorAccessor(Index));
			if (UV0.Num() > Index) MeshData->SetUV(Index, 0, UV0[Index]);
			if (bHasSecondUV && UV1.Num() > Index) MeshData->SetUV(Index, 1, UV1[Index]);
		}

		// Fill remaining mesh data
		for (int32 Index = OldVertexCount; Index < Vertices.Num(); Index++)
		{
			MeshData->SetPosition(Index, Vertices[Index]);
			MeshData->SetNormalTangent(Index,
				Normals.Num() > Index ? Normals[Index] : FVector(0.0f, 0.0f, 1.0f),
				Tangents.Num() > Index ? Tangents[Index] : FRuntimeMeshTangent(0, 0, 1.0f));
			MeshData->SetColor(Index, NumColors > Index ? ColorAccessor(Index) : FColor::White);
			MeshData->SetUV(Index, 0, UV0.Num() > Index ? UV0[Index] : FVector2D::ZeroVector);
			if (bHasSecondUV)
				MeshData->SetUV(Index, 1, UV1.Num() > Index ? UV1[Index] : FVector2D::ZeroVector);
		}
	}

	if (Vertices.Num() > 0)
	{
		Section->UpdateBoundingBox();
	}

	if (Triangles.Num() > 0)
	{
		Section->UpdateIndexBuffer(Triangles);
		BuffersToUpdate |= ERuntimeMeshBuffersToUpdate::IndexBuffer;
	}

	// Finalize section.
	UpdateSectionInternal(SectionIndex, BuffersToUpdate, UpdateFlags);
}

void FRuntimeMeshData::UpdateMeshSectionColors(int32 SectionIndex, TArray<FColor>& Colors, ESectionUpdateFlags UpdateFlags /*= ESectionUpdateFlags::None*/)
{
	if (Colors.Num() == 0)
		return;

	check(DoesSectionExist(SectionIndex));

	FRuntimeMeshScopeLock Lock(SyncRoot);
	const FRuntimeMeshSectionPtr& Section = MeshSections[SectionIndex];
	const ERuntimeMeshBuffersToUpdate BuffersToUpdate = ERuntimeMeshBuffersToUpdate::ColorBuffer;
	TSharedPtr<FRuntimeMeshAccessor> MeshData = Section->GetSectionMeshAccessor();

	for (int i = 0; i < Colors.Num(); i++)
		MeshData->SetColor(i, Colors[i]);

	if (RenderProxy.IsValid())
		RenderProxy->UpdateSection_GameThread(SectionIndex, Section->GetSectionUpdateData(BuffersToUpdate));

	const bool bRequireProxyRecreate = Section->GetUpdateFrequency() == EUpdateFrequency::Infrequent;
	if (bRequireProxyRecreate)
		MarkRenderStateDirty();

	MarkChanged();
}

void FRuntimeMeshData::CreateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents, bool bCreateCollision, EUpdateFrequency UpdateFrequency,
	ESectionUpdateFlags UpdateFlags, bool bUseHighPrecisionTangents, bool bUseHighPrecisionUVs)
{
	CreateMeshSectionFromComponents(SectionIndex, Vertices, Triangles, Normals, UV0, TArray<FVector2D>(), [&Colors](int32 Index) -> FColor { return Colors[Index]; },
		Colors.Num(), Tangents, bCreateCollision, UpdateFrequency, UpdateFlags, bUseHighPrecisionTangents, bUseHighPrecisionUVs, false);
}

void FRuntimeMeshData::CreateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents,
	bool bCreateCollision, EUpdateFrequency UpdateFrequency, ESectionUpdateFlags UpdateFlags, bool bUseHighPrecisionTangents, bool bUseHighPrecisionUVs)
{
	CreateMeshSectionFromComponents(SectionIndex, Vertices, Triangles, Normals, UV0, UV1, [&Colors](int32 Index) -> FColor { return Colors[Index]; },
		Colors.Num(), Tangents, bCreateCollision, UpdateFrequency, UpdateFlags, bUseHighPrecisionTangents, bUseHighPrecisionUVs, true);
}

void FRuntimeMeshData::UpdateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<FVector>& Normals, const TArray<FVector2D>& UV0,
	const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents, ESectionUpdateFlags UpdateFlags)
{
	UpdateMeshSectionFromComponents(SectionIndex, Vertices, TArray<int32>(), Normals, UV0, TArray<FVector2D>(),
		[&Colors](int32 Index) -> FColor { return Colors[Index]; }, Colors.Num(), Tangents, UpdateFlags);
}

void FRuntimeMeshData::UpdateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<FVector>& Normals, const TArray<FVector2D>& UV0,
	const TArray<FVector2D>& UV1, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents, ESectionUpdateFlags UpdateFlags)
{
	UpdateMeshSectionFromComponents(SectionIndex, Vertices, TArray<int32>(), Normals, UV0, UV1,
		[&Colors](int32 Index) -> FColor { return Colors[Index]; }, Colors.Num(), Tangents, UpdateFlags);
}

void FRuntimeMeshData::UpdateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents, ESectionUpdateFlags UpdateFlags)
{
	UpdateMeshSectionFromComponents(SectionIndex, Vertices, Triangles, Normals, UV0, TArray<FVector2D>(),
		[&Colors](int32 Index) -> FColor { return Colors[Index]; }, Colors.Num(), Tangents, UpdateFlags);
}

void FRuntimeMeshData::UpdateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents, ESectionUpdateFlags UpdateFlags)
{
	UpdateMeshSectionFromComponents(SectionIndex, Vertices, Triangles, Normals, UV0, UV1,
		[&Colors](int32 Index) -> FColor { return Colors[Index]; }, Colors.Num(), Tangents, UpdateFlags);
}

void FRuntimeMeshData::CreateMeshSection_Blueprint(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	const TArray<FRuntimeMeshTangent>& Tangents, const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FLinearColor>& VertexColors, bool bCreateCollision,
	bool bCalculateNormalTangent, bool bShouldCreateHardTangents, bool bGenerateTessellationTriangles, EUpdateFrequency UpdateFrequency, bool bUseHighPrecisionTangents, bool bUseHighPrecisionUVs)
{
	ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None;
	UpdateFlags |= bCalculateNormalTangent && !bShouldCreateHardTangents ? ESectionUpdateFlags::CalculateNormalTangent : ESectionUpdateFlags::None;
	UpdateFlags |= bShouldCreateHardTangents ? ESectionUpdateFlags::CalculateNormalTangentHard : ESectionUpdateFlags::None;
	UpdateFlags |= bGenerateTessellationTriangles ? ESectionUpdateFlags::CalculateTessellationIndices : ESectionUpdateFlags::None;

	CreateMeshSectionFromComponents(SectionIndex, Vertices, Triangles, Normals, UV0, UV1, [&VertexColors](int32 Index) -> FColor { return VertexColors[Index].ToFColor(false); },
		VertexColors.Num(), Tangents, bCreateCollision, UpdateFrequency, UpdateFlags, bUseHighPrecisionTangents, bUseHighPrecisionUVs, UV1.Num() > 0);
}

void FRuntimeMeshData::UpdateMeshSection_Blueprint(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals, const TArray<FRuntimeMeshTangent>& Tangents,
	const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FLinearColor>& VertexColors, bool bCalculateNormalTangent, bool bShouldCreateHardTangents, bool bGenerateTessellationTriangles)
{
	ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None;
	UpdateFlags |= bCalculateNormalTangent && !bShouldCreateHardTangents ? ESectionUpdateFlags::CalculateNormalTangent : ESectionUpdateFlags::None;
	UpdateFlags |= bShouldCreateHardTangents ? ESectionUpdateFlags::CalculateNormalTangentHard : ESectionUpdateFlags::None;
	UpdateFlags |= bGenerateTessellationTriangles ? ESectionUpdateFlags::CalculateTessellationIndices : ESectionUpdateFlags::None;

	UpdateMeshSectionFromComponents(SectionIndex, Vertices, TArray<int32>(), Normals, UV0, UV1,
		[&VertexColors](int32 Index) -> FColor { return VertexColors[Index].ToFColor(false); }, VertexColors.Num(), Tangents, UpdateFlags);
}



void FRuntimeMeshData::CreateMeshSectionPacked_Blueprint(int32 SectionIndex, const TArray<FRuntimeMeshBlueprintVertexSimple>& Vertices, const TArray<int32>& Triangles,
	bool bCreateCollision, bool bCalculateNormalTangent, bool bShouldCreateHardTangents, bool bGenerateTessellationTriangles, EUpdateFrequency UpdateFrequency, bool bUseHighPrecisionTangents, bool bUseHighPrecisionUVs)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_CreateMeshSectionPacked_Blueprint);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	// Create the section
	auto NewSection = CreateOrResetSectionForBlueprint(SectionIndex, false, bUseHighPrecisionTangents, bUseHighPrecisionUVs, UpdateFrequency);

	TSharedPtr<FRuntimeMeshAccessor> MeshData = NewSection->GetSectionMeshAccessor();

	// We base the size of the mesh data off the vertices/positions
	MeshData->SetNumVertices(Vertices.Num());

	for (int32 Index = 0; Index < Vertices.Num(); Index++)
	{
		MeshData->SetPosition(Index, Vertices[Index].Position);
		MeshData->SetNormal(Index, Vertices[Index].Normal);
		MeshData->SetTangent(Index, FVector4(Vertices[Index].Tangent.TangentX, Vertices[Index].Tangent.bFlipTangentY ? -1.0f : 1.0f));
		MeshData->SetColor(Index, Vertices[Index].Color.ToFColor(false));
		MeshData->SetUV(Index, 0, Vertices[Index].UV0);
	}

	NewSection->UpdateIndexBuffer(Triangles);

	NewSection->UpdateBoundingBox();

	// Track collision status and update collision information if necessary
	NewSection->SetCollisionEnabled(bCreateCollision);

	ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None;
	UpdateFlags |= bCalculateNormalTangent && !bShouldCreateHardTangents ? ESectionUpdateFlags::CalculateNormalTangent : ESectionUpdateFlags::None;
	UpdateFlags |= bShouldCreateHardTangents ? ESectionUpdateFlags::CalculateNormalTangentHard : ESectionUpdateFlags::None;
	UpdateFlags |= bGenerateTessellationTriangles ? ESectionUpdateFlags::CalculateTessellationIndices : ESectionUpdateFlags::None;

	// Finalize section.
	CreateSectionInternal(SectionIndex, UpdateFlags);
}

void FRuntimeMeshData::UpdateMeshSectionPacked_Blueprint(int32 SectionIndex, const TArray<FRuntimeMeshBlueprintVertexSimple>& Vertices, const TArray<int32>& Triangles,
	bool bCalculateNormalTangent, bool bShouldCreateHardTangents, bool bGenerateTessellationTriangles)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSectionPacked_Blueprint);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	// We only check stream 0 and 2 since stream 1 can change based on config, this is potentially dangerous to assume but probably not in practice.
//	CheckUpdate(GetStreamStructure<FVector>(), GetStreamStructure<FRuntimeMeshNullVertex>(), GetStreamStructure<FColor>(), true, SectionIndex, true, true, false, true);

	FRuntimeMeshSectionPtr& Section = MeshSections[SectionIndex];

	ERuntimeMeshBuffersToUpdate BuffersToUpdate = ERuntimeMeshBuffersToUpdate::None;
	if (Vertices.Num() > 0)
	{
		BuffersToUpdate |= ERuntimeMeshBuffersToUpdate::AllVertexBuffers;

		TSharedPtr<FRuntimeMeshAccessor> MeshData = Section->GetSectionMeshAccessor();

		// We base the size of the mesh data off the vertices/positions
		MeshData->SetNumVertices(Vertices.Num());

		// Copy mesh data
		for (int32 Index = 0; Index < Vertices.Num(); Index++)
		{
			MeshData->SetPosition(Index, Vertices[Index].Position);
			MeshData->SetNormal(Index, Vertices[Index].Normal);
			MeshData->SetTangent(Index, FVector4(Vertices[Index].Tangent.TangentX, Vertices[Index].Tangent.bFlipTangentY ? -1.0f : 1.0f));
			MeshData->SetColor(Index, Vertices[Index].Color.ToFColor(false));
			MeshData->SetUV(Index, 0, Vertices[Index].UV0);
		}

		Section->UpdateBoundingBox();
	}


	if (Triangles.Num() > 0)
	{
		Section->UpdateIndexBuffer(Triangles);
		BuffersToUpdate |= ERuntimeMeshBuffersToUpdate::IndexBuffer;
	}

	ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None;
	UpdateFlags |= bCalculateNormalTangent && !bShouldCreateHardTangents ? ESectionUpdateFlags::CalculateNormalTangent : ESectionUpdateFlags::None;
	UpdateFlags |= bShouldCreateHardTangents ? ESectionUpdateFlags::CalculateNormalTangentHard : ESectionUpdateFlags::None;
	UpdateFlags |= bGenerateTessellationTriangles ? ESectionUpdateFlags::CalculateTessellationIndices : ESectionUpdateFlags::None;

	// Finalize section.
	UpdateSectionInternal(SectionIndex, BuffersToUpdate, UpdateFlags);
}



TSharedPtr<const FRuntimeMeshAccessor> FRuntimeMeshData::GetReadonlyMeshAccessor(int32 SectionId)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_GetReadonlyMeshAccessor);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	check(DoesSectionExist(SectionId));
	FRuntimeMeshSectionPtr Section = MeshSections[SectionId];

	return ConstCastSharedPtr<const FRuntimeMeshAccessor, FRuntimeMeshAccessor>(Section->GetSectionMeshAccessor());
}

void FRuntimeMeshData::ClearMeshSection(int32 SectionId)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_ClearMeshSection);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	if (DoesSectionExist(SectionId))
	{
		FRuntimeMeshSectionPtr Section = MeshSections[SectionId];

		bool bHadCollision = Section->IsCollisionEnabled();
		bool bWasStaticSection = Section->GetUpdateFrequency() == EUpdateFrequency::Infrequent;

		MeshSections[SectionId].Reset();

		if (RenderProxy.IsValid())
		{
			RenderProxy->DeleteSection_GameThread(SectionId);
		}

		// Strip tailing invalid sections
		int32 LastValidIndex = GetLastSectionIndex();
		MeshSections.SetNum(LastValidIndex + 1);

		UpdateLocalBounds();
		MarkRenderStateDirty();

		if (bHadCollision)
		{
			MarkCollisionDirty();
		}
	}
}

void FRuntimeMeshData::ClearAllMeshSections()
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_ClearAllMeshSections);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	MeshSections.Empty();

	if (RenderProxy.IsValid())
	{
		RenderProxy->DeleteSection_GameThread(INDEX_NONE);
	}

	UpdateLocalBounds();
	MarkRenderStateDirty();
	MarkCollisionDirty();
}

FBox FRuntimeMeshData::GetSectionBoundingBox(int32 SectionIndex)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_GetSectionBoundingBox);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	if (DoesSectionExist(SectionIndex))
	{
		return MeshSections[SectionIndex]->GetBoundingBox();
	}

	return FBox();
}

void FRuntimeMeshData::SetMeshSectionVisible(int32 SectionIndex, bool bNewVisibility)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_SetMeshSectionVisible);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	if (DoesSectionExist(SectionIndex))
	{
		MeshSections[SectionIndex]->SetVisible(bNewVisibility);

		// Finish the update
		UpdateSectionPropertiesInternal(SectionIndex, false);
	}
}

bool FRuntimeMeshData::IsMeshSectionVisible(int32 SectionIndex) const
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_IsMeshSectionVisible);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	if (DoesSectionExist(SectionIndex))
	{
		return MeshSections[SectionIndex]->IsVisible();
	}

	return false;
}


void FRuntimeMeshData::SetMeshSectionCastsShadow(int32 SectionIndex, bool bNewCastsShadow)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_SetMeshSectionCastsShadow);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	if (DoesSectionExist(SectionIndex))
	{
		MeshSections[SectionIndex]->SetCastsShadow(bNewCastsShadow);

		// Finish the update
		UpdateSectionPropertiesInternal(SectionIndex, true);
	}
}

bool FRuntimeMeshData::IsMeshSectionCastingShadows(int32 SectionIndex) const
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_IsMeshSectionCastingShadows);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	if (DoesSectionExist(SectionIndex))
	{
		return MeshSections[SectionIndex]->CastsShadow();
	}

	return false;
}


void FRuntimeMeshData::SetMeshSectionCollisionEnabled(int32 SectionIndex, bool bNewCollisionEnabled)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_SetMeshSectionCollisionEnabled);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	if (DoesSectionExist(SectionIndex))
	{
		bool bWasCollisionEnabled = MeshSections[SectionIndex]->IsCollisionEnabled();

		if (bWasCollisionEnabled != bNewCollisionEnabled)
		{
			MeshSections[SectionIndex]->SetCollisionEnabled(bNewCollisionEnabled);

			MarkCollisionDirty();
		}
	}
}

bool FRuntimeMeshData::IsMeshSectionCollisionEnabled(int32 SectionIndex)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_IsMeshSectionCollisionEnabled);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	if (DoesSectionExist(SectionIndex))
	{
		return MeshSections[SectionIndex]->IsCollisionEnabled();
	}

	return false;
}


int32 FRuntimeMeshData::GetNumSections() const
{
	FRuntimeMeshScopeLock Lock(SyncRoot);

	return MeshSections.Num();
}

bool FRuntimeMeshData::DoesSectionExist(int32 SectionIndex) const
{
	FRuntimeMeshScopeLock Lock(SyncRoot);

	return MeshSections.IsValidIndex(SectionIndex) && MeshSections[SectionIndex].IsValid();
}

int32 FRuntimeMeshData::GetAvailableSectionIndex() const
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_GetAvailableSectionIndex);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	// TODO: Make this faster. We could do this by tracking a minimum index to search from.
	for (int32 Index = 0; Index < 10000; Index++)
	{
		if (!DoesSectionExist(Index))
		{
			return Index;
		}
	}
	return INDEX_NONE;
}


int32 FRuntimeMeshData::GetLastSectionIndex() const
{
	for (int32 Index = MeshSections.Num() - 1; Index >= 0; Index--)
	{
		if (MeshSections[Index].IsValid())
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

TArray<int32> FRuntimeMeshData::GetSectionIds() const
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_GetSectionIds);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	TArray<int32> Sections;
	for (int32 SectionId = 0; SectionId < MeshSections.Num(); SectionId++)
	{
		if (MeshSections[SectionId].IsValid())
		{
			Sections.Add(SectionId);
		}
	}
	return Sections;
}



void FRuntimeMeshData::SetMeshCollisionSection(int32 CollisionSectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_SetMeshCollisionSection);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	auto& Section = MeshCollisionSections.FindOrAdd(CollisionSectionIndex);

	Section.VertexBuffer = Vertices;
	Section.IndexBuffer = Triangles;

	MarkCollisionDirty();
}

void FRuntimeMeshData::ClearMeshCollisionSection(int32 CollisionSectionIndex)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_ClearMeshCollisionSection);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	MeshCollisionSections.Remove(CollisionSectionIndex);

	MarkCollisionDirty();
}

void FRuntimeMeshData::ClearAllMeshCollisionSections()
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_ClearAllMeshCollisionSections);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	MeshCollisionSections.Empty();

	MarkCollisionDirty();
}


int32 FRuntimeMeshData::AddConvexCollisionSection(TArray<FVector> ConvexVerts)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_AddConvexCollisionSection);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	int32 NewIndex = 0;
	while (ConvexCollisionSections.Contains(NewIndex))
	{
		NewIndex++;
	}

	auto& Section = ConvexCollisionSections.Add(NewIndex);

	Section.VertexBuffer = ConvexVerts;
	Section.BoundingBox = FBox(ConvexVerts);

	MarkCollisionDirty();

	return NewIndex;
}

void FRuntimeMeshData::SetConvexCollisionSection(int32 ConvexSectionIndex, TArray<FVector> ConvexVerts)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_SetConvexCollisionSection);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	auto& Section = ConvexCollisionSections.FindOrAdd(ConvexSectionIndex);

	Section.VertexBuffer = ConvexVerts;
	Section.BoundingBox = FBox(ConvexVerts);

	MarkCollisionDirty();
}

void FRuntimeMeshData::RemoveConvexCollisionSection(int32 ConvexSectionIndex)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_ClearConvexCollisionSection);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	// Empty simple collision info
	ConvexCollisionSections.Remove(ConvexSectionIndex);

	MarkCollisionDirty();
}

void FRuntimeMeshData::ClearConvexCollisionSections()
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_ClearAllConvexCollisionSections);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	ConvexCollisionSections.Empty();

	MarkCollisionDirty();
}

void FRuntimeMeshData::SetCollisionConvexMeshes(const TArray<TArray<FVector>>& ConvexMeshes)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_ClearAllConvexCollisionSections);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	ConvexCollisionSections.Empty();

	int32 Index = 0;
	for (const auto& Convex : ConvexMeshes)
	{
		auto& Section = ConvexCollisionSections.FindOrAdd(Index++);

		Section.VertexBuffer = Convex;
		Section.BoundingBox = FBox(Convex);
	}

	MarkCollisionDirty();
}



int32 FRuntimeMeshData::AddCollisionBox(const FRuntimeMeshCollisionBox& NewBox)
{
	FRuntimeMeshScopeLock Lock(SyncRoot);
	int32 NewIndex = CollisionBoxes.Add(NewBox);
	MarkCollisionDirty();
	return NewIndex;
}

void FRuntimeMeshData::RemoveCollisionBox(int32 Index)
{
	FRuntimeMeshScopeLock Lock(SyncRoot);
	CollisionBoxes.RemoveAt(Index);
	MarkCollisionDirty();
}

void FRuntimeMeshData::ClearCollisionBoxes()
{
	FRuntimeMeshScopeLock Lock(SyncRoot);
	CollisionBoxes.Empty();
	MarkCollisionDirty();
}

void FRuntimeMeshData::SetCollisionBoxes(const TArray<FRuntimeMeshCollisionBox>& NewBoxes)
{
	FRuntimeMeshScopeLock Lock(SyncRoot);
	CollisionBoxes = NewBoxes;
	MarkCollisionDirty();
}

int32 FRuntimeMeshData::AddCollisionSphere(const FRuntimeMeshCollisionSphere& NewSphere)
{
	FRuntimeMeshScopeLock Lock(SyncRoot);
	int32 NewIndex = CollisionSpheres.Add(NewSphere);
	MarkCollisionDirty();
	return NewIndex;
}

void FRuntimeMeshData::RemoveCollisionSphere(int32 Index)
{
	FRuntimeMeshScopeLock Lock(SyncRoot);
	CollisionSpheres.RemoveAt(Index);
	MarkCollisionDirty();
}

void FRuntimeMeshData::ClearCollisionSpheres()
{
	FRuntimeMeshScopeLock Lock(SyncRoot);
	CollisionSpheres.Empty();
	MarkCollisionDirty();
}

void FRuntimeMeshData::SetCollisionSpheres(const TArray<FRuntimeMeshCollisionSphere>& NewSpheres)
{
	FRuntimeMeshScopeLock Lock(SyncRoot);
	CollisionSpheres = NewSpheres;
	MarkCollisionDirty();
}

int32 FRuntimeMeshData::AddCollisionCapsule(const FRuntimeMeshCollisionCapsule& NewCapsule)
{
	FRuntimeMeshScopeLock Lock(SyncRoot);
	int32 NewIndex = CollisionCapsules.Add(NewCapsule);
	MarkCollisionDirty();
	return NewIndex;
}

void FRuntimeMeshData::RemoveCollisionCapsule(int32 Index)
{
	FRuntimeMeshScopeLock Lock(SyncRoot);
	CollisionCapsules.RemoveAt(Index);
	MarkCollisionDirty();
}

void FRuntimeMeshData::ClearCollisionCapsules()
{
	FRuntimeMeshScopeLock Lock(SyncRoot);
	CollisionCapsules.Empty();
	MarkCollisionDirty();
}

void FRuntimeMeshData::SetCollisionCapsules(const TArray<FRuntimeMeshCollisionCapsule>& NewCapsules)
{
	FRuntimeMeshScopeLock Lock(SyncRoot);

	CollisionCapsules = NewCapsules;
	MarkCollisionDirty();
}

FBoxSphereBounds FRuntimeMeshData::GetLocalBounds() const
{
	FRuntimeMeshScopeLock Lock(SyncRoot, false, true);
	return LocalBounds;
}

FRuntimeMeshSectionPtr FRuntimeMeshData::CreateOrResetSection(int32 SectionId, bool bInUseHighPrecisionTangents, bool bInUseHighPrecisionUVs,
	int32 InNumUVs, bool b32BitIndices, EUpdateFrequency UpdateFrequency)
{
	// Create new section
	FRuntimeMeshSectionPtr NewSection = MakeShared<FRuntimeMeshSection, ESPMode::ThreadSafe>(bInUseHighPrecisionTangents, bInUseHighPrecisionUVs, InNumUVs, b32BitIndices, UpdateFrequency/*, LockFactory()*/);

	// Store section at index
	if (MeshSections.Num() <= SectionId)
	{
		MeshSections.SetNum(SectionId + 1);
	}
	MeshSections[SectionId] = NewSection;

	return NewSection;
}

FRuntimeMeshSectionPtr FRuntimeMeshData::CreateOrResetSectionForBlueprint(int32 SectionId, bool bWantsSecondUV, bool bHighPrecisionTangents, bool bHighPrecisionUVs, EUpdateFrequency UpdateFrequency)
{
	return CreateOrResetSection(SectionId, bHighPrecisionTangents, bHighPrecisionUVs, bWantsSecondUV? 2 : 1, true, UpdateFrequency);
}




void FRuntimeMeshData::CreateSectionInternal(int32 SectionId, ESectionUpdateFlags UpdateFlags)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_CreateSectionInternal);

	check(DoesSectionExist(SectionId));
	FRuntimeMeshSectionPtr Section = MeshSections[SectionId];

	// Send section creation to render thread
	if (RenderProxy.IsValid())
	{
		RenderProxy->CreateSection_GameThread(SectionId, Section->GetSectionCreationParams());
	}

	// Update the combined local bounds		
	UpdateLocalBounds();

	// Do any additional processing on the section for this update.
	ERuntimeMeshBuffersToUpdate BuffersToUpdate; // This is ignored for creation as all buffers are updated.
	HandleCommonSectionUpdateFlags(SectionId, UpdateFlags, BuffersToUpdate);


	// Send the section creation notification to all linked RMC's
	DoOnGameThread(FRuntimeMeshGameThreadTaskDelegate::CreateLambda(
		[SectionId](URuntimeMesh* Mesh)
	{
		Mesh->SendSectionCreation(SectionId);
	}));

	// Update collision if necessary
	if (Section->IsCollisionEnabled())
	{
		MarkCollisionDirty(true);
	}

	MarkChanged();
}

void FRuntimeMeshData::UpdateSectionInternal(int32 SectionId, ERuntimeMeshBuffersToUpdate BuffersToUpdate, ESectionUpdateFlags UpdateFlags)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateSectionInternal);

	check(DoesSectionExist(SectionId));
	FRuntimeMeshSectionPtr Section = MeshSections[SectionId];

	// Send section update to render thread
	if (RenderProxy.IsValid())
	{
		RenderProxy->UpdateSection_GameThread(SectionId, Section->GetSectionUpdateData(BuffersToUpdate));
	}

	// Update the combined local bounds		
	UpdateLocalBounds();

	// Do any additional processing on the section for this update.
	HandleCommonSectionUpdateFlags(SectionId, UpdateFlags, BuffersToUpdate);

	bool bRequireProxyRecreate = Section->GetUpdateFrequency() == EUpdateFrequency::Infrequent;
	if (bRequireProxyRecreate)
	{
		// Send the section creation notification to all linked RMC's

		MarkRenderStateDirty();
	}

	// Update collision if necessary
	if (Section->IsCollisionEnabled())
	{
		MarkCollisionDirty(true);
	}

	MarkChanged();
}

void FRuntimeMeshData::HandleCommonSectionUpdateFlags(int32 SectionIndex, ESectionUpdateFlags UpdateFlags, ERuntimeMeshBuffersToUpdate& BuffersToUpdate)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_HandleCommonSectionUpdateFlags);

	check(DoesSectionExist(SectionIndex));

	FRuntimeMeshSectionPtr Section = MeshSections[SectionIndex];

	if (!!(UpdateFlags & ESectionUpdateFlags::CalculateNormalTangent) || !!(UpdateFlags & ESectionUpdateFlags::CalculateNormalTangentHard))
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_HandleCommonSectionUpdateFlags_CalculateTangents);
		URuntimeMeshLibrary::CalculateTangentsForMesh(Section->GetSectionMeshAccessor(), !(UpdateFlags & ESectionUpdateFlags::CalculateNormalTangentHard));
		BuffersToUpdate |= ERuntimeMeshBuffersToUpdate::AllVertexBuffers;
	}

	if (!!(UpdateFlags & ESectionUpdateFlags::CalculateTessellationIndices))
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_HandleCommonSectionUpdateFlags_CalculateTessellationIndices);
		URuntimeMeshLibrary::GenerateTessellationIndexBuffer(Section->GetSectionMeshAccessor(), Section->GetTessellationIndexAccessor());
		BuffersToUpdate |= ERuntimeMeshBuffersToUpdate::AdjacencyIndexBuffer;
	}
}

void FRuntimeMeshData::UpdateSectionPropertiesInternal(int32 SectionIndex, bool bUpdateRequiresProxyRecreateIfStatic)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateSectionPropertiesInternal);

	check(DoesSectionExist(SectionIndex));
	FRuntimeMeshSectionPtr Section = MeshSections[SectionIndex];

	if (RenderProxy.IsValid())
	{
		RenderProxy->UpdateSectionProperties_GameThread(SectionIndex, Section->GetSectionPropertyUpdateData());
	}

	bool bRequiresRecreate = bUpdateRequiresProxyRecreateIfStatic &&
		Section->GetUpdateFrequency() == EUpdateFrequency::Infrequent;

	if (bRequiresRecreate)
	{
		MarkRenderStateDirty();
	}
	else
	{
		SendSectionPropertiesUpdate(SectionIndex);
	}

	MarkChanged();
}

void FRuntimeMeshData::UpdateLocalBounds()
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateLocalBounds);

	FBox LocalBox(EForceInit::ForceInitToZero);

	for (int32 SectionId = 0; SectionId < MeshSections.Num(); SectionId++)
	{
		FRuntimeMeshSectionPtr Section = MeshSections[SectionId];
		if (Section.IsValid() && Section->ShouldRender())
		{
			LocalBox += Section->GetBoundingBox();
		}
	}

	LocalBounds = LocalBox.IsValid ? FBoxSphereBounds(LocalBox) :
		FBoxSphereBounds(FVector(0, 0, 0), FVector(0, 0, 0), 0); // fall back to reset box sphere bounds

	DoOnGameThread(FRuntimeMeshGameThreadTaskDelegate::CreateLambda(
		[](URuntimeMesh* Mesh)
	{
		Mesh->UpdateLocalBounds();
	}));
}

FRuntimeMeshProxyPtr FRuntimeMeshData::EnsureProxyCreated(ERHIFeatureLevel::Type InFeatureLevel)
{
	if (!RenderProxy.IsValid())
	{
		RenderProxy = MakeShareable(new FRuntimeMeshProxy(InFeatureLevel), FRuntimeMeshRenderThreadDeleter<FRuntimeMeshProxy>());
		Initialize();
	}

	// Sanity check that all RMC's are on the same feature level. Not sure any reason they wouldn't be.
	check(RenderProxy->GetFeatureLevel() == InFeatureLevel);

	return  RenderProxy;
}




void FRuntimeMeshData::Initialize()
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_Initialize);

	check(RenderProxy.IsValid());
	for (int32 SectionId = 0; SectionId < MeshSections.Num(); SectionId++)
	{
		if (MeshSections[SectionId].IsValid())
		{
			RenderProxy->CreateSection_GameThread(SectionId, MeshSections[SectionId]->GetSectionCreationParams());
		}
	}
}

bool FRuntimeMeshData::ContainsPhysicsTriMeshData(bool InUseAllTriData) const
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_ContainsPhysicsTriMeshData);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	for (int32 SectionId = 0; SectionId < MeshSections.Num(); SectionId++)
	{
		FRuntimeMeshSectionPtr Section = MeshSections[SectionId];
		if (Section.IsValid() && Section->HasValidMeshData() && Section->IsCollisionEnabled())
		{
			return true;
		}
	}

	for (const auto& Section : MeshCollisionSections)
	{
		if (Section.Value.VertexBuffer.Num() > 0 && Section.Value.IndexBuffer.Num() > 0)
		{
			return true;
		}
	}

	return false;
}

bool FRuntimeMeshData::GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_GetPhysicsTriMeshData);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	// Base vertex index for current section

	bool bHadCollision = false;

	// See if we should copy UVs
	bool bCopyUVs = UPhysicsSettings::Get()->bSupportUVFromHitResults;

	for (int32 SectionId = 0; SectionId < MeshSections.Num(); SectionId++)
	{
		if (MeshSections[SectionId].IsValid() && MeshSections[SectionId]->IsCollisionEnabled())
		{
			TArray<FVector2D> UVs;
			int32 NumTriangles = MeshSections[SectionId]->GetCollisionData(CollisionData->Vertices, CollisionData->Indices, UVs);

			if (bCopyUVs)
			{
				CollisionData->UVs.Add(MoveTemp(UVs));
			}

			for (int32 Index = 0; Index < NumTriangles; Index++)
			{
				CollisionData->MaterialIndices.Add(SectionId);
			}

			// Update the vertex base index
			bHadCollision = true;
		}
	}

	int32 VertexBase = CollisionData->Vertices.Num();

	for (const auto& SectionEntry : MeshCollisionSections)
	{
		const auto& Section = SectionEntry.Value;
		if (Section.VertexBuffer.Num() > 0 && Section.IndexBuffer.Num() > 0)
		{
			CollisionData->Vertices.Append(Section.VertexBuffer);

			const int32 NumTriangles = Section.IndexBuffer.Num() / 3;
			for (int32 TriIdx = 0; TriIdx < NumTriangles; TriIdx++)
			{
				// Add the triangle
				FTriIndices& Triangle = *new (CollisionData->Indices) FTriIndices;
				Triangle.v0 = Section.IndexBuffer[(TriIdx * 3) + 0] + VertexBase;
				Triangle.v1 = Section.IndexBuffer[(TriIdx * 3) + 1] + VertexBase;
				Triangle.v2 = Section.IndexBuffer[(TriIdx * 3) + 2] + VertexBase;

				// Add material info
				CollisionData->MaterialIndices.Add(SectionEntry.Key);
			}


			VertexBase = CollisionData->Vertices.Num();
			bHadCollision = true;
		}
	}

	CollisionData->bFlipNormals = true;

	return bHadCollision;
}

void FRuntimeMeshData::CopyCollisionElementsToBodySetup(UBodySetup* Setup)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_CopyCollisionElementsToBodySetup);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	check(IsInGameThread());

	auto& ConvexElems = Setup->AggGeom.ConvexElems;
	ConvexElems.Empty();
	for (const auto& Section : ConvexCollisionSections)
	{
		FKConvexElem& NewConvexElem = *new(ConvexElems) FKConvexElem();
		NewConvexElem.VertexData = Section.Value.VertexBuffer;
		// TODO: Store this on the section so we don't have to compute it on each cook
		NewConvexElem.ElemBox = Section.Value.BoundingBox;
	}

	auto& BoxElems = Setup->AggGeom.BoxElems;
	BoxElems.Empty();
	for (const FRuntimeMeshCollisionBox& Box : CollisionBoxes)
	{
		FKBoxElem& NewBox = *new(BoxElems) FKBoxElem();
		NewBox.Center = Box.Center;
		NewBox.Rotation = Box.Rotation;
		NewBox.X = Box.Extents.X;
		NewBox.Y = Box.Extents.Y;
		NewBox.Z = Box.Extents.Z;
	}

	auto& SphereElems = Setup->AggGeom.SphereElems;
	SphereElems.Empty();
	for (const FRuntimeMeshCollisionSphere& Sphere : CollisionSpheres)
	{
		FKSphereElem& NewSphere = *new(SphereElems)FKSphereElem();
		NewSphere.Center = Sphere.Center;
		NewSphere.Radius = Sphere.Radius;
	}

	auto& SphylElems = Setup->AggGeom.SphylElems;
	SphylElems.Empty();
	for (const FRuntimeMeshCollisionCapsule& Capsule : CollisionCapsules)
	{
		FKSphylElem& NewSphyl = *new(SphylElems)FKSphylElem();
		NewSphyl.Center = Capsule.Center;
		NewSphyl.Rotation = Capsule.Rotation;
		NewSphyl.Radius = Capsule.Radius;
		NewSphyl.Length = Capsule.Length;
	}
}

void FRuntimeMeshData::MarkCollisionDirty(bool bSkipChangedFlag)
{
	if (bSkipChangedFlag)
	{
		DoOnGameThread(FRuntimeMeshGameThreadTaskDelegate::CreateLambda(
			[](URuntimeMesh* Mesh)
		{
			Mesh->MarkCollisionDirty();
		}));
	}
	else
	{
		DoOnGameThread(FRuntimeMeshGameThreadTaskDelegate::CreateLambda(
			[](URuntimeMesh* Mesh)
		{
			Mesh->MarkCollisionDirty();
			Mesh->MarkChanged();
		}));
	}
}


void FRuntimeMeshData::MarkRenderStateDirty()
{
	DoOnGameThread(FRuntimeMeshGameThreadTaskDelegate::CreateLambda(
		[](URuntimeMesh* Mesh)
	{
		Mesh->ForceProxyRecreate();
	}));
}

void FRuntimeMeshData::SendSectionPropertiesUpdate(int32 SectionIndex)
{
	DoOnGameThread(FRuntimeMeshGameThreadTaskDelegate::CreateLambda(
		[SectionIndex](URuntimeMesh* Mesh)
	{
		Mesh->SendSectionPropertiesUpdate(SectionIndex);
	}));
}

int32 FRuntimeMeshData::GetSectionFromCollisionFaceIndex(int32 FaceIndex) const
{
	int32 FaceIdx = FaceIndex;

	return GetSectionAndFaceFromCollisionFaceIndex(FaceIdx);
}
/*
* Gets the section ID from the given face index reference,
* The face index reference then gets set to it's face index in the section.
*/
int32 FRuntimeMeshData::GetSectionAndFaceFromCollisionFaceIndex(int32& FaceIndex) const
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_GetSectionFromCollisionFaceIndex);

	FRuntimeMeshScopeLock Lock(SyncRoot);

	int32 SectionIndex = 0;

	// Look for element that corresponds to the supplied face
	int32 TotalFaceCount = 0;

	for (int32 SectionIdx = 0; SectionIdx < MeshSections.Num(); SectionIdx++)
	{
		const FRuntimeMeshSectionPtr& Section = MeshSections[SectionIdx];

		if (Section.IsValid() && Section->IsCollisionEnabled())
		{

			int32 NumFaces = Section->GetNumIndices() / 3;

			if (FaceIndex < TotalFaceCount + NumFaces)
			{
				// Grab the material
				SectionIndex = SectionIdx;
				FaceIndex -= TotalFaceCount;
				break;
			}
			TotalFaceCount += NumFaces;
		}
	}
	return SectionIndex;
}

class FRuntimeMeshGameThreadTask
{
	TWeakObjectPtr<URuntimeMesh> RuntimeMesh;
	FRuntimeMeshGameThreadTaskDelegate Delegate;
public:

	FRuntimeMeshGameThreadTask(TWeakObjectPtr<URuntimeMesh> InRuntimeMesh, FRuntimeMeshGameThreadTaskDelegate InDelegate)
		: RuntimeMesh(InRuntimeMesh), Delegate(InDelegate)
	{
	}

	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FRuntimeMeshGameThreadTask, STATGROUP_TaskGraphTasks);
	}

	static ENamedThreads::Type GetDesiredThread()
	{
		return ENamedThreads::GameThread;
	}

	static ESubsequentsMode::Type GetSubsequentsMode()
	{
		return ESubsequentsMode::FireAndForget;
	}

	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
	{
		if (URuntimeMesh* Comp = RuntimeMesh.Get())
		{
			Delegate.Execute(Comp);
		}
	}
};


void FRuntimeMeshData::DoOnGameThread(FRuntimeMeshGameThreadTaskDelegate Func)
{
	if (IsInGameThread())
	{
		if (URuntimeMesh* Comp = ParentMeshObject.Get())
		{
			Func.Execute(Comp);
		}
		else
		{
			check(false);
		}
	}
	else
	{
		TGraphTask<FRuntimeMeshGameThreadTask>::CreateTask().ConstructAndDispatchWhenReady(ParentMeshObject, Func);
	}
}

void FRuntimeMeshData::MarkChanged()
{
#if WITH_EDITOR
	DoOnGameThread(FRuntimeMeshGameThreadTaskDelegate::CreateLambda([](URuntimeMesh* Mesh)
	{
		Mesh->MarkChanged();
	}));
#endif
}
