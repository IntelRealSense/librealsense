// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshCore.h"
#include "RuntimeMeshSection.h"
#include "RuntimeMeshData.h"
#include "RuntimeMeshBlueprint.h"
#include "RuntimeMeshCollision.h"
#include "RuntimeMeshBlueprintMeshBuilder.h"
#include "RuntimeMesh.generated.h"

class UBodySetup;
class URuntimeMesh;
class URuntimeMeshComponent;




/*
*	This tick function is used to drive the collision cooker.
*	It is enabled for one frame when we need to update collision.
*	This keeps from cooking on each individual create/update section as the original PMC did
*/
struct FRuntimeMeshCollisionCookTickObject : FTickableGameObject
{
	TWeakObjectPtr<URuntimeMesh> Owner;

	FRuntimeMeshCollisionCookTickObject(TWeakObjectPtr<URuntimeMesh> InOwner) : Owner(InOwner) {}
	virtual void Tick(float DeltaTime);
	virtual bool IsTickable() const;
	virtual bool IsTickableInEditor() const { return false; }
	virtual TStatId GetStatId() const;

	virtual UWorld* GetTickableGameObjectWorld() const;
};


/**
*	Delegate for when the collision was updated.
*/
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRuntimeMeshCollisionUpdatedDelegate);


UCLASS(HideCategories = Object, BlueprintType)
class RUNTIMEMESHCOMPONENT_API URuntimeMesh : public UObject, public IInterface_CollisionDataProvider
{
	GENERATED_UCLASS_BODY()

private:
	/** Reference to the underlying data object */
	FRuntimeMeshDataRef Data;

	/** Materials for this Runtime Mesh */
	UPROPERTY(EditAnywhere, Category = "RuntimeMesh")
	TArray<UMaterialInterface*> Materials;


	/** Do we need to update our collision? */
	bool bCollisionIsDirty;

	/** Object used to tick the collision cooking at the end of the frame */
	TUniquePtr<FRuntimeMeshCollisionCookTickObject> CookTickObject;

	/** All RuntimeMeshComponents linked to this mesh. Used to alert the components of changes */
	TArray<TWeakObjectPtr<URuntimeMeshComponent>> LinkedComponents;

	/**
	*	Controls whether the complex (Per poly) geometry should be treated as 'simple' collision.
	*	Should be set to false if this component is going to be given simple collision and simulated.
	*/
	UPROPERTY(EditAnywhere, Category = "RuntimeMesh")
	bool bUseComplexAsSimpleCollision;

	/**
	*	Controls whether the physics cooking is done in parallel. This will increase throughput in
	*	multiple RMC scenarios, and keep from blocking the game thread, but when the collision becomes queryable
	*	is non-deterministic. See callback event for notification on collision updated.
	*/
	UPROPERTY(EditAnywhere, Category = "RuntimeMesh")
	bool bUseAsyncCooking;

	/**
	*	Controls whether the mesh data should be serialized with the component.
	*/
	UPROPERTY(EditAnywhere, Category = "RuntimeMesh")
	bool bShouldSerializeMeshData;

	/** Collision cooking configuration. Prefer runtime performance or cooktime speed */
	UPROPERTY(EditAnywhere, Category = "RuntimeMesh")
	ERuntimeMeshCollisionCookingMode CollisionMode;

	/** Collision data */
	UPROPERTY(Instanced)
	UBodySetup* BodySetup;

	/** Queue of pending collision cooks */
	UPROPERTY(Transient)
	TArray<UBodySetup*> AsyncBodySetupQueue;

public:

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	bool ShouldSerializeMeshData() 
	{
		return bShouldSerializeMeshData;			
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetShouldSerializeMeshData(bool bShouldSerialize)
	{
		bShouldSerializeMeshData = bShouldSerialize;
	}

	/** Event called when the collision has finished updated, this works both with standard following frame synchronous updates, as well as async updates */
	UPROPERTY(BlueprintAssignable, Category = "Components|RuntimeMesh")
	FRuntimeMeshCollisionUpdatedDelegate CollisionUpdated;

	/** Gets the internal mesh data */
	FRuntimeMeshDataRef GetRuntimeMeshData() const
	{
		//check(IsInGameThread());
		return Data;
	}

	FRuntimeMeshProxyPtr GetRuntimeMeshRenderProxy() const
	{
		check(IsInGameThread());
		return Data->GetRenderProxy();
	}

public:

	void CreateMeshSection(int32 SectionIndex, bool bWantsHighPrecisionTangents, bool bWantsHighPrecisionUVs, int32 NumUVs, bool bWants32BitIndices, bool bCreateCollision, EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->CreateMeshSection(SectionIndex, bWantsHighPrecisionTangents, bWantsHighPrecisionUVs, NumUVs, bWants32BitIndices, bCreateCollision, UpdateFrequency);
	}

	template<typename VertexType0, typename IndexType>
	FORCEINLINE void CreateMeshSection(int32 SectionIndex, TArray<VertexType0>& InVertices0, TArray<IndexType>& InTriangles, bool bCreateCollision = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->CreateMeshSection<VertexType0, IndexType>(SectionIndex, InVertices0, InTriangles, bCreateCollision, UpdateFrequency, UpdateFlags);
	}

	template<typename VertexType0, typename IndexType>
	FORCEINLINE void CreateMeshSection(int32 SectionIndex, TArray<VertexType0>& InVertices0, TArray<IndexType>& InTriangles, const FBox& BoundingBox,
		bool bCreateCollision = false, EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->CreateMeshSection<VertexType0, IndexType>(SectionIndex, InVertices0, InTriangles, BoundingBox, bCreateCollision, UpdateFrequency, UpdateFlags);
	}

	template<typename VertexType0, typename VertexType1, typename IndexType>
	FORCEINLINE void CreateMeshSectionDualBuffer(int32 SectionIndex, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1, TArray<IndexType>& InTriangles, bool bCreateCollision = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->CreateMeshSectionDualBuffer<VertexType0, VertexType1, IndexType>(SectionIndex, InVertices0,
			InVertices1, InTriangles, bCreateCollision, UpdateFrequency, UpdateFlags);
	}

	template<typename VertexType0, typename VertexType1, typename IndexType>
	FORCEINLINE void CreateMeshSectionDualBuffer(int32 SectionIndex, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1, TArray<IndexType>& InTriangles, const FBox& BoundingBox,
		bool bCreateCollision = false, EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->CreateMeshSectionDualBuffer<VertexType0, VertexType1, IndexType>(SectionIndex, InVertices0,
			InVertices1, InTriangles, BoundingBox, bCreateCollision, UpdateFrequency, UpdateFlags);
	}

	template<typename VertexType0, typename VertexType1, typename VertexType2, typename IndexType>
	FORCEINLINE void CreateMeshSectionTripleBuffer(int32 SectionIndex, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1, TArray<VertexType2>& InVertices2, TArray<IndexType>& InTriangles,
		bool bCreateCollision = false, EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->CreateMeshSectionTripleBuffer<VertexType0, VertexType1, VertexType2, IndexType>(SectionIndex,
			InVertices0, InVertices1, InVertices2, InTriangles, bCreateCollision, UpdateFrequency, UpdateFlags);
	}

	template<typename VertexType0, typename VertexType1, typename VertexType2, typename IndexType>
	FORCEINLINE void CreateMeshSectionTripleBuffer(int32 SectionIndex, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1, TArray<VertexType2>& InVertices2, TArray<IndexType>& InTriangles,
		const FBox& BoundingBox, bool bCreateCollision = false, EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->CreateMeshSectionTripleBuffer<VertexType0, VertexType1, VertexType2, IndexType>(SectionIndex,
			InVertices0, InVertices1, InVertices2, InTriangles, BoundingBox, bCreateCollision, UpdateFrequency, UpdateFlags);
	}



	template<typename VertexType0>
	FORCEINLINE void UpdateMeshSection(int32 SectionId, TArray<VertexType0>& InVertices0,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->UpdateMeshSection<VertexType0>(SectionId, InVertices0, UpdateFlags);
	}

	template<typename VertexType0>
	FORCEINLINE void UpdateMeshSection(int32 SectionId, TArray<VertexType0>& InVertices0,
		const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->UpdateMeshSection<VertexType0>(SectionId, InVertices0, BoundingBox, UpdateFlags);
	}

	template<typename VertexType0, typename IndexType>
	FORCEINLINE void UpdateMeshSection(int32 SectionId, TArray<VertexType0>& InVertices0, TArray<IndexType>& InTriangles,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->UpdateMeshSection<VertexType0, IndexType>(SectionId, InVertices0, InTriangles, UpdateFlags);
	}

	template<typename VertexType0, typename IndexType>
	FORCEINLINE void UpdateMeshSection(int32 SectionId, TArray<VertexType0>& InVertices0, TArray<IndexType>& InTriangles,
		const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->UpdateMeshSection<VertexType0, IndexType>(SectionId, InVertices0, InTriangles, BoundingBox, UpdateFlags);
	}

	template<typename VertexType0, typename VertexType1>
	FORCEINLINE void UpdateMeshSectionDualBuffer(int32 SectionId, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->UpdateMeshSectionDualBuffer<VertexType0, VertexType1>(SectionId, InVertices0, InVertices1, UpdateFlags);
	}

	template<typename VertexType0, typename VertexType1>
	FORCEINLINE void UpdateMeshSectionDualBuffer(int32 SectionId, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1,
		const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->UpdateMeshSectionDualBuffer<VertexType0, VertexType1>(SectionId, InVertices0, InVertices1, BoundingBox, UpdateFlags);
	}

	template<typename VertexType0, typename VertexType1, typename IndexType>
	FORCEINLINE void UpdateMeshSectionDualBuffer(int32 SectionId, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1,
		TArray<IndexType>& InTriangles, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->UpdateMeshSectionDualBuffer<VertexType0, VertexType1, IndexType>(SectionId, InVertices0, InVertices1, InTriangles, UpdateFlags);
	}

	template<typename VertexType0, typename VertexType1, typename IndexType>
	FORCEINLINE void UpdateMeshSectionDualBuffer(int32 SectionId, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1, TArray<IndexType>& InTriangles,
		const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->UpdateMeshSectionDualBuffer<VertexType0, VertexType1, IndexType>(SectionId, InVertices0, InVertices1, InTriangles, BoundingBox, UpdateFlags);
	}

	template<typename VertexType0, typename VertexType1, typename VertexType2>
	FORCEINLINE void UpdateMeshSectionTripleBuffer(int32 SectionId, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1, TArray<VertexType2>& InVertices2,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->UpdateMeshSectionTripleBuffer<VertexType0, VertexType1, VertexType2>(SectionId, InVertices0, InVertices1, InVertices2, UpdateFlags);
	}

	template<typename VertexType0, typename VertexType1, typename VertexType2>
	FORCEINLINE void UpdateMeshSectionTripleBuffer(int32 SectionId, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1, TArray<VertexType2>& InVertices2,
		const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->UpdateMeshSectionTripleBuffer<VertexType0, VertexType1, VertexType2>(SectionId, InVertices0, InVertices1, InVertices2, BoundingBox, UpdateFlags);
	}

	template<typename VertexType0, typename VertexType1, typename VertexType2, typename IndexType>
	FORCEINLINE void UpdateMeshSectionTripleBuffer(int32 SectionId, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1, TArray<VertexType2>& InVertices2,
		TArray<IndexType>& InTriangles, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->UpdateMeshSectionTripleBuffer<VertexType0, VertexType1, VertexType2, IndexType>(SectionId, InVertices0, InVertices1, InVertices2, InTriangles, UpdateFlags);
	}

	template<typename VertexType0, typename VertexType1, typename VertexType2, typename IndexType>
	FORCEINLINE void UpdateMeshSectionTripleBuffer(int32 SectionId, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1, TArray<VertexType2>& InVertices2,
		TArray<IndexType>& InTriangles, const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->UpdateMeshSectionTripleBuffer<VertexType0, VertexType1, VertexType2, IndexType>(SectionId, InVertices0, InVertices1, InVertices2, InTriangles, BoundingBox, UpdateFlags);
	}



	template<typename VertexType0>
	FORCEINLINE void UpdateMeshSectionPrimaryBuffer(int32 SectionId, TArray<VertexType0>& InVertices0, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->UpdateMeshSectionPrimaryBuffer<VertexType0>(SectionId, InVertices0, UpdateFlags);
	}

	template<typename VertexType0>
	FORCEINLINE void UpdateMeshSectionPrimaryBuffer(int32 SectionId, TArray<VertexType0>& InVertices0, const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->UpdateMeshSectionPrimaryBuffer<VertexType0>(SectionId, InVertices0, BoundingBox, UpdateFlags);
	}

	template<typename VertexType1>
	FORCEINLINE void UpdateMeshSectionSecondaryBuffer(int32 SectionId, TArray<VertexType1>& InVertices1, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->UpdateMeshSectionSecondaryBuffer<VertexType1>(SectionId, InVertices1, UpdateFlags);
	}

	template<typename VertexType2>
	FORCEINLINE void UpdateMeshSectionTertiaryBuffer(int32 SectionId, TArray<VertexType2>& InVertices2, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->UpdateMeshSectionTertiaryBuffer<VertexType2>(SectionId, InVertices2, UpdateFlags);
	}

	template<typename IndexType>
	FORCEINLINE void UpdateMeshSectionTriangles(int32 SectionId, TArray<IndexType>& InTriangles, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->UpdateMeshSectionTriangles<IndexType>(SectionId, InTriangles, UpdateFlags);
	}



	FORCEINLINE void CreateMeshSection(int32 SectionId, const TSharedPtr<FRuntimeMeshBuilder>& MeshData, bool bCreateCollision = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->CreateMeshSection(SectionId, MeshData, bCreateCollision, UpdateFrequency, UpdateFlags);
	}

	FORCEINLINE void CreateMeshSectionByMove(int32 SectionId, const TSharedPtr<FRuntimeMeshBuilder>& MeshData, bool bCreateCollision = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->CreateMeshSectionByMove(SectionId, MeshData, bCreateCollision, UpdateFrequency, UpdateFlags);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void CreateMeshSectionFromBuilder(int32 SectionId, URuntimeBlueprintMeshBuilder* MeshData, bool bCreateCollision = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average/*, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None*/)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->CreateMeshSection(SectionId, MeshData->GetMeshBuilder(), bCreateCollision, UpdateFrequency/*, UpdateFlags*/);
	}


	FORCEINLINE void UpdateMeshSection(int32 SectionId, const TSharedPtr<FRuntimeMeshBuilder>& MeshData, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->UpdateMeshSection(SectionId, MeshData, UpdateFlags);
	}

	FORCEINLINE void UpdateMeshSectionByMove(int32 SectionId, const TSharedPtr<FRuntimeMeshBuilder>& MeshData, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->UpdateMeshSectionByMove(SectionId, MeshData, UpdateFlags);
	}
	
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void UpdateMeshSectionFromBuilder(int32 SectionId, URuntimeBlueprintMeshBuilder* MeshData/*, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None*/)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->UpdateMeshSection(SectionId, MeshData->GetMeshBuilder()/*, UpdateFlags*/);
	}


	TUniquePtr<FRuntimeMeshScopedUpdater> BeginSectionUpdate(int32 SectionId, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		return GetRuntimeMeshData()->BeginSectionUpdate(SectionId, UpdateFlags);
	}

	TUniquePtr<FRuntimeMeshScopedUpdater> GetSectionReadonly(int32 SectionId)
	{
		check(IsInGameThread());
		return GetRuntimeMeshData()->GetSectionReadonly(SectionId);
	}




	FORCEINLINE void CreateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents, bool bCreateCollision = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None,
		bool bUseHighPrecisionTangents = false, bool bUseHighPrecisionUVs = true)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->CreateMeshSection(SectionIndex, Vertices, Triangles, Normals, UV0, Colors, Tangents,
			bCreateCollision, UpdateFrequency, UpdateFlags, bUseHighPrecisionTangents, bUseHighPrecisionUVs);
	}

	FORCEINLINE void CreateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents,
		bool bCreateCollision = false, EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None,
		bool bUseHighPrecisionTangents = false, bool bUseHighPrecisionUVs = true)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->CreateMeshSection(SectionIndex, Vertices, Triangles, Normals, UV0, UV1, Colors, Tangents,
			bCreateCollision, UpdateFrequency, UpdateFlags, bUseHighPrecisionTangents, bUseHighPrecisionUVs);
	}


	FORCEINLINE void UpdateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<FVector>& Normals, const TArray<FVector2D>& UV0,
	const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->UpdateMeshSection(SectionIndex, Vertices, Normals, UV0, Colors, Tangents, UpdateFlags);
	}

	FORCEINLINE void UpdateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<FVector>& Normals, const TArray<FVector2D>& UV0,
	const TArray<FVector2D>& UV1, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->UpdateMeshSection(SectionIndex, Vertices, Normals, UV0, UV1, Colors, Tangents, UpdateFlags);
	}

	FORCEINLINE void UpdateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->UpdateMeshSection(SectionIndex, Vertices, Triangles, Normals, UV0, Colors, Tangents, UpdateFlags);
	}

	FORCEINLINE void UpdateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->UpdateMeshSection(SectionIndex, Vertices, Triangles, Normals, UV0, UV1, Colors, Tangents, UpdateFlags);
	}



	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh", meta = (DisplayName = "Create Mesh Section", AutoCreateRefTerm = "Normals,Tangents,UV0,UV1,Colors"))
	void CreateMeshSection_Blueprint(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
		const TArray<FRuntimeMeshTangent>& Tangents, const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FLinearColor>& Colors,
		bool bCreateCollision = false, bool bCalculateNormalTangent = false, bool bShouldCreateHardTangents = false, bool bGenerateTessellationTriangles = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, bool bUseHighPrecisionTangents = false, bool bUseHighPrecisionUVs = true)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->CreateMeshSection_Blueprint(SectionIndex, Vertices, Triangles, Normals, Tangents, UV0, UV1, Colors,
			bCreateCollision, bCalculateNormalTangent, bShouldCreateHardTangents, bGenerateTessellationTriangles, UpdateFrequency, bUseHighPrecisionTangents, bUseHighPrecisionUVs);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh", meta = (DisplayName = "Update Mesh Section", AutoCreateRefTerm = "Triangles,Normals,Tangents,UV0,UV1,Colors"))
	void UpdateMeshSection_Blueprint(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
		const TArray<FRuntimeMeshTangent>& Tangents, const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FLinearColor>& Colors,
		bool bCalculateNormalTangent = false, bool bShouldCreateHardTangents = false, bool bGenerateTessellationTriangles = false)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->UpdateMeshSection_Blueprint(SectionIndex, Vertices, Triangles, Normals, Tangents, UV0, UV1, Colors,
			bCalculateNormalTangent, bShouldCreateHardTangents, bGenerateTessellationTriangles);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh", meta = (DisplayName = "Create Mesh Section Packed", AutoCreateRefTerm = "Normals,Tangents,UV0,UV1,Colors"))
	void CreateMeshSectionPacked_Blueprint(int32 SectionIndex, const TArray<FRuntimeMeshBlueprintVertexSimple>& Vertices, const TArray<int32>& Triangles,
		bool bCreateCollision = false, bool bCalculateNormalTangent = false, bool bShouldCreateHardTangents = false, bool bGenerateTessellationTriangles = false, EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average,
		bool bUseHighPrecisionTangents = false, bool bUseHighPrecisionUVs = true)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->CreateMeshSectionPacked_Blueprint(SectionIndex, Vertices, Triangles, bCreateCollision, bCalculateNormalTangent, bShouldCreateHardTangents,
			bGenerateTessellationTriangles, UpdateFrequency, bUseHighPrecisionTangents, bUseHighPrecisionUVs);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh", meta = (DisplayName = "Update Mesh Section Packed", AutoCreateRefTerm = "Triangles,Normals,Tangents,UV0,UV1,Colors"))
	void UpdateMeshSectionPacked_Blueprint(int32 SectionIndex, const TArray<FRuntimeMeshBlueprintVertexSimple>& Vertices, const TArray<int32>& Triangles,
		bool bCalculateNormalTangent = false, bool bShouldCreateHardTangents = false, bool bGenerateTessellationTriangles = false)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->UpdateMeshSectionPacked_Blueprint(SectionIndex, Vertices, Triangles, bCalculateNormalTangent, bShouldCreateHardTangents, bGenerateTessellationTriangles);
	}


	



	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetSectionTessellationTriangles(int32 SectionId, const TArray<int32>& Triangles)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->SetSectionTessellationTriangles(SectionId, Triangles);
	}

	template<typename IndexType>
	void SetSectionTessellationTriangles(int32 SectionId, const TArray<IndexType>& Triangles)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->SetSectionTessellationTriangles(SectionId, Triangles);
	}



	/** Clear a section of the procedural mesh. */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearMeshSection(int32 SectionIndex)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->ClearMeshSection(SectionIndex);
	}

	/** Clear all mesh sections and reset to empty state */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearAllMeshSections()
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->ClearAllMeshSections();
	}


	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetSectionMaterial(int32 SectionId, UMaterialInterface* Material)
	{
		check(IsInGameThread());
		if (SectionId >= Materials.Num())
		{
			Materials.SetNum(SectionId + 1);
		}

		Materials[SectionId] = Material;

		if (DoesSectionExist(SectionId))
		{
			ForceProxyRecreate();
		}
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	UMaterialInterface* GetSectionMaterial(int32 SectionId)
	{
		check(IsInGameThread());
		if (Materials.IsValidIndex(SectionId))
		{
			return Materials[SectionId];
		}
		return nullptr;
	}

	TArray<UMaterialInterface*> GetMaterials()
	{
		check(IsInGameThread());
		return Materials;
	}

	/** Gets the bounding box of a specific section */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	FBox GetSectionBoundingBox(int32 SectionIndex)
	{
		check(IsInGameThread());
		return GetRuntimeMeshData()->GetSectionBoundingBox(SectionIndex);
	}

	/** Control visibility of a particular section */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetMeshSectionVisible(int32 SectionIndex, bool bNewVisibility)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->SetMeshSectionVisible(SectionIndex, bNewVisibility);
	}

	/** Returns whether a particular section is currently visible */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	bool IsMeshSectionVisible(int32 SectionIndex) const
	{
		check(IsInGameThread());
		return GetRuntimeMeshData()->IsMeshSectionVisible(SectionIndex);
	}


	/** Control whether a particular section casts a shadow */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetMeshSectionCastsShadow(int32 SectionIndex, bool bNewCastsShadow)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->SetMeshSectionCastsShadow(SectionIndex, bNewCastsShadow);
	}

	/** Returns whether a particular section is currently casting shadows */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	bool IsMeshSectionCastingShadows(int32 SectionIndex) const
	{
		check(IsInGameThread());
		return GetRuntimeMeshData()->IsMeshSectionCastingShadows(SectionIndex);
	}


	/** Control whether a particular section has collision */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetMeshSectionCollisionEnabled(int32 SectionIndex, bool bNewCollisionEnabled)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->SetMeshSectionCollisionEnabled(SectionIndex, bNewCollisionEnabled);
	}

	/** Returns whether a particular section has collision */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	bool IsMeshSectionCollisionEnabled(int32 SectionIndex)
	{
		check(IsInGameThread());
		return GetRuntimeMeshData()->IsMeshSectionCollisionEnabled(SectionIndex);
	}


	/** Returns number of sections currently created for this component */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	int32 GetNumSections() const
	{
		check(IsInGameThread());
		return GetRuntimeMeshData()->GetNumSections();
	}

	/** Returns whether a particular section currently exists */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	bool DoesSectionExist(int32 SectionIndex) const
	{
		check(IsInGameThread());
		return GetRuntimeMeshData()->DoesSectionExist(SectionIndex);
	}

	/** Returns first available section index */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	int32 GetAvailableSectionIndex() const
	{
		check(IsInGameThread());
		return GetRuntimeMeshData()->GetAvailableSectionIndex();
	}

	TArray<int32> GetSectionIds() const
	{
		check(IsInGameThread());
		return GetRuntimeMeshData()->GetSectionIds();
	}




	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetMeshCollisionSection(int32 CollisionSectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->SetMeshCollisionSection(CollisionSectionIndex, Vertices, Triangles);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearMeshCollisionSection(int32 CollisionSectionIndex)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->ClearMeshCollisionSection(CollisionSectionIndex);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearAllMeshCollisionSections()
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->ClearAllMeshCollisionSections();
	}



	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	int32 AddConvexCollisionSection(TArray<FVector> ConvexVerts)
	{
		check(IsInGameThread());
		return GetRuntimeMeshData()->AddConvexCollisionSection(ConvexVerts);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetConvexCollisionSection(int32 ConvexSectionIndex, TArray<FVector> ConvexVerts)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->SetConvexCollisionSection(ConvexSectionIndex, ConvexVerts);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearConvexCollisionSection(int32 ConvexSectionIndex)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->RemoveConvexCollisionSection(ConvexSectionIndex);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearAllConvexCollisionSections()
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->ClearConvexCollisionSections();
	}

	void SetCollisionConvexMeshes(const TArray<TArray<FVector>>& ConvexMeshes)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->SetCollisionConvexMeshes(ConvexMeshes);
	}



	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	int32 AddCollisionBox(const FRuntimeMeshCollisionBox& NewBox)
	{
		check(IsInGameThread());
		return GetRuntimeMeshData()->AddCollisionBox(NewBox);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void RemoveCollisionBox(int32 Index)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->RemoveCollisionBox(Index);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearCollisionBoxes()
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->ClearCollisionBoxes();
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetCollisionBoxes(const TArray<FRuntimeMeshCollisionBox>& NewBoxes)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->SetCollisionBoxes(NewBoxes);
	}


	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	int32 AddCollisionSphere(const FRuntimeMeshCollisionSphere& NewSphere)
	{
		check(IsInGameThread());
		return GetRuntimeMeshData()->AddCollisionSphere(NewSphere);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void RemoveCollisionSphere(int32 Index)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->RemoveCollisionSphere(Index);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearCollisionSpheres()
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->ClearCollisionSpheres();
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetCollisionSpheres(const TArray<FRuntimeMeshCollisionSphere>& NewSpheres)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->SetCollisionSpheres(NewSpheres);
	}

	
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	int32 AddCollisionCapsule(const FRuntimeMeshCollisionCapsule& NewCapsule)
	{
		check(IsInGameThread());
		return GetRuntimeMeshData()->AddCollisionCapsule(NewCapsule);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void RemoveCollisionCapsule(int32 Index)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->RemoveCollisionCapsule(Index);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearCollisionCapsules()
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->ClearCollisionCapsules();
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetCollisionCapsules(const TArray<FRuntimeMeshCollisionCapsule>& NewCapsules)
	{
		check(IsInGameThread());
		GetRuntimeMeshData()->SetCollisionCapsules(NewCapsules);
	}


	/** Runs any pending collision cook (Not required to call this. This is only if you need to make sure all changes are cooked before doing something)*/
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void CookCollisionNow();

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetCollisionUseComplexAsSimple(bool bNewValue)
	{
		check(IsInGameThread());
		bUseComplexAsSimpleCollision = bNewValue;
		MarkCollisionDirty();
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	bool IsCollisionUsingComplexAsSimple()
	{
		check(IsInGameThread());
		return bUseComplexAsSimpleCollision;
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetCollisionUseAsyncCooking(bool bNewValue)
	{
		check(IsInGameThread());
		bUseAsyncCooking = bNewValue;
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	bool IsCollisionUsingAsyncCooking()
	{
		check(IsInGameThread());
		return bUseAsyncCooking;
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetCollisionMode(ERuntimeMeshCollisionCookingMode NewMode)
	{
		check(IsInGameThread());
		CollisionMode = NewMode;
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	ERuntimeMeshCollisionCookingMode GetCollisionMode() const
	{
		check(IsInGameThread());
		return CollisionMode;
	}

	FBoxSphereBounds GetLocalBounds() const 
	{
		//check(IsInGameThread()); 
		return GetRuntimeMeshData()->GetLocalBounds();
	}

	UBodySetup* GetBodySetup() 
	{ 
		check(IsInGameThread());
		return BodySetup; 
	}

private:

	void Initialize() { GetRuntimeMeshData()->Initialize(); }
	virtual void MarkChanged() 
	{ 
#if WITH_EDITOR
		Modify(true);
		PostEditChange();
#endif
	}

	void RegisterLinkedComponent(URuntimeMeshComponent* NewComponent);
	void UnRegisterLinkedComponent(URuntimeMeshComponent* ComponentToRemove);

	template<typename Function>
	void DoForAllLinkedComponents(Function Func)
	{
		bool bShouldPurge = false;
		for (TWeakObjectPtr<URuntimeMeshComponent> MeshReference : LinkedComponents)
		{
			if (URuntimeMeshComponent* Mesh = MeshReference.Get())
			{
				Func(Mesh);
			}
			else
			{
				bShouldPurge = true;
			}
		}
		if (bShouldPurge)
		{
			LinkedComponents = LinkedComponents.FilterByPredicate([](const TWeakObjectPtr<URuntimeMeshComponent>& MeshReference)
			{
				return MeshReference.IsValid();
			});
		}
	}


	void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials) const
	{
		OutMaterials.Append(Materials.FilterByPredicate([](UMaterialInterface* Mat) -> bool { return Mat != nullptr; }));
	}

	//~ Begin Interface_CollisionDataProvider Interface
	virtual bool GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData) override;
	virtual bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const override;
	virtual bool WantsNegXTriMesh() override { return false; }
	//~ End Interface_CollisionDataProvider Interface

	virtual void Serialize(FArchive& Ar) override;
	void PostLoad();


public:
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	UMaterialInterface* GetMaterialFromCollisionFaceIndex(int32 FaceIndex, int32& SectionIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	int32 GetSectionIdFromCollisionFaceIndex(int32 FaceIndex) const;


private:
	/** Triggers a rebuild of the collision data on the next tick */
	void MarkCollisionDirty();

#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 21
	/** Helper to create new body setup objects */
	UBodySetup* CreateNewBodySetup();
#endif
	/** Copies the convex element geometry to a supplied body setup */
	void CopyCollisionElementsToBodySetup(UBodySetup* Setup);
	/** Sets all basic configuration on body setup */
	void SetBasicBodySetupParameters(UBodySetup* Setup);
	/** Mark collision data as dirty, and re-create on instance if necessary */
	void UpdateCollision(bool bForceCookNow = false);
	/** Once async physics cook is done, create needed state, and then call the user event */
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 21
	void FinishPhysicsAsyncCook(bool bSuccess, UBodySetup* FinishedBodySetup);
	/** Runs all post cook tasks like alerting the user event and alerting linked components */
	void FinalizeNewCookedData();
#endif


	FRuntimeMeshProxyPtr EnsureProxyCreated(ERHIFeatureLevel::Type InFeatureLevel)
	{
		return GetRuntimeMeshData()->EnsureProxyCreated(InFeatureLevel);
	}


	void UpdateLocalBounds();

	void ForceProxyRecreate();

	void SendSectionCreation(int32 SectionIndex);

	void SendSectionPropertiesUpdate(int32 SectionIndex);


	friend class FRuntimeMeshData;
	friend class URuntimeMeshComponent;
	friend class FRuntimeMeshComponentSceneProxy;
	friend struct FRuntimeMeshCollisionCookTickObject;
};

