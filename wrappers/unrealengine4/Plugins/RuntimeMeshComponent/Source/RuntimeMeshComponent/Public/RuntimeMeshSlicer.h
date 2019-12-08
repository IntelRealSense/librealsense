// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RuntimeMeshBuilder.h"
#include "RuntimeMeshSlicer.generated.h"


struct FUtilPoly2D;
struct FUtilEdge3D;
struct FKConvexElem;

/** Options for creating cap geometry when slicing */
UENUM()
enum class ERuntimeMeshSlicerCapOption : uint8
{
	/** Do not create cap geometry */
	NoCap,
	/** Add a new section to ProceduralMesh for cap */
	CreateNewSectionForCap,
	/** Add cap geometry to existing last section */
	UseLastSectionForCap
};

/**
 *
 */
UCLASS()
class RUNTIMEMESHCOMPONENT_API URuntimeMeshSlicer : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()


	/**
	*	Utility to test what side of a plane the box is on.
	*	Returns 1 if on positive side of plane
	*	Returns -1 if on negative side of plane
	*	Returns 0 if intersecting plane
	*/
	static int32 CompareBoxPlane(const FBox& InBox, const FPlane& InPlane);

	/**
	*	Take two RuntimeMeshAccesorVertex's and interpolate all their values
	*/
	static FRuntimeMeshAccessorVertex InterpolateVert(const FRuntimeMeshAccessorVertex& V0, const FRuntimeMeshAccessorVertex& V1, float Alpha);

	/** Transform triangle from 2D to 3D static-mesh triangle. */
	static void Transform2DPolygonTo3D(const FUtilPoly2D& InPoly, const FMatrix& InMatrix, TSharedPtr<FRuntimeMeshAccessor> OutMesh);

	/** Given a polygon, decompose into triangles. */
	static bool TriangulatePoly(TSharedPtr<FRuntimeMeshAccessor> Mesh, int32 VertBase, const FVector& PolyNormal);

	/** Util to slice a convex hull with a plane */
	static void SliceConvexElem(const FKConvexElem& InConvex, const FPlane& SlicePlane, TArray<FVector>& OutConvexVerts);

	/** Slice Convex Collision of a URuntimeMesh */
	static void SliceRuntimeMeshConvexCollision(URuntimeMesh* InRuntimeMesh, URuntimeMesh* OutOtherHalf, FVector PlanePosition, FVector PlaneNormal);

	/** Slice mesh data of section within URuntimeMesh */
	static void SliceRuntimeMeshSection(const FRuntimeMeshDataPtr& InRuntimeMesh, const FRuntimeMeshDataPtr& OutOtherHalf, int32 SectionIndex, const FPlane& SlicePlane, TArray<FUtilEdge3D>& ClipEdges);

	/** Cap slice opening of URuntimeMesh */
	static int32 CapMeshSlice(const FRuntimeMeshDataPtr& InRuntimeMesh, const FRuntimeMeshDataPtr& OutOtherHalf, TArray<FUtilEdge3D>& ClipEdges, const FPlane& SlicePlane, FVector PlaneNormal, ERuntimeMeshSlicerCapOption CapOption);


public:

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	static void SliceRuntimeMesh(URuntimeMesh* InRuntimeMesh, FVector PlanePosition, FVector PlaneNormal, URuntimeMesh* OutOtherHalf, ERuntimeMeshSlicerCapOption CapOption, UMaterialInterface* CapMaterial);

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMesh")
	static void SliceRuntimeMeshComponent(URuntimeMeshComponent* InRuntimeMesh, FVector PlanePosition, FVector PlaneNormal, bool bCreateOtherHalf, URuntimeMeshComponent*& OutOtherHalf, ERuntimeMeshSlicerCapOption CapOption, UMaterialInterface* CapMaterial);

};
