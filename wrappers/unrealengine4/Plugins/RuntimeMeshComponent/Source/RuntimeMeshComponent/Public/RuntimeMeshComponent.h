// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "Components/MeshComponent.h"
#include "RuntimeMeshCore.h"
#include "RuntimeMeshSection.h"
#include "RuntimeMeshGenericVertex.h"
#include "PhysicsEngine/ConvexElem.h"
#include "RuntimeMesh.h"
#include "Interfaces/Interface_CollisionDataProvider.h"
#include "RuntimeMeshComponent.generated.h"

UENUM(BlueprintType)		//"BlueprintType" is essential to include
enum class ERuntimeMeshSetAction : uint8
{
	Create 	UMETA(DisplayName = "Created new section"),
	Update 	UMETA(DisplayName = "Updated section"),
	Remove	UMETA(DisplayName = "Removed section"),
	None	UMETA(DisplayName = "Did nothing")
};
/**
*	Component that allows you to specify custom triangle mesh geometry for rendering and collision.
*/
UCLASS(HideCategories = (Object, LOD), Meta = (BlueprintSpawnableComponent))
class RUNTIMEMESHCOMPONENT_API URuntimeMeshComponent : public UMeshComponent, public IInterface_CollisionDataProvider
{
	GENERATED_BODY()

private:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = RuntimeMesh, Meta = (AllowPrivateAccess = "true", DisplayName = "Runtime Mesh"))
	URuntimeMesh* RuntimeMeshReference;

	void EnsureHasRuntimeMesh();




public:

	URuntimeMeshComponent(const FObjectInitializer& ObjectInitializer);

	/** Clears the geometry for ALL collision only sections */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	FORCEINLINE URuntimeMesh* GetRuntimeMesh() const
	{
		return RuntimeMeshReference;
	}

	/** Clears the geometry for ALL collision only sections */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	FORCEINLINE URuntimeMesh* GetOrCreateRuntimeMesh()
	{
		EnsureHasRuntimeMesh();

		return RuntimeMeshReference;
	}

	FORCEINLINE FRuntimeMeshDataRef GetRuntimeMeshData()
	{
		return GetRuntimeMesh() ? GetRuntimeMesh()->GetRuntimeMeshData() : FRuntimeMeshDataRef();
	}

	FORCEINLINE FRuntimeMeshDataRef GetOrCreateRuntimeMeshData()
	{
		return GetOrCreateRuntimeMesh()->GetRuntimeMeshData();
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	bool ShouldSerializeMeshData()
	{
		return GetRuntimeMesh() ? GetRuntimeMesh()->ShouldSerializeMeshData() : false;
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetShouldSerializeMeshData(bool bShouldSerialize)
	{
		GetOrCreateRuntimeMesh()->SetShouldSerializeMeshData(bShouldSerialize);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh", Meta = (AllowPrivateAccess = "true", DisplayName = "Get Mobility"))
		ERuntimeMeshMobility GetRuntimeMeshMobility()
	{
		return Mobility == EComponentMobility::Movable ? ERuntimeMeshMobility::Movable :
			Mobility == EComponentMobility::Stationary ? ERuntimeMeshMobility::Stationary : ERuntimeMeshMobility::Static;
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh", Meta = (AllowPrivateAccess = "true", DisplayName = "Set Mobility"))
		void SetRuntimeMeshMobility(ERuntimeMeshMobility NewMobility)
	{
		Super::SetMobility(
			NewMobility == ERuntimeMeshMobility::Movable ? EComponentMobility::Movable :
			NewMobility == ERuntimeMeshMobility::Stationary ? EComponentMobility::Stationary : EComponentMobility::Static);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetRuntimeMesh(URuntimeMesh* NewMesh);


	void CreateMeshSection(int32 SectionIndex, bool bWantsHighPrecisionTangents, bool bWantsHighPrecisionUVs, int32 NumUVs, bool bWants32BitIndices, bool bCreateCollision, EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average)
	{
		GetOrCreateRuntimeMesh()->CreateMeshSection(SectionIndex, bWantsHighPrecisionTangents, bWantsHighPrecisionUVs, NumUVs, bWants32BitIndices, bCreateCollision, UpdateFrequency);
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/*
	 * Creates the mesh section if it doesn't exist,
	 * Otherwise update the section.
	 * Will automatically delete the section if there are no vertices given
	 */
	template<typename VertexType0, typename IndexType>
	ERuntimeMeshSetAction SetMeshSection(int32 SectionIndex, TArray<VertexType0>& InVertices0, TArray<IndexType>& InTriangles, bool bCreateCollision = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		if (GetOrCreateRuntimeMeshData()->DoesSectionExist(SectionIndex)) {
			if (InVertices0.Num() == 0) {
				ClearMeshSection(SectionIndex);
				return ERuntimeMeshSetAction::Remove;
			}
			else {
				UpdateMeshSection(SectionIndex, InVertices0, InTriangles, UpdateFlags);
				return ERuntimeMeshSetAction::Update;
			}
		}
		else if (InVertices0.Num() != 0) {
			CreateMeshSection(SectionIndex, InVertices0, InTriangles, bCreateCollision, UpdateFrequency, UpdateFlags);
			return ERuntimeMeshSetAction::Create;
		}
		return ERuntimeMeshSetAction::None;
	}

	/*
	 * Creates the mesh section if it doesn't exist,
	 * Otherwise update the section.
	 * Will automatically delete the section if there are no vertices given
	 */
	ERuntimeMeshSetAction SetMeshSection(int32 SectionId, const TSharedPtr<FRuntimeMeshBuilder>& MeshData, bool bCreateCollision = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		if (GetOrCreateRuntimeMeshData()->DoesSectionExist(SectionId)) {
			if (MeshData->NumIndices() == 0) {
				GetOrCreateRuntimeMeshData()->ClearMeshSection(SectionId);
				return ERuntimeMeshSetAction::Remove;
			}
			else {
				GetOrCreateRuntimeMeshData()->UpdateMeshSection(SectionId, MeshData, UpdateFlags);
				return ERuntimeMeshSetAction::Update;
			}
		}
		else if (MeshData->NumIndices() != 0) {
			GetOrCreateRuntimeMeshData()->CreateMeshSection(SectionId, MeshData, bCreateCollision, UpdateFrequency, UpdateFlags);
			return ERuntimeMeshSetAction::Create;
		}
		return ERuntimeMeshSetAction::None;
	}

	/*
	 * Creates the mesh section if it doesn't exist,
	 * Otherwise update the section.
	 * Will automatically delete the section if there are no vertices given
	 */
	ERuntimeMeshSetAction SetMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
		const TArray<FVector2D>& UV0, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents, bool bCreateCollision = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None,
		bool bUseHighPrecisionTangents = false, bool bUseHighPrecisionUVs = true)
	{
		if (GetOrCreateRuntimeMeshData()->DoesSectionExist(SectionIndex)) {
			if (Vertices.Num() == 0) {
				GetOrCreateRuntimeMeshData()->ClearMeshSection(SectionIndex);
				return ERuntimeMeshSetAction::Remove;
			}
			else {
				UpdateMeshSection(SectionIndex, Vertices, Triangles, Normals, UV0, Colors, Tangents, UpdateFlags);
				return ERuntimeMeshSetAction::Update;
			}
		}
		else if (Vertices.Num() != 0) {
			CreateMeshSection(SectionIndex, Vertices, Triangles, Normals, UV0, Colors, Tangents, bCreateCollision,
				UpdateFrequency, UpdateFlags, bUseHighPrecisionTangents, bUseHighPrecisionUVs);
			return ERuntimeMeshSetAction::Create;
		}
		return ERuntimeMeshSetAction::None;
	}

	/*
	 * Creates the mesh section if it doesn't exist,
	 * Otherwise update the section.
	 * Will automatically delete the section if there are no vertices given
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh", meta = (DisplayName = "Set Mesh Section", AutoCreateRefTerm = "Normals,Tangents,UV0,UV1,Colors"))
		ERuntimeMeshSetAction SetMeshSection_Blueprint(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
			const TArray<FRuntimeMeshTangent>& Tangents, const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FLinearColor>& Colors,
			bool bCreateCollision = false, bool bCalculateNormalTangent = false, bool bShouldCreateHardTangents = false, bool bGenerateTessellationTriangles = false,
			EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, bool bUseHighPrecisionTangents = false, bool bUseHighPrecisionUVs = true)
	{
		if (DoesSectionExist(SectionIndex)) {
			if (Vertices.Num() == 0) {
				ClearMeshSection(SectionIndex);
				return ERuntimeMeshSetAction::Remove;
			}
			else {
				UpdateMeshSection_Blueprint(SectionIndex, Vertices, Triangles, Normals, Tangents, UV0, UV1, Colors,
					bCalculateNormalTangent, bShouldCreateHardTangents, bGenerateTessellationTriangles);
				return ERuntimeMeshSetAction::Update;
			}
		}
		else if (Vertices.Num() != 0) {
			CreateMeshSection_Blueprint(SectionIndex, Vertices, Triangles, Normals, Tangents, UV0, UV1, Colors, bCreateCollision,
				bCalculateNormalTangent, bShouldCreateHardTangents, bGenerateTessellationTriangles, UpdateFrequency, bUseHighPrecisionTangents, bUseHighPrecisionUVs);
			return ERuntimeMeshSetAction::Create;
		}
		return ERuntimeMeshSetAction::None;
	}

	/*
	 * Creates the mesh section if it doesn't exist,
	 * Otherwise update the section.
	 * Will automatically delete the section if there are no vertices given
	 */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh", meta = (DisplayName = "Set Mesh Section Packed", AutoCreateRefTerm = "Normals,Tangents,UV0,UV1,Colors"))
		ERuntimeMeshSetAction SetMeshSectionPacked_Blueprint(int32 SectionIndex, const TArray<FRuntimeMeshBlueprintVertexSimple>& Vertices, const TArray<int32>& Triangles,
			bool bCreateCollision = false, bool bCalculateNormalTangent = false, bool bShouldCreateHardTangents = false, bool bGenerateTessellationTriangles = false, EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average,
			bool bUseHighPrecisionTangents = false, bool bUseHighPrecisionUVs = true)
	{
		if (DoesSectionExist(SectionIndex)) {
			if (Vertices.Num() == 0) {
				ClearMeshSection(SectionIndex);
				return ERuntimeMeshSetAction::Remove;
			}
			else {
				UpdateMeshSectionPacked_Blueprint(SectionIndex, Vertices, Triangles, bCalculateNormalTangent, bShouldCreateHardTangents,
					bGenerateTessellationTriangles);
				return ERuntimeMeshSetAction::Update;
			}
		}
		else if (Vertices.Num() != 0) {
			CreateMeshSectionPacked_Blueprint(SectionIndex, Vertices, Triangles, bCreateCollision, bCalculateNormalTangent, bShouldCreateHardTangents,
				bGenerateTessellationTriangles, UpdateFrequency, bUseHighPrecisionTangents, bUseHighPrecisionUVs);
			return ERuntimeMeshSetAction::Create;
		}
		return ERuntimeMeshSetAction::None;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	template<typename VertexType0, typename IndexType>
	FORCEINLINE void CreateMeshSection(int32 SectionIndex, TArray<VertexType0>& InVertices0, TArray<IndexType>& InTriangles, bool bCreateCollision = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->CreateMeshSection(SectionIndex, InVertices0, InTriangles, bCreateCollision, UpdateFrequency, UpdateFlags);
	}

	template<typename VertexType0, typename IndexType>
	FORCEINLINE void CreateMeshSection(int32 SectionIndex, TArray<VertexType0>& InVertices0, TArray<IndexType>& InTriangles, const FBox& BoundingBox,
		bool bCreateCollision = false, EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->CreateMeshSection(SectionIndex, InVertices0, InTriangles, BoundingBox, bCreateCollision, UpdateFrequency, UpdateFlags);
	}

	template<typename VertexType0, typename VertexType1, typename IndexType>
	FORCEINLINE void CreateMeshSectionDualBuffer(int32 SectionIndex, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1, TArray<IndexType>& InTriangles, bool bCreateCollision = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->CreateMeshSectionDualBuffer(SectionIndex, InVertices0, InVertices1, InTriangles, bCreateCollision, UpdateFrequency, UpdateFlags);
	}

	template<typename VertexType0, typename VertexType1, typename IndexType>
	FORCEINLINE void CreateMeshSectionDualBuffer(int32 SectionIndex, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1, TArray<IndexType>& InTriangles, const FBox& BoundingBox,
		bool bCreateCollision = false, EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->CreateMeshSectionDualBuffer(SectionIndex, InVertices0, InVertices1, InTriangles, BoundingBox, bCreateCollision, UpdateFrequency, UpdateFlags);
	}

	template<typename VertexType0, typename VertexType1, typename VertexType2, typename IndexType>
	FORCEINLINE void CreateMeshSectionTripleBuffer(int32 SectionIndex, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1, TArray<VertexType2>& InVertices2, TArray<IndexType>& InTriangles,
		bool bCreateCollision = false, EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->CreateMeshSectionTripleBuffer(SectionIndex, InVertices0, InVertices1, InVertices2, InTriangles, bCreateCollision, UpdateFrequency, UpdateFlags);
	}

	template<typename VertexType0, typename VertexType1, typename VertexType2, typename IndexType>
	FORCEINLINE void CreateMeshSectionTripleBuffer(int32 SectionIndex, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1, TArray<VertexType2>& InVertices2, TArray<IndexType>& InTriangles,
		const FBox& BoundingBox, bool bCreateCollision = false, EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->CreateMeshSectionTripleBuffer(SectionIndex, InVertices0, InVertices1, InVertices2, InTriangles, BoundingBox, bCreateCollision, UpdateFrequency, UpdateFlags);
	}







	template<typename VertexType0>
	FORCEINLINE void UpdateMeshSection(int32 SectionId, TArray<VertexType0>& InVertices0,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSection(SectionId, InVertices0, UpdateFlags);
	}

	template<typename VertexType0>
	FORCEINLINE void UpdateMeshSection(int32 SectionId, TArray<VertexType0>& InVertices0,
		const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSection(SectionId, InVertices0, BoundingBox, UpdateFlags);
	}

	template<typename VertexType0, typename IndexType>
	FORCEINLINE void UpdateMeshSection(int32 SectionId, TArray<VertexType0>& InVertices0, TArray<IndexType>& InTriangles,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSection(SectionId, InVertices0, InTriangles, UpdateFlags);
	}

	template<typename VertexType0, typename IndexType>
	FORCEINLINE void UpdateMeshSection(int32 SectionId, TArray<VertexType0>& InVertices0, TArray<IndexType>& InTriangles,
		const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSection(SectionId, InVertices0, InTriangles, BoundingBox, UpdateFlags);
	}

	template<typename VertexType0, typename VertexType1>
	FORCEINLINE void UpdateMeshSectionDualBuffer(int32 SectionId, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionDualBuffer(SectionId, InVertices0, InVertices1, UpdateFlags);
	}

	template<typename VertexType0, typename VertexType1>
	FORCEINLINE void UpdateMeshSectionDualBuffer(int32 SectionId, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1,
		const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionDualBuffer(SectionId, InVertices0, InVertices1, BoundingBox, UpdateFlags);
	}

	template<typename VertexType0, typename VertexType1, typename IndexType>
	FORCEINLINE void UpdateMeshSectionDualBuffer(int32 SectionId, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1,
		TArray<IndexType>& InTriangles, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionDualBuffer(SectionId, InVertices0, InVertices1, InTriangles, UpdateFlags);
	}

	template<typename VertexType0, typename VertexType1, typename IndexType>
	FORCEINLINE void UpdateMeshSectionDualBuffer(int32 SectionId, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1, TArray<IndexType>& InTriangles,
		const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionDualBuffer(SectionId, InVertices0, InVertices1, InTriangles, BoundingBox, UpdateFlags);
	}

	template<typename VertexType0, typename VertexType1, typename VertexType2>
	FORCEINLINE void UpdateMeshSectionTripleBuffer(int32 SectionId, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1, TArray<VertexType2>& InVertices2,
		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionTripleBuffer(SectionId, InVertices0, InVertices1, InVertices2, UpdateFlags);
	}

	template<typename VertexType0, typename VertexType1, typename VertexType2>
	FORCEINLINE void UpdateMeshSectionTripleBuffer(int32 SectionId, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1, TArray<VertexType2>& InVertices2,
		const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionTripleBuffer(SectionId, InVertices0, InVertices1, InVertices2, BoundingBox, UpdateFlags);
	}

	template<typename VertexType0, typename VertexType1, typename VertexType2, typename IndexType>
	FORCEINLINE void UpdateMeshSectionTripleBuffer(int32 SectionId, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1, TArray<VertexType2>& InVertices2,
		TArray<IndexType>& InTriangles, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionTripleBuffer(SectionId, InVertices0, InVertices1, InVertices2, InTriangles, UpdateFlags);
	}

	template<typename VertexType0, typename VertexType1, typename VertexType2, typename IndexType>
	FORCEINLINE void UpdateMeshSectionTripleBuffer(int32 SectionId, TArray<VertexType0>& InVertices0, TArray<VertexType1>& InVertices1, TArray<VertexType2>& InVertices2,
		TArray<IndexType>& InTriangles, const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionTripleBuffer(SectionId, InVertices0, InVertices1, InVertices2, InTriangles, BoundingBox, UpdateFlags);
	}




	/** DEPRECATED! Use UpdateMeshSectionDualBuffer() instead.  Updates the dual buffer mesh section */
	template<typename VertexType>
	UE_DEPRECATED(3.0, "UpdateMeshSection for dual buffer sections deprecated. Please use UpdateMeshSectionDualBuffer instead.")
	void UpdateMeshSection(int32 SectionIndex, TArray<FVector>& VertexPositions, TArray<VertexType>& VertexData, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		UpdateMeshSectionDualBuffer(SectionIndex, VertexPositions, VertexData, UpdateFlags);
	}

	/** DEPRECATED! Use UpdateMeshSectionDualBuffer() instead.  Updates the dual buffer mesh section */
	template<typename VertexType>
	UE_DEPRECATED(3.0, "UpdateMeshSection for dual buffer sections deprecated. Please use UpdateMeshSectionDualBuffer instead.")
	void UpdateMeshSection(int32 SectionIndex, TArray<FVector>& VertexPositions, TArray<VertexType>& VertexData, const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		UpdateMeshSectionDualBuffer(SectionIndex, VertexPositions, VertexData, BoundingBox, UpdateFlags);
	}

	/** DEPRECATED! Use UpdateMeshSectionDualBuffer() instead.  Updates the dual buffer mesh section */
	template<typename VertexType>
	UE_DEPRECATED(3.0, "UpdateMeshSection for dual buffer sections deprecated. Please use UpdateMeshSectionDualBuffer instead.")
	void UpdateMeshSection(int32 SectionIndex, TArray<FVector>& VertexPositions, TArray<VertexType>& VertexData, TArray<int32>& Triangles, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		UpdateMeshSectionDualBuffer(SectionIndex, VertexPositions, VertexData, Triangles, UpdateFlags);
	}

	/** DEPRECATED! Use UpdateMeshSectionDualBuffer() instead.  Updates the dual buffer mesh section */
	template<typename VertexType>
	UE_DEPRECATED(3.0, "UpdateMeshSection for dual buffer sections deprecated. Please use UpdateMeshSectionDualBuffer instead.")
	void UpdateMeshSection(int32 SectionIndex, TArray<FVector>& VertexPositions, TArray<VertexType>& VertexData, TArray<int32>& Triangles, const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		UpdateMeshSectionDualBuffer(SectionIndex, VertexPositions, VertexData, Triangles, BoundingBox, UpdateFlags);
	}





	template<typename VertexType0>
	FORCEINLINE void UpdateMeshSectionPrimaryBuffer(int32 SectionId, TArray<VertexType0>& InVertices0, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionPrimaryBuffer(SectionId, InVertices0, UpdateFlags);
	}

	template<typename VertexType0>
	FORCEINLINE void UpdateMeshSectionPrimaryBuffer(int32 SectionId, TArray<VertexType0>& InVertices0, const FBox& BoundingBox, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionPrimaryBuffer(SectionId, InVertices0, BoundingBox, UpdateFlags);
	}

	template<typename VertexType1>
	FORCEINLINE void UpdateMeshSectionSecondaryBuffer(int32 SectionId, TArray<VertexType1>& InVertices1, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionSecondaryBuffer(SectionId, InVertices1, UpdateFlags);
	}

	template<typename VertexType2>
	FORCEINLINE void UpdateMeshSectionTertiaryBuffer(int32 SectionId, TArray<VertexType2>& InVertices2, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionTertiaryBuffer(SectionId, InVertices2, UpdateFlags);
	}

	template<typename IndexType>
	FORCEINLINE void UpdateMeshSectionTriangles(int32 SectionId, TArray<IndexType>& InTriangles, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionTriangles(SectionId, InTriangles, UpdateFlags);
	}



	FORCEINLINE void CreateMeshSection(int32 SectionId, const TSharedPtr<FRuntimeMeshBuilder>& MeshData, bool bCreateCollision = false,
	EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->CreateMeshSection(SectionId, MeshData, bCreateCollision, UpdateFrequency, UpdateFlags);
	}

	FORCEINLINE void CreateMeshSectionByMove(int32 SectionId, const TSharedPtr<FRuntimeMeshBuilder>& MeshData, bool bCreateCollision = false,
	EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->CreateMeshSectionByMove(SectionId, MeshData, bCreateCollision, UpdateFrequency, UpdateFlags);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void CreateMeshSectionFromBuilder(int32 SectionId, URuntimeBlueprintMeshBuilder* MeshData, bool bCreateCollision = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average/*, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None*/)
	{
		GetOrCreateRuntimeMesh()->CreateMeshSectionFromBuilder(SectionId, MeshData, bCreateCollision, UpdateFrequency/*, UpdateFlags*/);
	}



	FORCEINLINE void UpdateMeshSection(int32 SectionId, const TSharedPtr<FRuntimeMeshBuilder>& MeshData, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSection(SectionId, MeshData, UpdateFlags);
	}

	FORCEINLINE void UpdateMeshSectionByMove(int32 SectionId, const TSharedPtr<FRuntimeMeshBuilder>& MeshData, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionByMove(SectionId, MeshData, UpdateFlags);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void UpdateMeshSectionFromBuilder(int32 SectionId, URuntimeBlueprintMeshBuilder* MeshData/*, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None*/)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionFromBuilder(SectionId, MeshData/*, UpdateFlags*/);
	}

	
	TUniquePtr<FRuntimeMeshScopedUpdater> BeginSectionUpdate(int32 SectionId, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		check(IsInGameThread());
		return GetOrCreateRuntimeMesh()->BeginSectionUpdate(SectionId, UpdateFlags);
	}

	TUniquePtr<FRuntimeMeshScopedUpdater> GetSectionReadonly(int32 SectionId)
	{
		check(IsInGameThread());
		return GetOrCreateRuntimeMesh()->GetSectionReadonly(SectionId);
	}


	
	FORCEINLINE void CreateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents, bool bCreateCollision = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None,
		bool bUseHighPrecisionTangents = false, bool bUseHighPrecisionUVs = true)
	{
		GetOrCreateRuntimeMesh()->CreateMeshSection(SectionIndex, Vertices, Triangles, Normals, UV0, Colors, Tangents, bCreateCollision,
			UpdateFrequency, UpdateFlags, bUseHighPrecisionTangents, bUseHighPrecisionUVs);
	}

	FORCEINLINE void CreateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents,
		bool bCreateCollision = false, EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None,
		bool bUseHighPrecisionTangents = false, bool bUseHighPrecisionUVs = true)
	{
		GetOrCreateRuntimeMesh()->CreateMeshSection(SectionIndex, Vertices, Triangles, Normals, UV0, UV1, Colors, Tangents, bCreateCollision,
			UpdateFrequency, UpdateFlags, bUseHighPrecisionTangents, bUseHighPrecisionUVs);
	}


	FORCEINLINE void UpdateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<FVector>& Normals, const TArray<FVector2D>& UV0,
	const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSection(SectionIndex, Vertices, Normals, UV0, Colors, Tangents, UpdateFlags);
	}

	FORCEINLINE void UpdateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<FVector>& Normals, const TArray<FVector2D>& UV0,
	const TArray<FVector2D>& UV1, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSection(SectionIndex, Vertices, Normals, UV0, UV1, Colors, Tangents, UpdateFlags);
	}

	FORCEINLINE void UpdateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
		const TArray<FVector2D>& UV0, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSection(SectionIndex, Vertices, Triangles, Normals, UV0, Colors, Tangents, UpdateFlags);
	}

	FORCEINLINE void UpdateMeshSection(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
	const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FColor>& Colors, const TArray<FRuntimeMeshTangent>& Tangents, ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSection(SectionIndex, Vertices, Triangles, Normals, UV0, UV1, Colors, Tangents, UpdateFlags);
	}



	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh", meta = (DisplayName = "Create Mesh Section", AutoCreateRefTerm = "Normals,Tangents,UV0,UV1,Colors"))
	void CreateMeshSection_Blueprint(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
		const TArray<FRuntimeMeshTangent>& Tangents, const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FLinearColor>& Colors,
		bool bCreateCollision = false, bool bCalculateNormalTangent = false, bool bShouldCreateHardTangents = false, bool bGenerateTessellationTriangles = false,
		EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average, bool bUseHighPrecisionTangents = false, bool bUseHighPrecisionUVs = true)
	{
		GetOrCreateRuntimeMesh()->CreateMeshSection_Blueprint(SectionIndex, Vertices, Triangles, Normals, Tangents, UV0, UV1, Colors, bCreateCollision,
			bCalculateNormalTangent, bShouldCreateHardTangents, bGenerateTessellationTriangles, UpdateFrequency, bUseHighPrecisionTangents, bUseHighPrecisionUVs);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh", meta = (DisplayName = "Update Mesh Section", AutoCreateRefTerm = "Triangles,Normals,Tangents,UV0,UV1,Colors"))
	void UpdateMeshSection_Blueprint(int32 SectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles, const TArray<FVector>& Normals,
		const TArray<FRuntimeMeshTangent>& Tangents, const TArray<FVector2D>& UV0, const TArray<FVector2D>& UV1, const TArray<FLinearColor>& Colors,
		bool bCalculateNormalTangent = false, bool bShouldCreateHardTangents = false, bool bGenerateTessellationTriangles = false)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSection_Blueprint(SectionIndex, Vertices, Triangles, Normals, Tangents, UV0, UV1, Colors, bCalculateNormalTangent, bShouldCreateHardTangents, bGenerateTessellationTriangles);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh", meta = (DisplayName = "Create Mesh Section Packed", AutoCreateRefTerm = "Normals,Tangents,UV0,UV1,Colors"))
	void CreateMeshSectionPacked_Blueprint(int32 SectionIndex, const TArray<FRuntimeMeshBlueprintVertexSimple>& Vertices, const TArray<int32>& Triangles,
		bool bCreateCollision = false, bool bCalculateNormalTangent = false, bool bShouldCreateHardTangents = false, bool bGenerateTessellationTriangles = false, EUpdateFrequency UpdateFrequency = EUpdateFrequency::Average,
		bool bUseHighPrecisionTangents = false, bool bUseHighPrecisionUVs = true)
	{
		GetOrCreateRuntimeMesh()->CreateMeshSectionPacked_Blueprint(SectionIndex, Vertices, Triangles, bCreateCollision, bCalculateNormalTangent, bShouldCreateHardTangents,
			bGenerateTessellationTriangles, UpdateFrequency, bUseHighPrecisionTangents, bUseHighPrecisionUVs);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh", meta = (DisplayName = "Update Mesh Section Packed", AutoCreateRefTerm = "Triangles,Normals,Tangents,UV0,UV1,Colors"))
	void UpdateMeshSectionPacked_Blueprint(int32 SectionIndex, const TArray<FRuntimeMeshBlueprintVertexSimple>& Vertices, const TArray<int32>& Triangles,
		bool bCalculateNormalTangent = false, bool bShouldCreateHardTangents = false, bool bGenerateTessellationTriangles = false)
	{
		GetOrCreateRuntimeMesh()->UpdateMeshSectionPacked_Blueprint(SectionIndex, Vertices, Triangles, bCalculateNormalTangent, bShouldCreateHardTangents, bGenerateTessellationTriangles);
	}

	




	/** Clear a section of the procedural mesh. */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearMeshSection(int32 SectionIndex)
	{
		if (URuntimeMesh* Mesh = GetRuntimeMesh())
		{
			Mesh->ClearMeshSection(SectionIndex);
		}
	}

	/** Clear all mesh sections and reset to empty state */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearAllMeshSections()
	{
		if (URuntimeMesh* Mesh = GetRuntimeMesh())
		{
			Mesh->ClearAllMeshSections();
		}
	}



	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetSectionMaterial(int32 SectionId, UMaterialInterface* Material)
	{
		if (URuntimeMesh* Mesh = GetRuntimeMesh())
		{
			Mesh->SetSectionMaterial(SectionId, Material);
		}
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	UMaterialInterface* GetSectionMaterial(int32 SectionId)
	{
		if (URuntimeMesh* Mesh = GetRuntimeMesh())
		{
			return Mesh->GetSectionMaterial(SectionId);
		}
		return nullptr;
	}

	/** Gets the bounding box of a specific section */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	FBox GetSectionBoundingBox(int32 SectionIndex)
	{
		if (URuntimeMesh* Mesh = GetRuntimeMesh())
		{
			return Mesh->GetSectionBoundingBox(SectionIndex);
		}
		return FBox(EForceInit::ForceInitToZero);
	}

	/** Control visibility of a particular section */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetMeshSectionVisible(int32 SectionIndex, bool bNewVisibility)
	{
		if (URuntimeMesh* Mesh = GetRuntimeMesh())
		{
			Mesh->SetMeshSectionVisible(SectionIndex, bNewVisibility);
		}
	}

	/** Returns whether a particular section is currently visible */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	bool IsMeshSectionVisible(int32 SectionIndex) const
	{
		if (URuntimeMesh* Mesh = GetRuntimeMesh())
		{
			return Mesh->IsMeshSectionVisible(SectionIndex);
		}
		return false;
	}


	/** Control whether a particular section casts a shadow */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetMeshSectionCastsShadow(int32 SectionIndex, bool bNewCastsShadow)
	{
		if (URuntimeMesh* Mesh = GetRuntimeMesh())
		{
			Mesh->SetMeshSectionCastsShadow(SectionIndex, bNewCastsShadow);
		}
	}

	/** Returns whether a particular section is currently casting shadows */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	bool IsMeshSectionCastingShadows(int32 SectionIndex) const
	{
		if (URuntimeMesh* Mesh = GetRuntimeMesh())
		{
			return Mesh->IsMeshSectionCastingShadows(SectionIndex);
		}
		return false;
	}


	/** Control whether a particular section has collision */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetMeshSectionCollisionEnabled(int32 SectionIndex, bool bNewCollisionEnabled)
	{
		if (URuntimeMesh* Mesh = GetRuntimeMesh())
		{
			Mesh->SetMeshSectionCollisionEnabled(SectionIndex, bNewCollisionEnabled);
		}
	}

	/** Returns whether a particular section has collision */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	bool IsMeshSectionCollisionEnabled(int32 SectionIndex)
	{
		if (URuntimeMesh* Mesh = GetRuntimeMesh())
		{
			return Mesh->IsMeshSectionCollisionEnabled(SectionIndex);
		}
		return false;
	}


	/** Returns number of sections currently created for this component */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	int32 GetNumSections() const
	{
		if (URuntimeMesh* Mesh = GetRuntimeMesh())
		{
			return Mesh->GetNumSections();
		}
		return 0;
	}

	/** Returns whether a particular section currently exists */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	bool DoesSectionExist(int32 SectionIndex) const
	{
		if (URuntimeMesh* Mesh = GetRuntimeMesh())
		{
			return Mesh->DoesSectionExist(SectionIndex);
		}
		return false;
	}

	/** Returns first available section index */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	int32 GetAvailableSectionIndex() const
	{
		if (URuntimeMesh* Mesh = GetRuntimeMesh())
		{
			return Mesh->GetAvailableSectionIndex();
		}
		return 0;
	}









	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetMeshCollisionSection(int32 CollisionSectionIndex, const TArray<FVector>& Vertices, const TArray<int32>& Triangles)
	{
		GetOrCreateRuntimeMesh()->SetMeshCollisionSection(CollisionSectionIndex, Vertices, Triangles);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearMeshCollisionSection(int32 CollisionSectionIndex)
	{
		GetOrCreateRuntimeMesh()->ClearMeshCollisionSection(CollisionSectionIndex);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearAllMeshCollisionSections()
	{
		GetOrCreateRuntimeMesh()->ClearAllMeshCollisionSections();
	}



	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	int32 AddConvexCollisionSection(TArray<FVector> ConvexVerts)
	{
		return GetOrCreateRuntimeMesh()->AddConvexCollisionSection(ConvexVerts);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetConvexCollisionSection(int32 ConvexSectionIndex, TArray<FVector> ConvexVerts)
	{
		GetOrCreateRuntimeMesh()->SetConvexCollisionSection(ConvexSectionIndex, ConvexVerts);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearConvexCollisionSection(int32 ConvexSectionIndex)
	{
		GetOrCreateRuntimeMesh()->ClearConvexCollisionSection(ConvexSectionIndex);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearAllConvexCollisionSections()
	{
		GetOrCreateRuntimeMesh()->ClearAllConvexCollisionSections();
	}

	void SetCollisionConvexMeshes(const TArray<TArray<FVector>>& ConvexMeshes)
	{
		GetOrCreateRuntimeMesh()->SetCollisionConvexMeshes(ConvexMeshes);
	}

	
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	int32 AddCollisionBox(const FRuntimeMeshCollisionBox& NewBox)
	{
		return GetOrCreateRuntimeMesh()->AddCollisionBox(NewBox);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void RemoveCollisionBox(int32 Index)
	{
		GetOrCreateRuntimeMesh()->RemoveCollisionBox(Index);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearCollisionBoxes()
	{
		GetOrCreateRuntimeMesh()->ClearCollisionBoxes();
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetCollisionBoxes(const TArray<FRuntimeMeshCollisionBox>& NewBoxes)
	{
		GetOrCreateRuntimeMesh()->SetCollisionBoxes(NewBoxes);
	}

	
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	int32 AddCollisionSphere(const FRuntimeMeshCollisionSphere& NewSphere)
	{
		return GetOrCreateRuntimeMesh()->AddCollisionSphere(NewSphere);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void RemoveCollisionSphere(int32 Index)
	{
		GetOrCreateRuntimeMesh()->RemoveCollisionSphere(Index);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearCollisionSpheres()
	{
		GetOrCreateRuntimeMesh()->ClearCollisionSpheres();
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetCollisionSpheres(const TArray<FRuntimeMeshCollisionSphere>& NewSpheres)
	{
		GetOrCreateRuntimeMesh()->SetCollisionSpheres(NewSpheres);
	}

	
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	int32 AddCollisionCapsule(const FRuntimeMeshCollisionCapsule& NewCapsule)
	{
		return GetOrCreateRuntimeMesh()->AddCollisionCapsule(NewCapsule);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void RemoveCollisionCapsule(int32 Index)
	{
		GetOrCreateRuntimeMesh()->RemoveCollisionCapsule(Index);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void ClearCollisionCapsules()
	{
		GetOrCreateRuntimeMesh()->ClearCollisionCapsules();
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetCollisionCapsules(const TArray<FRuntimeMeshCollisionCapsule>& NewCapsules)
	{
		GetOrCreateRuntimeMesh()->SetCollisionCapsules(NewCapsules);
	}


	/** Runs any pending collision cook (Not required to call this. This is only if you need to make sure all changes are cooked before doing something)*/
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void CookCollisionNow()
	{
		GetOrCreateRuntimeMesh()->CookCollisionNow();
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetCollisionUseComplexAsSimple(bool bNewValue)
	{
		GetOrCreateRuntimeMesh()->SetCollisionUseComplexAsSimple(bNewValue);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	bool IsCollisionUsingComplexAsSimple()
	{
		check(IsInGameThread());
		return GetRuntimeMesh() != nullptr ? GetRuntimeMesh()->IsCollisionUsingComplexAsSimple() : true;
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetCollisionUseAsyncCooking(bool bNewValue)
	{
		GetOrCreateRuntimeMesh()->SetCollisionUseAsyncCooking(bNewValue);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	bool IsCollisionUsingAsyncCooking()
	{
		check(IsInGameThread());
		return GetRuntimeMesh() != nullptr ? GetRuntimeMesh()->IsCollisionUsingAsyncCooking() : false;
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void SetCollisionMode(ERuntimeMeshCollisionCookingMode NewMode)
	{
		GetOrCreateRuntimeMesh()->SetCollisionMode(NewMode);
	}


private:

	//~ Begin USceneComponent Interface.
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual bool IsSupportedForNetworking() const override
	{
		return true;
	}
	//~ Begin USceneComponent Interface.

	//~ Begin UPrimitiveComponent Interface.
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual class UBodySetup* GetBodySetup() override;

public:

	// HORU: returns true if any async collision cooking is pending.
	UFUNCTION(BlueprintCallable)
	bool IsAsyncCollisionCookingPending() const;

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	int32 GetSectionIdFromCollisionFaceIndex(int32 FaceIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	void GetSectionIdAndFaceIdFromCollisionFaceIndex(int32 FaceIndex, int32& SectionIndex, int32& SectionFaceIndex) const;

	virtual UMaterialInterface* GetMaterialFromCollisionFaceIndex(int32 FaceIndex, int32& SectionIndex) const override;
	//~ End UPrimitiveComponent Interface.

public:
	//~ Begin UMeshComponent Interface.
	virtual int32 GetNumMaterials() const override;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;
	virtual UMaterialInterface* GetMaterial(int32 ElementIndex) const override;
	virtual UMaterialInterface* GetOverrideMaterial(int32 ElementIndex) const;
	//~ End UMeshComponent Interface.

private:

	/* Serializes this component */
	virtual void Serialize(FArchive& Ar) override;


	/* Does post load fixups */
	virtual void PostLoad() override;



	/** Called by URuntimeMesh any time it has new collision data that we should use */
	void NewCollisionMeshReceived();
	void NewBoundsReceived();
	void ForceProxyRecreate();

	void SendSectionCreation(int32 SectionIndex);
	void SendSectionPropertiesUpdate(int32 SectionIndex);

	// This collision setup is only to support older engine versions where the BodySetup being owned by a non UActorComponent breaks runtime cooking

	/** Collision data */
	UPROPERTY(Instanced)
	UBodySetup* BodySetup;

	/** Queue of pending collision cooks */
	UPROPERTY(Transient)
	TArray<UBodySetup*> AsyncBodySetupQueue;

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 22
	virtual bool GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData) override;
	virtual bool ContainsPhysicsTriMeshData(bool InUseAllTriData) const override;
	virtual bool WantsNegXTriMesh() override { return false; }
	//~ End Interface_CollisionDataProvider Interface
#endif

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 21
	UBodySetup* CreateNewBodySetup();
	void FinishPhysicsAsyncCook(UBodySetup* FinishedBodySetup);

	void UpdateCollision(bool bForceCookNow);
#endif



	friend class URuntimeMesh;
	friend class FRuntimeMeshComponentSceneProxy;
	friend class FRuntimeMeshComponentLegacySerialization;
};
