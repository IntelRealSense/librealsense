// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshGenericVertex.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RuntimeMeshBuilder.h"
#include "RuntimeMeshBlueprint.h"
#include "RuntimeMeshLibrary.generated.h"

/**
 *
 */
UCLASS()
class RUNTIMEMESHCOMPONENT_API URuntimeMeshLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	*	Automatically generate normals and tangent vectors for a mesh
	*	UVs are required for correct tangent generation.
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh", meta = (AutoCreateRefTerm = "UVs"))
	static void CalculateTangentsForMesh(const TArray<FVector>& Vertices, const TArray<int32>& Triangles,
		TArray<FVector>& Normals, const TArray<FVector2D>& UVs, TArray<FRuntimeMeshTangent>& Tangents, bool bCreateSmoothNormals = true);

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	static void CalculateTangentsForMeshPacked(TArray<FRuntimeMeshBlueprintVertexSimple>& Vertices, const TArray<int32>& Triangles, bool bCreateSmoothNormals = true);

	static void CalculateTangentsForMesh(TArray<FRuntimeMeshVertexSimple>& Vertices, const TArray<int32>& Triangles, bool bCreateSmoothNormals = true);

	static void CalculateTangentsForMesh(const TSharedPtr<FRuntimeMeshAccessor>& MeshAccessor, bool bCreateSmoothNormals = true);



	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh", meta = (AutoCreateRefTerm = "UVs"))
	static TArray<int32> GenerateTessellationIndexBuffer(const TArray<FVector>& Vertices, const TArray<int32>& Triangles,
		TArray<FVector>& Normals, const TArray<FVector2D>& UVs, TArray<FRuntimeMeshTangent>& Tangents);

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	static TArray<int32> GenerateTessellationIndexBufferPacked(const TArray<FRuntimeMeshBlueprintVertexSimple>& Vertices, const TArray<int32>& Triangles);

	static TArray<int32> GenerateTessellationIndexBuffer(const TArray<FRuntimeMeshVertexSimple>& Vertices, const TArray<int32>& Triangles);

	static TArray<int32> GenerateTessellationIndexBuffer(const TSharedPtr<FRuntimeMeshAccessor>& MeshAccessor);

	static void GenerateTessellationIndexBuffer(const TSharedPtr<FRuntimeMeshAccessor>& MeshAccessor, const TSharedPtr<FRuntimeMeshIndicesAccessor>& OutTessellationIndices);


	/** Grab geometry data from a StaticMesh asset. */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh", meta = (AutoCreateRefTerm = "UVs"))
	static void GetStaticMeshSection(UStaticMesh* InMesh, int32 LODIndex, int32 SectionIndex, TArray<FVector>& Vertices, TArray<int32>& Triangles,
		TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FColor>& Colors, TArray<FRuntimeMeshTangent>& Tangents);

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	static void GetStaticMeshSectionPacked(UStaticMesh* InMesh, int32 LODIndex, int32 SectionIndex, TArray<FRuntimeMeshBlueprintVertexSimple>& Vertices, TArray<int32>& Triangles);

	static void GetStaticMeshSection(UStaticMesh* InMesh, int32 LODIndex, int32 SectionIndex, TArray<FRuntimeMeshVertexSimple>& Vertices, TArray<int32>& Triangles);

	static void GetStaticMeshSection(UStaticMesh* InMesh, int32 LODIndex, int32 SectionIndex, TArray<FRuntimeMeshVertexSimple>& Vertices, TArray<int32>& Triangles, TArray<int32>& AdjacencyTriangles);

	static void GetStaticMeshSection(UStaticMesh* InMesh, int32 LODIndex, int32 SectionIndex, const TSharedPtr<FRuntimeMeshAccessor>& MeshAccessor);

	static void GetStaticMeshSection(UStaticMesh* InMesh, int32 LODIndex, int32 SectionIndex, const TSharedPtr<FRuntimeMeshAccessor>& MeshAccessor, const TSharedPtr<FRuntimeMeshIndicesAccessor>& TessellationIndicesAccessor);

	template<typename VertexType0, typename IndexType>
	static void GetStaticMeshSection(UStaticMesh* InMesh, int32 LODIndex, int32 SectionIndex, TArray<VertexType0>& Vertices, TArray<IndexType>& Triangles) {
		Vertices.Empty();
		Triangles.Empty();

		GetStaticMeshSection(InMesh, LODIndex, SectionIndex, 1,
			[&Vertices](FVector Position, FVector TangentX, FVector TangentY, FVector TangentZ) -> int32
		{
			return Vertices.Add(VertexType0(Position, TangentX, TangentY, TangentZ));
		},
			[&Vertices](int32 Index, int32 UVIndex, FVector2D UV)
		{
			check(UVIndex == 0);
			Vertices[Index].UV0 = UV;
		},
			[&Vertices](int32 Index, FColor Color)
		{
			Vertices[Index].Color = Color;
		},
			[&Triangles](int32 Index)
		{
			Triangles.Add(Index);
		},
			[](int32 Index)
		{

		});
	};


	/** Copy sections from a static mesh to a runtime mesh */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	static void CopyStaticMeshToRuntimeMesh(UStaticMeshComponent* StaticMeshComponent, int32 LODIndex, URuntimeMesh* RuntimeMesh, bool bCreateCollision);
	
	/** Copy sections from a static mesh to a runtime mesh */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	static void CopyStaticMeshToRuntimeMeshComponent(UStaticMeshComponent* StaticMeshComponent, int32 LODIndex, URuntimeMeshComponent* RuntimeMeshComponent, bool bCreateCollision);


	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	static void CopyCollisionFromStaticMesh(UStaticMesh* StaticMesh, URuntimeMesh* RuntimeMesh);

	static void CopyCollisionFromStaticMesh(UStaticMeshComponent* StaticMeshComponent, URuntimeMesh* RuntimeMesh)
	{
		CopyCollisionFromStaticMesh(StaticMeshComponent->GetStaticMesh(), RuntimeMesh);
	}

private:
	static void CalculateTangentsForMesh(TFunction<int32(int32 Index)> IndexAccessor, TFunction<FVector(int32 Index)> VertexAccessor, TFunction<FVector2D(int32 Index)> UVAccessor,
		TFunction<void(int32 Index, FVector TangentX, FVector TangentY, FVector TangentZ)> TangentSetter, int32 NumVertices, int32 NumUVs, int32 NumIndices, bool bCreateSmoothNormals);


	static int32 GetNewIndexForOldVertIndex(const FPositionVertexBuffer* PosBuffer, const FStaticMeshVertexBuffer* VertBuffer, const FColorVertexBuffer* ColorBuffer,
		TMap<int32, int32>& MeshToSectionVertMap, int32 VertexIndex, int32 NumUVChannels, TFunction<int32(FVector Position, FVector TangentX, FVector TangentY, FVector TangentZ)> VertexCreator,
		TFunction<void(int32 VertexIndex, int32 UVIndex, FVector2D UV)> UVSetter, TFunction<void(int32 VertexIndex, FColor Color)> ColorSetter);

	static void GetStaticMeshSection(UStaticMesh* InMesh, int32 LODIndex, int32 SectionIndex, int32 NumSupportedUVs,
		TFunction<int32(FVector Position, FVector TangentX, FVector TangentY, FVector TangentZ)> VertexCreator,
		TFunction<void(int32 VertexIndex, int32 UVIndex, FVector2D UV)> UVSetter, TFunction<void(int32 VertexIndex, FColor Color)> ColorSetter, TFunction<void(int32)> IndexCreator, TFunction<void(int32)> AdjacencyIndexCreator);

};
