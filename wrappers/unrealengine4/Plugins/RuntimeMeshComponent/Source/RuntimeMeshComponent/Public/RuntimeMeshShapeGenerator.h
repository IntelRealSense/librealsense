// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RuntimeMeshGenericVertex.h"
#include "RuntimeMeshBlueprint.h"
#include "RuntimeMeshBuilder.h"
#include "RuntimeMeshShapeGenerator.generated.h"

typedef TFunction<void(const FVector& Position, const FVector& Normal, const FRuntimeMeshTangent& Tangent, const FVector2D& UV0)> FVerticesBuilderFunction;
typedef TFunction<void(int32 Index)> FTrianglesBuilderFunction;

/**
 *
 */
UCLASS()
class RUNTIMEMESHCOMPONENT_API URuntimeMeshShapeGenerator : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/** Add a quad, specified by four indices, to a triangle index buffer as two triangles. */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	static void ConvertQuadToTriangles(UPARAM(ref) TArray<int32>& Triangles, int32 Vert0, int32 Vert1, int32 Vert2, int32 Vert3);


	/** Generate vertex and index buffer for a simple box, given the supplied dimensions. Normals, UVs and tangents are also generated for each vertex. */
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	static void CreateBoxMesh(FVector BoxRadius, TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FRuntimeMeshTangent>& Tangents);

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	static void CreateBoxMeshPacked(FVector BoxRadius, TArray<FRuntimeMeshBlueprintVertexSimple>& Vertices, TArray<int32>& Triangles);

	static void CreateBoxMesh(FVector BoxRadius, TArray<FRuntimeMeshVertexSimple>& Vertices, TArray<int32>& Triangles);

	static void CreateBoxMesh(FVector BoxRadius, const TSharedPtr<FRuntimeMeshAccessor>& MeshBuilder);

	static void CreateBoxMesh(FVector BoxRadius, FRuntimeMeshAccessor& MeshBuilder);

	/**
	*	Generate an index buffer for a grid of quads.
	*	@param	NumX			Number of vertices in X direction (must be >= 2)
	*	@param	NumY			Number of vertices in y direction (must be >= 2)
	*	@param	bWinding		Reverses winding of indices generated for each quad
	*	@out	Triangles		Output index buffer
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	static void CreateGridMeshTriangles(int32 NumX, int32 NumY, bool bWinding, TArray<int32>& Triangles);

	/**
	*	Generate a grid mesh, from the specified size and number of subdivisions.
	*/
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	static void CreateGridMesh(float Width, float Height, int32 NumSubdivisionsX, int32 NumSubdivisionsY, TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FRuntimeMeshTangent>& Tangents);

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	static void CreateGridMeshPacked(float Width, float Height, int32 NumSubdivisionsX, int32 NumSubdivisionsY, TArray<FRuntimeMeshBlueprintVertexSimple>& Vertices, TArray<int32>& Triangles);

	static void CreateGridMesh(float Width, float Height, int32 NumSubdivisionsX, int32 NumSubdivisionsY, TArray<FRuntimeMeshVertexSimple>& Vertices, TArray<int32>& Triangles);

	static void CreateGridMesh(float Width, float Height, int32 NumSubdivisionsX, int32 NumSubdivisionsY, const TSharedPtr<FRuntimeMeshAccessor>& MeshBuilder);



public:

	static void ConvertQuadToTriangles(TFunction<void(int32 Index)> TrianglesBuilder, int32 Vert0, int32 Vert1, int32 Vert2, int32 Vert3);

	static void CreateBoxMesh(FVector BoxRadius, FVerticesBuilderFunction VerticesBuilder, FTrianglesBuilderFunction TrianglesBuilder);

	static void CreateGridMeshTriangles(int32 NumX, int32 NumY, bool bWinding, FTrianglesBuilderFunction TrianglesBuilder);

	static void CreateGridMesh(float Width, float Height, int32 NumSubdivisionsX, int32 NumSubdivisionsY, FVerticesBuilderFunction VerticesBuilder, FTrianglesBuilderFunction TrianglesBuilder);

};
