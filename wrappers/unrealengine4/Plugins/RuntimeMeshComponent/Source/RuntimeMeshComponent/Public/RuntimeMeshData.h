// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshCore.h"
#include "RuntimeMeshCollision.h"
#include "RuntimeMeshSection.h"
#include "RuntimeMeshBlueprint.h"

class URuntimeMesh;
class FRuntimeMeshProxy;
class FRuntimeMeshBuilder;

using FRuntimeMeshProxyPtr = TSharedPtr<FRuntimeMeshProxy, ESPMode::ThreadSafe>;

DECLARE_DELEGATE_OneParam(FRuntimeMeshGameThreadTaskDelegate, URuntimeMesh*);



DECLARE_CYCLE_STAT(TEXT("RM - Create Mesh Section - No Data"), STAT_RuntimeMesh_CreateMeshSection_NoData, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Create Dual Buffer Mesh Section - No Data"), STAT_RuntimeMesh_CreateMeshSectionDualBuffer_NoData, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Create Triple Buffer Mesh Section - No Data"), STAT_RuntimeMesh_CreateMeshSectionTripleBuffer_NoData, STATGROUP_RuntimeMesh);

DECLARE_CYCLE_STAT(TEXT("RM - Create Mesh Section"), STAT_RuntimeMesh_CreateMeshSection, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Create Mesh Section - With Bounding Box"), STAT_RuntimeMesh_CreateMeshSection_BoundingBox, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Create Mesh Section Dual Buffer"), STAT_RuntimeMesh_CreateMeshSectionDualBuffer, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Create Mesh Section Dual Buffer - With Bounding Box"), STAT_RuntimeMesh_CreateMeshSectionDualBuffer_BoundingBox, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Create Mesh Section Triple Buffer"), STAT_RuntimeMesh_CreateMeshSectionTripleBuffer, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Create Mesh Section Triple Buffer - With Bounding Box"), STAT_RuntimeMesh_CreateMeshSectionTripleBuffer_BoundingBox, STATGROUP_RuntimeMesh);

DECLARE_CYCLE_STAT(TEXT("RM - Update Mesh Section - No Triangles"), STAT_RuntimeMesh_UpdateMeshSection_NoTriangles, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Update Mesh Section - No Triangles - With Bounding Box"), STAT_RuntimeMesh_UpdateMeshSection_NoTriangles_BoundingBox, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Update Mesh Section"), STAT_RuntimeMesh_UpdateMeshSection, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Update Mesh Section - With Bounding Box"), STAT_RuntimeMesh_UpdateMeshSection_BoundingBox, STATGROUP_RuntimeMesh);

DECLARE_CYCLE_STAT(TEXT("RM - Update Mesh Section Dual Buffer - No Triangles"), STAT_RuntimeMesh_UpdateMeshSectionDualBuffer_NoTriangles, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Update Mesh Section Dual Buffer - No Triangles - With Bounding Box"), STAT_RuntimeMesh_UpdateMeshSectionDualBuffer_NoTriangles_BoundingBox, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Update Mesh Section Dual Buffer"), STAT_RuntimeMesh_UpdateMeshSectionDualBuffer, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Update Mesh Section Dual Buffer - With Bounding Box"), STAT_RuntimeMesh_UpdateMeshSectionDualBuffer_BoundingBox, STATGROUP_RuntimeMesh);

DECLARE_CYCLE_STAT(TEXT("RM - Update Mesh Section Triple Buffer - No Triangles"), STAT_RuntimeMesh_UpdateMeshSectionTripleBuffer_NoTriangles, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Update Mesh Section Triple Buffer - No Triangles - With Bounding Box"), STAT_RuntimeMesh_UpdateMeshSectionTripleBuffer_NoTriangles_BoundingBox, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Update Mesh Section Triple Buffer"), STAT_RuntimeMesh_UpdateMeshSectionTripleBuffer, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Update Mesh Section Triple Buffer - With Bounding Box"), STAT_RuntimeMesh_UpdateMeshSectionTripleBuffer_BoundingBox, STATGROUP_RuntimeMesh);

DECLARE_CYCLE_STAT(TEXT("RM - Update Mesh Section Primary Buffer"), STAT_RuntimeMesh_UpdateMeshSectionPrimaryBuffer, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Update Mesh Section Primary Buffer - With Bounding Box"), STAT_RuntimeMesh_UpdateMeshSectionPrimaryBuffer_BoundingBox, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Update Mesh Section Secondary Buffer"), STAT_RuntimeMesh_UpdateMeshSectionSecondaryBuffer, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Update Mesh Section Tertiary Buffer"), STAT_RuntimeMesh_UpdateMeshSectionTertiaryBuffer, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Update Mesh Section Triangles"), STAT_RuntimeMesh_UpdateMeshSectionTriangles, STATGROUP_RuntimeMesh);

DECLARE_CYCLE_STAT(TEXT("RM - Set Section Tessellation Triangles"), STAT_RuntimeMesh_SetSectionTessellationTriangles, STATGROUP_RuntimeMesh);

DECLARE_CYCLE_STAT(TEXT("RM - Serialize Data"), STAT_RuntimeMesh_SerializationOperator, STATGROUP_RuntimeMesh);


/**
 *
 */
class RUNTIMEMESHCOMPONENT_API FRuntimeMeshData : public TSharedFromThis<FRuntimeMeshData, ESPMode::ThreadSafe>
{
	/** Array of sections of mesh */
	TArray<FRuntimeMeshSectionPtr> MeshSections;

	/* Array of collision only mesh sections*/
	TMap<int32, FRuntimeMeshCollisionSection> MeshCollisionSections;

	/** Convex shapes used for simple collision */
	TMap<int32, FRuntimeMeshCollisionConvexMesh> ConvexCollisionSections;

	TArray<FRuntimeMeshCollisionBox> CollisionBoxes;
	TArray<FRuntimeMeshCollisionSphere> CollisionSpheres;
	TArray<FRuntimeMeshCollisionCapsule> CollisionCapsules;

	/** Local space bounds of mesh */
	FBoxSphereBounds LocalBounds;

	/** Parent mesh object that owns this data. */
	TWeakObjectPtr<URuntimeMesh> ParentMeshObject;

	/** Render proxy for this mesh */
	FRuntimeMeshProxyPtr RenderProxy;

	TUniquePtr<FRuntimeMeshLockProvider> SyncRoot;
	
public:
	FRuntimeMeshData();
	~FRuntimeMeshData();

private:
	void CheckCreate(int32 NumUVs, bool bIndexIsValid) const;

	void CheckCreateLegacyInternal(const FRuntimeMeshVertexStreamStructure& Stream0Structure, const FRuntimeMeshVertexStreamStructure& Stream1Structure, const FRuntimeMeshVertexStreamStructure& Stream2Structure, bool bIsIndexValid) const;

	template<typename VertexType0, typename VertexType1, typename VertexType2, typename IndexType>
	void CheckCreateLegacy() const
	{
		CheckCreateLegacyInternal(GetStreamStructure<VertexType0>(), GetStreamStructure<VertexType1>(), GetStreamStructure<VertexType2>(), FRuntimeMeshIndexTraits<IndexType>::IsValidIndexType);
	}

	void CheckUpdate(bool bUseHighPrecisionTangents, bool bUseHighPrecisionUVs, int32 NumUVs, bool b32BitIndices, int32 SectionIndex, bool bShouldCheckIndexType, 
		bool bCheckTangentVertexStream, bool bCheckUVVertexStream) const;

	template<typename TangentType, typename UVType, typename IndexType>
	void CheckUpdate(int32 SectionIndex, bool bShouldCheckIndexType, bool bCheckTangentVertexStream, bool bCheckUVVertexStream) const
	{
#if DO_CHECK
		bool UVsAreHighPrecision;
		int32 NumUVs;
		GetUVVertexProperties<UVType>(UVsAreHighPrecision, NumUVs);
		CheckUpdate(GetTangentIsHighPrecision<TangentType>(), UVsAreHighPrecision, NumUVs, FRuntimeMeshIndexTraits<IndexType>::Is32Bit, SectionIndex, bShouldCheckIndexType, bCheckTangentVertexStream, bCheckUVVertexStream);
#endif
	}

	template<typename VertexType0, typename VertexType1, typename VertexType2, typename IndexType>
	void CheckUpdateLegacy(int32 SectionIndex, bool bShouldCheckIndexType = true) const
	{
	}

	void CheckBoundingBox(const FBox& Box) const;

public:

	/* 
		This will put the RuntimeMesh into serialized mode where it becomes safe to access it from other threads. 
		Don't do this if you don't need it as it will make interacting with it slighly slower as it incurs thread locks.
	*/
	void EnterSerializedMode();


	void CreateMeshSection(int32 SectionIndex, bool bWantsHighPrecisionTangents, bool bWantsHighPrecisionUVs, int32 NumUVs, bool bWants32BitIndices, bool bCreateCollision, EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average);
	
	template<typename VertexType0, typename IndexType>
	void CreateMeshSection(int32 SectionIndex, TArray<VertexType0>& InVertices0, TArray<IndexType>& InTriangles, bool bCreateCollision = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_CreateMeshSection);

		FRuntimeMeshScopeLock Lock(SyncRoot);

		CheckCreateLegacy<VertexType0, FRuntimeMeshNullVertex, FRuntimeMeshNullVertex, IndexType>();

		bool bWantsHighPrecisionTangents = FRuntimeMeshVertexTypeTraitsAggregator::IsUsingHighPrecisionTangents<VertexType0>();
		bool bWantsHighPrecisionUVs = FRuntimeMeshVertexTypeTraitsAggregator::IsUsingHighPrecisionUVs<VertexType0>();
		int32 NumUVs = FRuntimeMeshVertexTypeTraitsAggregator::NumUVs<VertexType0>();
		bool bUsing32BitIndices = FRuntimeMeshIndexTraits<IndexType>::Is32Bit;

		CreateMeshSection(SectionIndex, bWantsHighPrecisionTangents, bWantsHighPrecisionUVs, NumUVs, bUsing32BitIndices, bCreateCollision, UpdateFrequency);

		auto Mesh = BeginSectionUpdate(SectionIndex, UpdateFlags);
		
		Mesh->EmptyVertices(InVertices0.Num());

		// Copy the mesh data to the mesh builder
		for (int32 Index = 0; Index < InVertices0.Num(); Index++)
		{
			Mesh->AddVertexByProperties<VertexType0>(InVertices0[Index]);
		}

		Mesh->EmptyIndices(InTriangles.Num());

		for (int32 Index = 0; Index < InTriangles.Num(); Index++)
		{
			Mesh->AddIndex(InTriangles[Index]);
		}
		
		Mesh->Commit();
	}

	template<typename VertexType0, typename IndexType>
	void CreateMeshSection(int32 SectionIndex, TArray<VertexType0>& InVertices0, TArray<IndexType>& InTriangles, const FBox& BoundingBox,
		bool bCreateCollision = false, EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_CreateMeshSection_BoundingBox);

		FRuntimeMeshScopeLock Lock(SyncRoot);

		CheckCreateLegacy<VertexType0, FRuntimeMeshNullVertex, FRuntimeMeshNullVertex, IndexType>();

		bool bWantsHighPrecisionTangents = FRuntimeMeshVertexTypeTraitsAggregator::IsUsingHighPrecisionTangents<VertexType0>();
		bool bWantsHighPrecisionUVs = FRuntimeMeshVertexTypeTraitsAggregator::IsUsingHighPrecisionUVs<VertexType0>();
		int32 NumUVs = FRuntimeMeshVertexTypeTraitsAggregator::NumUVs<VertexType0>();
		bool bUsing32BitIndices = FRuntimeMeshIndexTraits<IndexType>::Is32Bit;

		CreateMeshSection(SectionIndex, bWantsHighPrecisionTangents, bWantsHighPrecisionUVs, NumUVs, bUsing32BitIndices, bCreateCollision, UpdateFrequency);

		auto Mesh = BeginSectionUpdate(SectionIndex, UpdateFlags);

		Mesh->EmptyVertices(InVertices0.Num());

		// Copy the mesh data to the mesh builder
		for (int32 Index = 0; Index < InVertices0.Num(); Index++)
		{
			Mesh->AddVertexByProperties<VertexType0>(InVertices0[Index]);
		}

		Mesh->EmptyIndices(InTriangles.Num());

		for (int32 Index = 0; Index < InTriangles.Num(); Index++)
		{
			Mesh->AddIndex(InTriangles[Index]);
		}

		Mesh->Commit(BoundingBox);
	}

	template<typename VertexType0, typename VertexType1, typename IndexType>
	void CreateMeshSectionDualBuffer(int32 SectionIndex, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1, TArray<IndexType>& InTriangles, bool bCreateCollision = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_CreateMeshSectionDualBuffer);

		FRuntimeMeshScopeLock Lock(SyncRoot);

		CheckCreateLegacy<VertexType0, VertexType1, FRuntimeMeshNullVertex, IndexType>();

		bool bWantsHighPrecisionTangents = FRuntimeMeshVertexTypeTraitsAggregator::IsUsingHighPrecisionTangents<VertexType0, VertexType1>();
		bool bWantsHighPrecisionUVs = FRuntimeMeshVertexTypeTraitsAggregator::IsUsingHighPrecisionUVs<VertexType0, VertexType1>();
		int32 NumUVs = FRuntimeMeshVertexTypeTraitsAggregator::NumUVs<VertexType0, VertexType1>();
		bool bUsing32BitIndices = FRuntimeMeshIndexTraits<IndexType>::Is32Bit;

		CreateMeshSection(SectionIndex, bWantsHighPrecisionTangents, bWantsHighPrecisionUVs, NumUVs, bUsing32BitIndices, bCreateCollision, UpdateFrequency);

		auto Mesh = BeginSectionUpdate(SectionIndex, UpdateFlags);

		Mesh->EmptyVertices(InVertices0.Num());

		// Copy the mesh data to the mesh builder
		for (int32 Index = 0; Index < InVertices0.Num(); Index++)
		{
			Mesh->AddVertexByProperties<VertexType0, VertexType1>(InVertices0[Index], InVertices1[Index]);
		}

		Mesh->EmptyIndices(InTriangles.Num());

		for (int32 Index = 0; Index < InTriangles.Num(); Index++)
		{
			Mesh->AddIndex(InTriangles[Index]);
		}

		Mesh->Commit();
	}

	template<typename VertexType0, typename VertexType1, typename IndexType>
	void CreateMeshSectionDualBuffer(int32 SectionIndex, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1, TArray<IndexType>& InTriangles, const FBox& BoundingBox,
		bool bCreateCollision = false, EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_CreateMeshSectionDualBuffer_BoundingBox);

		FRuntimeMeshScopeLock Lock(SyncRoot);

		CheckCreateLegacy<VertexType0, VertexType1, FRuntimeMeshNullVertex, IndexType>();

		bool bWantsHighPrecisionTangents = FRuntimeMeshVertexTypeTraitsAggregator::IsUsingHighPrecisionTangents<VertexType0, VertexType1>();
		bool bWantsHighPrecisionUVs = FRuntimeMeshVertexTypeTraitsAggregator::IsUsingHighPrecisionUVs<VertexType0, VertexType1>();
		int32 NumUVs = FRuntimeMeshVertexTypeTraitsAggregator::NumUVs<VertexType0, VertexType1>();
		bool bUsing32BitIndices = FRuntimeMeshIndexTraits<IndexType>::Is32Bit;

		CreateMeshSection(SectionIndex, bWantsHighPrecisionTangents, bWantsHighPrecisionUVs, NumUVs, bUsing32BitIndices, bCreateCollision, UpdateFrequency);

		auto Mesh = BeginSectionUpdate(SectionIndex, UpdateFlags);

		Mesh->EmptyVertices(InVertices0.Num());

		// Copy the mesh data to the mesh builder
		for (int32 Index = 0; Index < InVertices0.Num(); Index++)
		{
			Mesh->AddVertexByProperties<VertexType0, VertexType1>(InVertices0[Index], InVertices1[Index]);
		}

		Mesh->EmptyIndices(InTriangles.Num());

		for (int32 Index = 0; Index < InTriangles.Num(); Index++)
		{
			Mesh->AddIndex(InTriangles[Index]);
		}

		Mesh->Commit(BoundingBox);
	}

	template<typename VertexType0, typename VertexType1, typename VertexType2, typename IndexType>
	void CreateMeshSectionTripleBuffer(int32 SectionIndex, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1, TArray<VertexType2>& InVertices2, TArray<IndexType>& InTriangles,
		bool bCreateCollision = false, EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_CreateMeshSectionTripleBuffer);

		FRuntimeMeshScopeLock Lock(SyncRoot);
		
		CheckCreateLegacy<VertexType0, VertexType1, VertexType2, IndexType>();

		bool bWantsHighPrecisionTangents = FRuntimeMeshVertexTypeTraitsAggregator::IsUsingHighPrecisionTangents<VertexType0, VertexType1, VertexType2>();
		bool bWantsHighPrecisionUVs = FRuntimeMeshVertexTypeTraitsAggregator::IsUsingHighPrecisionUVs<VertexType0, VertexType1, VertexType2>();
		int32 NumUVs = FRuntimeMeshVertexTypeTraitsAggregator::NumUVs<VertexType0, VertexType1, VertexType2>();
		bool bUsing32BitIndices = FRuntimeMeshIndexTraits<IndexType>::Is32Bit;

		CreateMeshSection(SectionIndex, bWantsHighPrecisionTangents, bWantsHighPrecisionUVs, NumUVs, bUsing32BitIndices, bCreateCollision, UpdateFrequency);

		auto Mesh = BeginSectionUpdate(SectionIndex, UpdateFlags);

		Mesh->EmptyVertices(InVertices0.Num());

		// Copy the mesh data to the mesh builder
		for (int32 Index = 0; Index < InVertices0.Num(); Index++)
		{
			Mesh->AddVertexByProperties<VertexType0, VertexType1, VertexType2>(InVertices0[Index], InVertices1[Index], InVertices2[Index]);
		}

		Mesh->EmptyIndices(InTriangles.Num());

		for (int32 Index = 0; Index < InTriangles.Num(); Index++)
		{
			Mesh->AddIndex(InTriangles[Index]);
		}

		Mesh->Commit();
	}

	template<typename VertexType0, typename VertexType1, typename VertexType2, typename IndexType>
	void CreateMeshSectionTripleBuffer(int32 SectionIndex, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1, TArray<VertexType2>& InVertices2, TArray<IndexType>& InTriangles,
		const FBox& BoundingBox, bool bCreateCollision = false, EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_CreateMeshSectionTripleBuffer_BoundingBox);

		FRuntimeMeshScopeLock Lock(SyncRoot);

		CheckCreateLegacy<VertexType0, VertexType1, VertexType2, IndexType>();

		bool bWantsHighPrecisionTangents = FRuntimeMeshVertexTypeTraitsAggregator::IsUsingHighPrecisionTangents<VertexType0, VertexType1, VertexType2>();
		bool bWantsHighPrecisionUVs = FRuntimeMeshVertexTypeTraitsAggregator::IsUsingHighPrecisionUVs<VertexType0, VertexType1, VertexType2>();
		int32 NumUVs = FRuntimeMeshVertexTypeTraitsAggregator::NumUVs<VertexType0, VertexType1, VertexType2>();
		bool bUsing32BitIndices = FRuntimeMeshIndexTraits<IndexType>::Is32Bit;

		CreateMeshSection(SectionIndex, bWantsHighPrecisionTangents, bWantsHighPrecisionUVs, NumUVs, bUsing32BitIndices, bCreateCollision, UpdateFrequency);

		auto Mesh = BeginSectionUpdate(SectionIndex, UpdateFlags);

		Mesh->EmptyVertices(InVertices0.Num());

		// Copy the mesh data to the mesh builder
		for (int32 Index = 0; Index < InVertices0.Num(); Index++)
		{
			Mesh->AddVertexByProperties<VertexType0, VertexType1, VertexType2>(InVertices0[Index], InVertices1[Index], InVertices2[Index]);
		}

		Mesh->EmptyIndices(InTriangles.Num());

		for (int32 Index = 0; Index < InTriangles.Num(); Index++)
		{
			Mesh->AddIndex(InTriangles[Index]);
		}

		Mesh->Commit(BoundingBox);
	}



	template<typename VertexType0>
	void UpdateMeshSection(int32 SectionId, TArray<VertexType0>& InVertices0, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSection_NoTriangles);

		FRuntimeMeshScopeLock Lock(SyncRoot);

		CheckUpdateLegacy<VertexType0, FRuntimeMeshNullVertex, FRuntimeMeshNullVertex, uint16>(SectionId, false);

		auto Mesh = BeginSectionUpdate(SectionId, UpdateFlags);

		Mesh->SetNumVertices(InVertices0.Num());

		// Copy the mesh data to the mesh builder
		for (int32 Index = 0; Index < InVertices0.Num(); Index++)
		{
			Mesh->SetVertexProperties<VertexType0>(Index, InVertices0[Index]);
		}

		Mesh->Commit();
	}

	template<typename VertexType0>
	void UpdateMeshSection(int32 SectionId, TArray<VertexType0>& InVertices0, const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSection_NoTriangles_BoundingBox);

		FRuntimeMeshScopeLock Lock(SyncRoot);

		CheckUpdateLegacy<VertexType0, FRuntimeMeshNullVertex, FRuntimeMeshNullVertex, uint16>(SectionId, false);
		CheckBoundingBox(BoundingBox);

		auto Mesh = BeginSectionUpdate(SectionId, UpdateFlags);

		Mesh->SetNumVertices(InVertices0.Num());

		// Copy the mesh data to the mesh builder
		for (int32 Index = 0; Index < InVertices0.Num(); Index++)
		{
			Mesh->SetVertexProperties<VertexType0>(Index, InVertices0[Index]);
		}

		Mesh->Commit(BoundingBox);
	}

	template<typename VertexType0, typename IndexType>
	void UpdateMeshSection(int32 SectionId, TArray<VertexType0>& InVertices0, TArray<IndexType>& InTriangles, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSection);

		FRuntimeMeshScopeLock Lock(SyncRoot);

		CheckUpdateLegacy<VertexType0, FRuntimeMeshNullVertex, FRuntimeMeshNullVertex, IndexType>(SectionId, true);

		auto Mesh = BeginSectionUpdate(SectionId, UpdateFlags);

		Mesh->SetNumVertices(InVertices0.Num());

		// Copy the mesh data to the mesh builder
		for (int32 Index = 0; Index < InVertices0.Num(); Index++)
		{
			Mesh->SetVertexProperties<VertexType0>(Index, InVertices0[Index]);
		}

		Mesh->SetNumIndices(InTriangles.Num());

		for (int32 Index = 0; Index < InTriangles.Num(); Index++)
		{
			Mesh->SetIndex(Index, InTriangles[Index]);
		}

		Mesh->Commit();
	}

	template<typename VertexType0, typename IndexType>
	void UpdateMeshSection(int32 SectionId, TArray<VertexType0>& InVertices0, TArray<IndexType>& InTriangles,
		const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSection_BoundingBox);

		FRuntimeMeshScopeLock Lock(SyncRoot);

		CheckUpdateLegacy<VertexType0, FRuntimeMeshNullVertex, FRuntimeMeshNullVertex, IndexType>(SectionId, true);
		CheckBoundingBox(BoundingBox);

		auto Mesh = BeginSectionUpdate(SectionId, UpdateFlags);

		Mesh->SetNumVertices(InVertices0.Num());

		// Copy the mesh data to the mesh builder
		for (int32 Index = 0; Index < InVertices0.Num(); Index++)
		{
			Mesh->SetVertexProperties<VertexType0>(Index, InVertices0[Index]);
		}

		Mesh->SetNumIndices(InTriangles.Num());

		for (int32 Index = 0; Index < InTriangles.Num(); Index++)
		{
			Mesh->SetIndex(Index, InTriangles[Index]);
		}

		Mesh->Commit(BoundingBox);
	}

	template<typename VertexType0, typename VertexType1>
	void UpdateMeshSectionDualBuffer(int32 SectionId, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSectionDualBuffer_NoTriangles);

		FRuntimeMeshScopeLock Lock(SyncRoot);

		CheckUpdateLegacy<VertexType0, VertexType1, FRuntimeMeshNullVertex, uint16>(SectionId, false);
		
		auto Mesh = BeginSectionUpdate(SectionId, UpdateFlags);

		Mesh->SetNumVertices(InVertices0.Num());

		// Copy the mesh data to the mesh builder
		for (int32 Index = 0; Index < InVertices0.Num(); Index++)
		{
			Mesh->SetVertexProperties<VertexType0>(Index, InVertices0[Index]);
			if (InVertices1.Num() > Index)
			{
				Mesh->SetVertexProperties<VertexType1>(Index, InVertices1[Index]);
			}
		}

		Mesh->Commit();
	}

	template<typename VertexType0, typename VertexType1>
	void UpdateMeshSectionDualBuffer(int32 SectionId, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1,
		const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSectionDualBuffer_NoTriangles_BoundingBox);

		FRuntimeMeshScopeLock Lock(SyncRoot);

		CheckUpdateLegacy<VertexType0, VertexType1, FRuntimeMeshNullVertex, uint16>(SectionId, false);
		CheckBoundingBox(BoundingBox);

		auto Mesh = BeginSectionUpdate(SectionId, UpdateFlags);

		Mesh->SetNumVertices(InVertices0.Num());

		// Copy the mesh data to the mesh builder
		for (int32 Index = 0; Index < InVertices0.Num(); Index++)
		{
			Mesh->SetVertexProperties<VertexType0>(Index, InVertices0[Index]);
			if (InVertices1.Num() > Index)
			{
				Mesh->SetVertexProperties<VertexType1>(Index, InVertices1[Index]);
			}
		}

		Mesh->Commit(BoundingBox);
	}

	template<typename VertexType0, typename VertexType1, typename IndexType>
	void UpdateMeshSectionDualBuffer(int32 SectionId, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1,
		TArray<IndexType>& InTriangles, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSectionDualBuffer);

		FRuntimeMeshScopeLock Lock(SyncRoot);

		CheckUpdateLegacy<VertexType0, VertexType1, FRuntimeMeshNullVertex, IndexType>(SectionId, true);

		auto Mesh = BeginSectionUpdate(SectionId, UpdateFlags);

		Mesh->SetNumVertices(InVertices0.Num());

		// Copy the mesh data to the mesh builder
		for (int32 Index = 0; Index < InVertices0.Num(); Index++)
		{
			Mesh->SetVertexProperties<VertexType0>(Index, InVertices0[Index]);
			if (InVertices1.Num() > Index)
			{
				Mesh->SetVertexProperties<VertexType1>(Index, InVertices1[Index]);
			}
		}

		Mesh->SetNumIndices(InTriangles.Num());

		for (int32 Index = 0; Index < InTriangles.Num(); Index++)
		{
			Mesh->SetIndex(Index, InTriangles[Index]);
		}

		Mesh->Commit();
	}

	template<typename VertexType0, typename VertexType1, typename IndexType>
	void UpdateMeshSectionDualBuffer(int32 SectionId, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1, TArray<IndexType>& InTriangles,
		const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSectionDualBuffer_BoundingBox);

		FRuntimeMeshScopeLock Lock(SyncRoot);

		CheckUpdateLegacy<VertexType0, VertexType1, FRuntimeMeshNullVertex, IndexType>(SectionId, true);
		CheckBoundingBox(BoundingBox);

		auto Mesh = BeginSectionUpdate(SectionId, UpdateFlags);

		Mesh->SetNumVertices(InVertices0.Num());

		// Copy the mesh data to the mesh builder
		for (int32 Index = 0; Index < InVertices0.Num(); Index++)
		{
			Mesh->SetVertexProperties<VertexType0>(Index, InVertices0[Index]);
			if (InVertices1.Num() > Index)
			{
				Mesh->SetVertexProperties<VertexType1>(Index, InVertices1[Index]);
			}
		}

		Mesh->SetNumIndices(InTriangles.Num());

		for (int32 Index = 0; Index < InTriangles.Num(); Index++)
		{
			Mesh->SetIndex(Index, InTriangles[Index]);
		}

		Mesh->Commit(BoundingBox);
	}

	template<typename VertexType0, typename VertexType1, typename VertexType2>
	void UpdateMeshSectionTripleBuffer(int32 SectionId, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1, TArray<VertexType2>& InVertices2,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSectionTripleBuffer_NoTriangles);

		FRuntimeMeshScopeLock Lock(SyncRoot);

		CheckUpdateLegacy<VertexType0, VertexType1, VertexType2, uint16>(SectionId, false);

		auto Mesh = BeginSectionUpdate(SectionId, UpdateFlags);

		Mesh->SetNumVertices(InVertices0.Num());

		// Copy the mesh data to the mesh builder
		for (int32 Index = 0; Index < InVertices0.Num(); Index++)
		{
			Mesh->SetVertexProperties<VertexType0>(Index, InVertices0[Index]);
			if (InVertices1.Num() > Index)
			{
				Mesh->SetVertexProperties<VertexType1>(Index, InVertices1[Index]);
			}
			if (InVertices2.Num() > Index)
			{
				Mesh->SetVertexProperties<VertexType2>(Index, InVertices2[Index]);
			}
		}

		Mesh->Commit();
	}

	template<typename VertexType0, typename VertexType1, typename VertexType2>
	void UpdateMeshSectionTripleBuffer(int32 SectionId, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1, TArray<VertexType2>& InVertices2,
		const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSectionTripleBuffer_NoTriangles_BoundingBox);

		FRuntimeMeshScopeLock Lock(SyncRoot);

		CheckUpdateLegacy<VertexType0, VertexType1, VertexType2, uint16>(SectionId, false);
		CheckBoundingBox(BoundingBox);

		auto Mesh = BeginSectionUpdate(SectionId, UpdateFlags);

		Mesh->SetNumVertices(InVertices0.Num());

		// Copy the mesh data to the mesh builder
		for (int32 Index = 0; Index < InVertices0.Num(); Index++)
		{
			Mesh->SetVertexProperties<VertexType0>(Index, InVertices0[Index]);
			if (InVertices1.Num() > Index)
			{
				Mesh->SetVertexProperties<VertexType1>(Index, InVertices1[Index]);
			}
			if (InVertices2.Num() > Index)
			{
				Mesh->SetVertexProperties<VertexType2>(Index, InVertices2[Index]);
			}
		}

		Mesh->Commit(BoundingBox);
	}

	template<typename VertexType0, typename VertexType1, typename VertexType2, typename IndexType>
	void UpdateMeshSectionTripleBuffer(int32 SectionId, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1, TArray<VertexType2>& InVertices2,
		TArray<IndexType>& InTriangles, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSectionTripleBuffer);

		FRuntimeMeshScopeLock Lock(SyncRoot);

		CheckUpdateLegacy<VertexType0, VertexType1, VertexType2, IndexType>(SectionId, true);

		auto Mesh = BeginSectionUpdate(SectionId, UpdateFlags);

		Mesh->SetNumVertices(InVertices0.Num());

		// Copy the mesh data to the mesh builder
		for (int32 Index = 0; Index < InVertices0.Num(); Index++)
		{
			Mesh->SetVertexProperties<VertexType0>(Index, InVertices0[Index]);
			if (InVertices1.Num() > Index)
			{
				Mesh->SetVertexProperties<VertexType1>(Index, InVertices1[Index]);
			}
			if (InVertices2.Num() > Index)
			{
				Mesh->SetVertexProperties<VertexType2>(Index, InVertices2[Index]);
			}
		}

		Mesh->SetNumIndices(InTriangles.Num());

		for (int32 Index = 0; Index < InTriangles.Num(); Index++)
		{
			Mesh->SetIndex(Index, InTriangles[Index]);
		}

		Mesh->Commit();
	}

	template<typename VertexType0, typename VertexType1, typename VertexType2, typename IndexType>
	void UpdateMeshSectionTripleBuffer(int32 SectionId, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1, TArray<VertexType2>& InVertices2,
		TArray<IndexType>& InTriangles, const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSectionTripleBuffer_BoundingBox);

		FRuntimeMeshScopeLock Lock(SyncRoot);

		CheckUpdateLegacy<VertexType0, VertexType1, VertexType2, IndexType>(SectionId, true);
		CheckBoundingBox(BoundingBox);

		auto Mesh = BeginSectionUpdate(SectionId, UpdateFlags);

		Mesh->SetNumVertices(InVertices0.Num());

		// Copy the mesh data to the mesh builder
		for (int32 Index = 0; Index < InVertices0.Num(); Index++)
		{
			Mesh->SetVertexProperties<VertexType0>(Index, InVertices0[Index]);
			if (InVertices1.Num() > Index)
			{
				Mesh->SetVertexProperties<VertexType1>(Index, InVertices1[Index]);
			}
			if (InVertices2.Num() > Index)
			{
				Mesh->SetVertexProperties<VertexType2>(Index, InVertices2[Index]);
			}
		}

		Mesh->SetNumIndices(InTriangles.Num());

		for (int32 Index = 0; Index < InTriangles.Num(); Index++)
		{
			Mesh->SetIndex(Index, InTriangles[Index]);
		}

		Mesh->Commit(BoundingBox);
	}
	


	template<typename VertexType0>
	void UpdateMeshSectionPrimaryBuffer(int32 SectionId, TArray<VertexType0>& InVertices0, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSectionPrimaryBuffer);

		FRuntimeMeshScopeLock Lock(SyncRoot);

		CheckUpdateLegacy<VertexType0, FRuntimeMeshNullVertex, FRuntimeMeshNullVertex, uint16>(SectionId, false);

		auto Mesh = BeginSectionUpdate(SectionId, UpdateFlags);

		Mesh->SetNumVertices(InVertices0.Num());

		// Copy the mesh data to the mesh builder
		for (int32 Index = 0; Index < InVertices0.Num(); Index++)
		{
			Mesh->SetVertexProperties<VertexType0>(Index, InVertices0[Index]);
		}

		Mesh->Commit();
	}

	template<typename VertexType0>
	void UpdateMeshSectionPrimaryBuffer(int32 SectionId, TArray<VertexType0>& InVertices0, const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSectionPrimaryBuffer_BoundingBox);

		FRuntimeMeshScopeLock Lock(SyncRoot);

		CheckUpdateLegacy<VertexType0, FRuntimeMeshNullVertex, FRuntimeMeshNullVertex, uint16>(SectionId, false);
		CheckBoundingBox(BoundingBox);

		auto Mesh = BeginSectionUpdate(SectionId, UpdateFlags);

		Mesh->SetNumVertices(InVertices0.Num());

		// Copy the mesh data to the mesh builder
		for (int32 Index = 0; Index < InVertices0.Num(); Index++)
		{
			Mesh->SetVertexProperties<VertexType0>(Index, InVertices0[Index]);
		}

		Mesh->Commit(BoundingBox);
	}

	template<typename VertexType1>
	void UpdateMeshSectionSecondaryBuffer(int32 SectionId, TArray<VertexType1>& InVertices1, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSectionSecondaryBuffer);

		FRuntimeMeshScopeLock Lock(SyncRoot);

		CheckUpdateLegacy<FRuntimeMeshNullVertex, VertexType1, FRuntimeMeshNullVertex, uint16>(SectionId, false);

		auto Mesh = BeginSectionUpdate(SectionId, UpdateFlags);
		
		int32 NumVerts = FMath::Min(Mesh->NumVertices(), InVertices1.Num());

		// Copy the mesh data to the mesh builder
		for (int32 Index = 0; Index < NumVerts; Index++)
		{
			Mesh->SetVertexProperties<VertexType1>(Index, InVertices1[Index]);
		}

		Mesh->Commit();
	}

	template<typename VertexType2>
	void UpdateMeshSectionTertiaryBuffer(int32 SectionId, TArray<VertexType2>& InVertices2, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSectionTertiaryBuffer);

		FRuntimeMeshScopeLock Lock(SyncRoot);

		CheckUpdateLegacy<FRuntimeMeshNullVertex, FRuntimeMeshNullVertex, VertexType2, uint16>(SectionId, false);

		auto Mesh = BeginSectionUpdate(SectionId, UpdateFlags);

		int32 NumVerts = FMath::Min(Mesh->NumVertices(), InVertices2.Num());

		// Copy the mesh data to the mesh builder
		for (int32 Index = 0; Index < NumVerts; Index++)
		{
			Mesh->SetVertexProperties<VertexType2>(Index, InVertices2[Index]);
		}

		Mesh->Commit();
	}

	template<typename IndexType>
	void UpdateMeshSectionTriangles(int32 SectionId, TArray<IndexType>& InTriangles, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_UpdateMeshSectionTriangles);

		FRuntimeMeshScopeLock Lock(SyncRoot);

		CheckUpdateLegacy<FRuntimeMeshNullVertex, FRuntimeMeshNullVertex, FRuntimeMeshNullVertex, IndexType>(SectionId, true);

		auto Mesh = BeginSectionUpdate(SectionId, UpdateFlags);
		
		Mesh->SetNumIndices(InTriangles.Num());

		for (int32 Index = 0; Index < InTriangles.Num(); Index++)
		{
			Mesh->SetIndex(Index, InTriangles[Index]);
		}

		Mesh->Commit();
	}



	void CreateMeshSection(int32 SectionId, const TSharedPtr<FRuntimeMeshBuilder>& MeshData, bool bCreateCollision = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None);

	void CreateMeshSectionByMove(int32 SectionId, const TSharedPtr<FRuntimeMeshBuilder>& MeshData, bool bCreateCollision = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None);



	void UpdateMeshSection(int32 SectionId, const TSharedPtr<FRuntimeMeshBuilder>& MeshData, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None);

	void UpdateMeshSectionByMove(int32 SectionId, const TSharedPtr<FRuntimeMeshBuilder>& MeshData, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None);





	TUniquePtr<FRuntimeMeshScopedUpdater> BeginSectionUpdate(int32 SectionId, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None);

	TUniquePtr<FRuntimeMeshScopedUpdater> GetSectionReadonly(int32 SectionId);


private:
	void EndSectionUpdate(FRuntimeMeshScopedUpdater* Updater, ERuntimeMeshBuffersToUpdate BuffersToUpdate, const FBox* BoundingBox = nullptr);


private:
	void CreateMeshSectionFromComponents(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
		const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, TFunction<FColor(int32 Index)> ColorAccessor, int32 NumColors, const TArray<FRuntimeMeshTangent>& Tangents,
		bool bCreateCollision, EUpdateFrequency UpdateFrequency, ESectionUpdateFlags UpdateFlags, bool bUseHighPrecisionTangents, bool bUseHighPrecisionUVs, bool bWantsSecondUV);

	void UpdateMeshSectionFromComponents(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
		const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, TFunction<FColor(int32 Index)> ColorAccessor, int32 NumColors, const TArray<FRuntimeMeshTangent>& Tangents, ESectionUpdateFlags UpdateFlags);

public:

	// HORU :)
	void UpdateMeshSectionColors(int32 SectionIndex, TArray<FColor>& Colors, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None);

	void CreateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
		const TArray<FVector2D>& UV0, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents, bool bCreateCollision = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None,
		bool bUseHighPrecisionTangents = false, bool bUseHighPrecisionUVs = true);

	void CreateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
		const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents,
		bool bCreateCollision = false, EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None,
		bool bUseHighPrecisionTangents = false, bool bUseHighPrecisionUVs = true);


	void UpdateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<FVector>& Normals, const TArray<FVector2D>& UV0,
		const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None);

	void UpdateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<FVector>& Normals, const TArray<FVector2D>& UV0,
		const TArray<FVector2D>& UV1, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None);

	void UpdateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
		const TArray<FVector2D>& UV0, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None);

	void UpdateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
		const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None);

	

	void CreateMeshSection_Blueprint(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
		const TArray<FRuntimeMeshTangent>& Tangents, const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FLinearColor>& Colors,
		bool bCreateCollision = false, bool bCalculateNormalTangent = false, bool bShouldCreateHardTangents = false, bool bGenerateTessellationTriangles = false, 
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, bool bUseHighPrecisionTangents = false, bool bUseHighPrecisionUVs = true);
	
	void UpdateMeshSection_Blueprint(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
		const TArray<FRuntimeMeshTangent>& Tangents, const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FLinearColor>& Colors,
		bool bCalculateNormalTangent = false, bool bShouldCreateHardTangents = false, bool bGenerateTessellationTriangles = false);
	
	void CreateMeshSectionPacked_Blueprint(int32 SectionIndex, const TArray<FRuntimeMeshBlueprintVertexSimple>& Vertices, const TArray<int32>& Triangles,
		bool bCreateCollision = false, bool bCalculateNormalTangent = false, bool bShouldCreateHardTangents = false, bool bGenerateTessellationTriangles = false, EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average,
		bool bUseHighPrecisionTangents = false, bool bUseHighPrecisionUVs = true);
		
	void UpdateMeshSectionPacked_Blueprint(int32 SectionIndex, const TArray<FRuntimeMeshBlueprintVertexSimple>& Vertices, const TArray<int32>& Triangles,
		bool bCalculateNormalTangent = false, bool bShouldCreateHardTangents = false, bool bGenerateTessellationTriangles = false);

	



	

	template<typename IndexType>
	void SetSectionTessellationTriangles(int32 SectionId, const TArray<IndexType>& Triangles)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_SetSectionTessellationTriangles);

		FRuntimeMeshScopeLock Lock(SyncRoot);
		CheckUpdate(false, false, 0, FRuntimeMeshIndexTraits<IndexType>::Is32Bit, SectionId, true, false, false);

		FRuntimeMeshSectionPtr Section = MeshSections[SectionId];

		ERuntimeMeshBuffersToUpdate BuffersToUpdate = ERuntimeMeshBuffersToUpdate::None;

		// Update indices if supplied
		if (Triangles.Num() > 0)
		{
			Section->UpdateAdjacencyIndexBuffer(Triangles);
			BuffersToUpdate |= ERuntimeMeshBuffersToUpdate::AdjacencyIndexBuffer;
		}

		// Finalize section update if we have anything to apply
		if (BuffersToUpdate != ERuntimeMeshBuffersToUpdate::None)
		{
			UpdateSectionInternal(SectionId, BuffersToUpdate, ESectionUpdateFlags::None);
		}
	}
	
	/** Clear a section of the procedural mesh. */
	void ClearMeshSection(int32 SectionIndex);

	/** Clear all mesh sections and reset to empty state */
	void ClearAllMeshSections();



	/** Gets the bounding box of a specific section */
	FBox GetSectionBoundingBox(int32 SectionIndex);

	/** Control visibility of a particular section */
	void SetMeshSectionVisible(int32 SectionIndex, bool bNewVisibility);

	/** Returns whether a particular section is currently visible */
	bool IsMeshSectionVisible(int32 SectionIndex) const;


	/** Control whether a particular section casts a shadow */
	void SetMeshSectionCastsShadow(int32 SectionIndex, bool bNewCastsShadow);

	/** Returns whether a particular section is currently casting shadows */
	bool IsMeshSectionCastingShadows(int32 SectionIndex) const;


	/** Control whether a particular section has collision */
	void SetMeshSectionCollisionEnabled(int32 SectionIndex, bool bNewCollisionEnabled);

	/** Returns whether a particular section has collision */
	bool IsMeshSectionCollisionEnabled(int32 SectionIndex);


	/** Returns number of sections currently created for this component */
	int32 GetNumSections() const;

	/** Returns whether a particular section currently exists */
	bool DoesSectionExist(int32 SectionIndex) const;

	/** Returns first available section index */
	int32 GetAvailableSectionIndex() const;

	int32 GetLastSectionIndex() const;


	TArray<int32> GetSectionIds() const;


	void SetMeshCollisionSection(int32 CollisionSectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles);

	void ClearMeshCollisionSection(int32 CollisionSectionIndex);

	void ClearAllMeshCollisionSections();



	int32 AddConvexCollisionSection(TArray<FVector> ConvexVerts);

	void SetConvexCollisionSection(int32 ConvexSectionIndex, TArray<FVector> ConvexVerts);

	void RemoveConvexCollisionSection(int32 ConvexSectionIndex);

	void ClearConvexCollisionSections();

	void SetCollisionConvexMeshes(const TArray<TArray<FVector>>& ConvexMeshes);


	int32 AddCollisionBox(const FRuntimeMeshCollisionBox& NewBox);

	void RemoveCollisionBox(int32 Index);

	void ClearCollisionBoxes();

	void SetCollisionBoxes(const TArray<FRuntimeMeshCollisionBox>& NewBoxes);

	
	int32 AddCollisionSphere(const FRuntimeMeshCollisionSphere& NewSphere);

	void RemoveCollisionSphere(int32 Index);

	void ClearCollisionSpheres();

	void SetCollisionSpheres(const TArray<FRuntimeMeshCollisionSphere>& NewSpheres);

	
	int32 AddCollisionCapsule(const FRuntimeMeshCollisionCapsule& NewCapsule);

	void RemoveCollisionCapsule(int32 Index);

	void ClearCollisionCapsules();

	void SetCollisionCapsules(const TArray<FRuntimeMeshCollisionCapsule>& NewCapsules);


	FBoxSphereBounds GetLocalBounds() const;

private:
	
	FRuntimeMeshSectionPtr GetSection(int32 SectionIndex) { return MeshSections[SectionIndex]; }

	void Setup(TWeakObjectPtr<URuntimeMesh> InParentMeshObject);

	FRuntimeMeshProxyPtr GetRenderProxy() const { return RenderProxy; }


	/* Creates an mesh section of a specified type at the specified index. */
	template<typename TangentType, typename UVType, typename IndexType>
	FRuntimeMeshSectionPtr CreateOrResetSection(int32 SectionId, EUpdateFrequency UpdateFrequency)
	{
		static_assert(FRuntimeMeshIndexTraits<IndexType>::IsValidIndexType, "Indices can only be of type uint16, uint32, or int32");

		bool bHighPrecisionUVs;
		int32 NumUVs;
		GetUVVertexProperties<UVType>(bHighPrecisionUVs, NumUVs);

		return CreateOrResetSection(SectionId, GetTangentIsHighPrecision<TangentType>(), bHighPrecisionUVs, NumUVs, FRuntimeMeshIndexTraits<IndexType>::Is32Bit, UpdateFrequency);
	}

	FRuntimeMeshSectionPtr CreateOrResetSection(int32 SectionId, bool bInUseHighPrecisionTangents, bool bInUseHighPrecisionUVs, 
		int32 InNumUVs, bool b32BitIndices, EUpdateFrequency UpdateFrequency);

	FRuntimeMeshSectionPtr CreateOrResetSectionForBlueprint(int32 SectionId, bool bWantsSecondUV,
		bool bHighPrecisionTangents, bool bHighPrecisionUVs, EUpdateFrequency UpdateFrequency);

	/* Finishes creating a section, including entering it for batch updating, or updating the RT directly */
	void CreateSectionInternal(int32 SectionIndex, ESectionUpdateFlags UpdateFlags);

	/* Finishes updating a section, including entering it for batch updating, or updating the RT directly */
	void UpdateSectionInternal(int32 SectionIndex, ERuntimeMeshBuffersToUpdate BuffersToUpdate, ESectionUpdateFlags UpdateFlags);

	/* Handles things like automatic tessellation and tangent calculation that is common to both section creation and update. */
	void HandleCommonSectionUpdateFlags(int32 SectionIndex, ESectionUpdateFlags UpdateFlags, ERuntimeMeshBuffersToUpdate& BuffersToUpdate);

	/* Finishes updating a sections properties, like visible/casts shadow, a*/
	void UpdateSectionPropertiesInternal(int32 SectionIndex, bool bUpdateRequiresProxyRecreateIfStatic);

	/** Update LocalBounds member from the local box of each section */
	void UpdateLocalBounds();

	FRuntimeMeshProxyPtr EnsureProxyCreated(ERHIFeatureLevel::Type InFeatureLevel);
	
	TSharedPtr<const FRuntimeMeshAccessor> GetReadonlyMeshAccessor(int32 SectionId);

	void Initialize();

	bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const;
	bool GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData);
	void CopyCollisionElementsToBodySetup(UBodySetup* Setup);
	void MarkCollisionDirty(bool bSkipChangedFlag = false);

	void MarkRenderStateDirty();
	void SendSectionPropertiesUpdate(int32 SectionIndex);

	int32 GetSectionFromCollisionFaceIndex(int32 FaceIndex) const;

	int32 GetSectionAndFaceFromCollisionFaceIndex(int32 & FaceIndex) const;


	void DoOnGameThread(FRuntimeMeshGameThreadTaskDelegate Func);

	void MarkChanged();


	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshData& MeshData)
	{
		SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_SerializationOperator);

		FRuntimeMeshScopeLock Lock(MeshData.SyncRoot, false, true);

		Ar << MeshData.MeshSections;

		Ar << MeshData.MeshCollisionSections;
		Ar << MeshData.ConvexCollisionSections;

		Ar << MeshData.CollisionBoxes;
		Ar << MeshData.CollisionSpheres;
		Ar << MeshData.CollisionCapsules;

		// Update all state since we don't know what really changed (or this could be an initial load)
		if (Ar.IsLoading())
		{
			MeshData.UpdateLocalBounds();
		}
		return Ar;
	}

	friend class URuntimeMesh;
	friend class FRuntimeMeshScopedUpdater;
};

using FRuntimeMeshDataRef = TSharedRef<FRuntimeMeshData, ESPMode::ThreadSafe>;
using FRuntimeMeshDataPtr = TSharedPtr<FRuntimeMeshData, ESPMode::ThreadSafe>;