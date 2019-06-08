// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshSlicer.h"
#include "RuntimeMeshComponentPlugin.h"
#include "GeomTools.h"
#include "RuntimeMesh.h"
#include "RuntimeMeshData.h"
#include "RuntimeMeshComponent.h"
#include "PhysicsEngine/BodySetup.h"


int32 URuntimeMeshSlicer::CompareBoxPlane(const FBox& InBox, const FPlane& InPlane)
{
	FVector BoxCenter, BoxExtents;
	InBox.GetCenterAndExtents(BoxCenter, BoxExtents);

	// Find the distance from the plane to the center of the box
	float BoxCenterDist = InPlane.PlaneDot(BoxCenter);

	// Find the size of the box in direction of plane normal
	float BoxSize = FVector::BoxPushOut(InPlane, BoxExtents);

	return (BoxCenterDist > BoxSize) ? 1 : ((BoxCenterDist < -BoxSize) ? -1 : 0);
}

FRuntimeMeshAccessorVertex URuntimeMeshSlicer::InterpolateVert(const FRuntimeMeshAccessorVertex& V0, const FRuntimeMeshAccessorVertex& V1, float Alpha)
{
	FRuntimeMeshAccessorVertex Result;

	// Handle bad alpha
	if (FMath::IsNaN(Alpha) || !FMath::IsFinite(Alpha))
	{
		Result = V1;
		return Result;
	}

	Result.Position = FMath::Lerp(V0.Position, V1.Position, Alpha);

	// We assume the sign of the binormal (W component of Normal) doesn't change
	Result.Normal = FMath::Lerp(V0.Normal, V1.Normal, Alpha);
	Result.Tangent = FMath::Lerp(V0.Tangent, V1.Tangent, Alpha);

	Result.Color.R = FMath::Clamp(FMath::TruncToInt(FMath::Lerp(float(V0.Color.R), float(V1.Color.R), Alpha)), 0, 255);
	Result.Color.G = FMath::Clamp(FMath::TruncToInt(FMath::Lerp(float(V0.Color.G), float(V1.Color.G), Alpha)), 0, 255);
	Result.Color.B = FMath::Clamp(FMath::TruncToInt(FMath::Lerp(float(V0.Color.B), float(V1.Color.B), Alpha)), 0, 255);
	Result.Color.A = FMath::Clamp(FMath::TruncToInt(FMath::Lerp(float(V0.Color.A), float(V1.Color.A), Alpha)), 0, 255);

	// We should never have a different number of UVs between the vertices
	check(V0.UVs.Num() == V1.UVs.Num());

	Result.UVs.SetNum(V0.UVs.Num());
	for (int32 Index = 0; Index < Result.UVs.Num(); Index++)
	{
		Result.UVs[Index] = FMath::Lerp(V0.UVs[Index], V1.UVs[Index], Alpha);
	}

	return Result;
}


void URuntimeMeshSlicer::Transform2DPolygonTo3D(const FUtilPoly2D& InPoly, const FMatrix& InMatrix, TSharedPtr<FRuntimeMeshAccessor> OutMesh)
{
	// Set the W component to 1 to not flip the binormal
	FVector4 PolyNormal = FVector4(-InMatrix.GetUnitAxis(EAxis::Z), 1.0f);
	FVector PolyTangent = InMatrix.GetUnitAxis(EAxis::X);

	int32 NumUVs = OutMesh->NumUVChannels();

	for (int32 VertexIndex = 0; VertexIndex < InPoly.Verts.Num(); VertexIndex++)
	{
		const FUtilVertex2D& InVertex = InPoly.Verts[VertexIndex];

		FRuntimeMeshAccessorVertex NewVert;

		int32 NewIndex = OutMesh->AddVertex(InMatrix.TransformPosition(FVector(InVertex.Pos.X, InVertex.Pos.Y, 0.f)));
		OutMesh->SetNormal(NewIndex, PolyNormal);
		OutMesh->SetTangent(NewIndex, PolyTangent);
		OutMesh->SetColor(NewIndex, InVertex.Color);

		NewVert.UVs.SetNum(NumUVs);
		for (int32 Index = 0; Index < NumUVs; Index++)
		{
			OutMesh->SetUV(NewIndex, Index, InVertex.UV);
		}
	}
}

bool URuntimeMeshSlicer::TriangulatePoly(TSharedPtr<FRuntimeMeshAccessor> Mesh, int32 VertBase, const FVector& PolyNormal)
{
	// Bail if we don't have enough vertices
	int32 NumVerts = Mesh->NumVertices() - VertBase;
	if (NumVerts <= 3)
	{
		Mesh->AddIndex(FMath::Clamp(0 + VertBase, 0, NumVerts));
		Mesh->AddIndex(FMath::Clamp(2 + VertBase, 0, NumVerts));
		Mesh->AddIndex(FMath::Clamp(1 + VertBase, 0, NumVerts));

		return true;
	}

	// Store the starting size of the indices. 
	const int32 OriginalNumIndices = Mesh->NumIndices();

	// Initialize starting array of indices
	TArray<int32> VertIndices;
	VertIndices.SetNumUninitialized(NumVerts);
	for (int VertIndex = 0; VertIndex < NumVerts; VertIndex++)
	{
		VertIndices[VertIndex] = VertBase + VertIndex;
	}

	// Find ears until there are no vertices
	while (VertIndices.Num() >= 3)
	{
		// Look for an 'ear' triangle
		bool bFoundEar = false;
		for (int32 EarVertexIndex = 0; EarVertexIndex < VertIndices.Num(); EarVertexIndex++)
		{
			// Triangle is 'this' vert plus the one before and after it
			const int32 Index1 = (EarVertexIndex == 0) ? VertIndices.Num() - 1 : EarVertexIndex - 1;
			const int32 Index2 = EarVertexIndex;
			const int32 Index3 = (EarVertexIndex + 1) % VertIndices.Num();

			const FRuntimeMeshAccessorVertex Vert1 = Mesh->GetVertex(VertIndices[Index1]);
			const FRuntimeMeshAccessorVertex Vert2 = Mesh->GetVertex(VertIndices[Index2]);
			const FRuntimeMeshAccessorVertex Vert3 = Mesh->GetVertex(VertIndices[Index3]);

			// Check that this vertex is convex (cross product must be positive)
			const FVector Edge12 = Vert2.Position - Vert1.Position;
			const FVector Edge13 = Vert3.Position - Vert1.Position;
			const float Determinant = (Edge12 ^ Edge13) | PolyNormal;
			if (Determinant > 0.f)
			{
				continue;
			}

			bool bFoundVertInside = false;
			// Look through all verts before this in array to see if any are inside triangle
			for (int32 VertexIndex = 0; VertexIndex < VertIndices.Num(); VertexIndex++)
			{
				const FRuntimeMeshAccessorVertex& TestVert = Mesh->GetVertex(VertIndices[VertexIndex]);

				if (VertexIndex != Index1 &&
					VertexIndex != Index2 &&
					VertexIndex != Index3 &&
					FGeomTools::PointInTriangle(Vert1.Position, Vert2.Position, Vert3.Position, TestVert.Position))
				{
					bFoundVertInside = true;
					break;
				}
			}

			// Triangle with no verts inside - its an 'ear'! 
			if (!bFoundVertInside)
			{
				Mesh->AddIndex(VertIndices[Index1]);
				Mesh->AddIndex(VertIndices[Index3]);
				Mesh->AddIndex(VertIndices[Index2]);

				// And remove vertex from polygon
				VertIndices.RemoveAt(EarVertexIndex);

				bFoundEar = true;
				break;
			}
		}

		// If we couldn't find an 'ear' it indicates something is bad with this polygon - discard triangles and return.
		if (!bFoundEar)
		{
			Mesh->SetNumIndices(OriginalNumIndices);
			return false;
		}
	}

	return true;
}

void URuntimeMeshSlicer::SliceConvexElem(const FKConvexElem& InConvex, const FPlane& SlicePlane, TArray<FVector>& OutConvexVerts)
{
	FPlane SlicePlaneFlipped = SlicePlane.Flip();

	// Get set of planes that make up hull
	TArray<FPlane> ConvexPlanes;
	InConvex.GetPlanes(ConvexPlanes);

	if (ConvexPlanes.Num() >= 4)
	{
		// Add on the slicing plane (need to flip as it culls geom in the opposite sense to our geom culling code)
		ConvexPlanes.Add(SlicePlaneFlipped);

		// Create output convex based on new set of planes
		FKConvexElem SlicedElem;
		bool bSuccess = SlicedElem.HullFromPlanes(ConvexPlanes, InConvex.VertexData);
		if (bSuccess)
		{
			OutConvexVerts = SlicedElem.VertexData;
		}
	}
}


void URuntimeMeshSlicer::SliceRuntimeMeshConvexCollision(URuntimeMesh* InRuntimeMesh, URuntimeMesh* OutOtherHalf, FVector PlanePosition, FVector PlaneNormal)
{
	bool bCreateOtherHalf = OutOtherHalf != nullptr;

	PlaneNormal.Normalize();
	FPlane SlicePlane(PlanePosition, PlaneNormal);

	// Array of sliced collision shapes
	TArray<TArray<FVector>> SlicedCollision;
	TArray<TArray<FVector>> OtherSlicedCollision;

	UBodySetup* BodySetup = InRuntimeMesh->GetBodySetup();

	for (int32 ConvexIndex = 0; ConvexIndex < BodySetup->AggGeom.ConvexElems.Num(); ConvexIndex++)
	{
		FKConvexElem& BaseConvex = BodySetup->AggGeom.ConvexElems[ConvexIndex];

		int32 BoxCompare = CompareBoxPlane(BaseConvex.ElemBox, SlicePlane);

		// If box totally clipped, add to other half (if desired)
		if (BoxCompare == -1)
		{
			if (bCreateOtherHalf)
			{
				OtherSlicedCollision.Add(BaseConvex.VertexData);
			}
		}
		// If box totally valid, just keep mesh as is
		else if (BoxCompare == 1)
		{
			SlicedCollision.Add(BaseConvex.VertexData);
		}
		// Need to actually slice the convex shape
		else
		{
			TArray<FVector> SlicedConvexVerts;
			SliceConvexElem(BaseConvex, SlicePlane, SlicedConvexVerts);
			// If we got something valid, add it
			if (SlicedConvexVerts.Num() >= 4)
			{
				SlicedCollision.Add(SlicedConvexVerts);
			}

			// Slice again to get the other half of the collision, if desired
			if (bCreateOtherHalf)
			{
				TArray<FVector> OtherSlicedConvexVerts;
				SliceConvexElem(BaseConvex, SlicePlane.Flip(), OtherSlicedConvexVerts);
				if (OtherSlicedConvexVerts.Num() >= 4)
				{
					OtherSlicedCollision.Add(OtherSlicedConvexVerts);
				}
			}
		}
	}

	// Update collision of runtime mesh
	InRuntimeMesh->SetCollisionConvexMeshes(SlicedCollision);

	// Set collision for other mesh
	if (bCreateOtherHalf)
	{
		OutOtherHalf->SetCollisionConvexMeshes(OtherSlicedCollision);
	}
}

void URuntimeMeshSlicer::SliceRuntimeMeshSection(const FRuntimeMeshDataPtr& InRuntimeMesh, const FRuntimeMeshDataPtr& OutOtherHalf, int32 SectionIndex, const FPlane& SlicePlane, TArray<FUtilEdge3D>& ClipEdges)
{
	bool bShouldCreateOtherHalf = OutOtherHalf.IsValid();

	auto SourceMeshData = InRuntimeMesh->GetSectionReadonly(SectionIndex);
	TSharedPtr<FRuntimeMeshBuilder> NewMeshData = MakeRuntimeMeshBuilder(*SourceMeshData.Get());
	TSharedPtr<FRuntimeMeshBuilder> OtherMeshData = bShouldCreateOtherHalf ? MakeRuntimeMeshBuilder(*SourceMeshData.Get()) : TSharedPtr<FRuntimeMeshBuilder>(nullptr);

	// Map of base vert index to sliced vert index
	TMap<int32, int32> BaseToSlicedVertIndex;
	TMap<int32, int32> BaseToOtherSlicedVertIndex;

	const int32 NumBaseVerts = SourceMeshData->NumVertices();

	// Distance of each base vert from slice plane
	TArray<float> VertDistance;
	VertDistance.SetNumUninitialized(NumBaseVerts);

	// Build vertex buffer 
	for (int32 BaseVertIndex = 0; BaseVertIndex < NumBaseVerts; BaseVertIndex++)
	{
		FRuntimeMeshAccessorVertex BaseVert = SourceMeshData->GetVertex(BaseVertIndex);

		// Calculate distance from plane
		VertDistance[BaseVertIndex] = SlicePlane.PlaneDot(BaseVert.Position);

		// See if vert is being kept in this section
		if (VertDistance[BaseVertIndex] > 0.f)
		{
			// Copy to sliced v buffer
			int32 SlicedVertIndex = NewMeshData->AddVertex(BaseVert);

			// Add to map
			BaseToSlicedVertIndex.Add(BaseVertIndex, SlicedVertIndex);
		}
		// Or add to other half if desired
		else if (bShouldCreateOtherHalf)
		{
			int32 SlicedVertIndex = OtherMeshData->AddVertex(BaseVert);

			BaseToOtherSlicedVertIndex.Add(BaseVertIndex, SlicedVertIndex);
		}
	}


	// Iterate over base triangles (IE. 3 indices at a time)
	int32 NumBaseIndices = SourceMeshData->NumIndices();
	for (int32 BaseIndex = 0; BaseIndex < NumBaseIndices; BaseIndex += 3)
	{
		int32 BaseV[3]; // Triangle vert indices in original mesh
		int32* SlicedV[3]; // Pointers to vert indices in new v buffer
		int32* SlicedOtherV[3]; // Pointers to vert indices in new 'other half' v buffer

								// For each vertex..
		for (int32 i = 0; i < 3; i++)
		{
			// Get triangle vert index
			BaseV[i] = SourceMeshData->GetIndex(BaseIndex + i);
			// Look up in sliced v buffer
			SlicedV[i] = BaseToSlicedVertIndex.Find(BaseV[i]);
			// Look up in 'other half' v buffer (if desired)
			if (bShouldCreateOtherHalf)
			{
				SlicedOtherV[i] = BaseToOtherSlicedVertIndex.Find(BaseV[i]);
				// Each base vert _must_ exist in either BaseToSlicedVertIndex or BaseToOtherSlicedVertIndex 
				check((SlicedV[i] != nullptr) != (SlicedOtherV[i] != nullptr));
			}
		}

		// If all verts survived plane cull, keep the triangle
		if (SlicedV[0] != nullptr && SlicedV[1] != nullptr && SlicedV[2] != nullptr)
		{
			NewMeshData->AddIndex(*SlicedV[0]);
			NewMeshData->AddIndex(*SlicedV[1]);
			NewMeshData->AddIndex(*SlicedV[2]);
		}
		// If all verts were removed by plane cull
		else if (SlicedV[0] == nullptr && SlicedV[1] == nullptr && SlicedV[2] == nullptr)
		{
			// If creating other half, add all verts to that
			if (bShouldCreateOtherHalf)
			{
				OtherMeshData->AddIndex(*SlicedOtherV[0]);
				OtherMeshData->AddIndex(*SlicedOtherV[1]);
				OtherMeshData->AddIndex(*SlicedOtherV[2]);
			}
		}
		// If partially culled, clip to create 1 or 2 new triangles
		else
		{
			int32 FinalVerts[4];
			int32 NumFinalVerts = 0;

			int32 OtherFinalVerts[4];
			int32 NumOtherFinalVerts = 0;

			FUtilEdge3D NewClipEdge;
			int32 ClippedEdges = 0;

			float PlaneDist[3];
			PlaneDist[0] = VertDistance[BaseV[0]];
			PlaneDist[1] = VertDistance[BaseV[1]];
			PlaneDist[2] = VertDistance[BaseV[2]];

			for (int32 EdgeIdx = 0; EdgeIdx < 3; EdgeIdx++)
			{
				int32 ThisVert = EdgeIdx;

				// If start vert is inside, add it.
				if (SlicedV[ThisVert] != nullptr)
				{
					check(NumFinalVerts < 4);
					FinalVerts[NumFinalVerts++] = *SlicedV[ThisVert];
				}
				// If not, add to other side
				else if (bShouldCreateOtherHalf)
				{
					check(NumOtherFinalVerts < 4);
					OtherFinalVerts[NumOtherFinalVerts++] = *SlicedOtherV[ThisVert];
				}

				// If start and next vert are on opposite sides, add intersection
				int32 NextVert = (EdgeIdx + 1) % 3;

				if ((SlicedV[EdgeIdx] == nullptr) != (SlicedV[NextVert] == nullptr))
				{
					// Find distance along edge that plane is
					float Alpha = -PlaneDist[ThisVert] / (PlaneDist[NextVert] - PlaneDist[ThisVert]);
					// Interpolate vertex params to that point
					FRuntimeMeshAccessorVertex InterpVert = InterpolateVert(
						SourceMeshData->GetVertex(BaseV[ThisVert]),
						SourceMeshData->GetVertex(BaseV[NextVert]),
						FMath::Clamp(Alpha, 0.0f, 1.0f));

					// Add to vertex buffer
					int32 InterpVertIndex = NewMeshData->AddVertex(InterpVert);

					// Save vert index for this poly
					check(NumFinalVerts < 4);
					FinalVerts[NumFinalVerts++] = InterpVertIndex;

					// If desired, add to the poly for the other half as well
					if (bShouldCreateOtherHalf)
					{
						int32 OtherInterpVertIndex = OtherMeshData->AddVertex(InterpVert);

						check(NumOtherFinalVerts < 4);
						OtherFinalVerts[NumOtherFinalVerts++] = OtherInterpVertIndex;
					}

					// When we make a new edge on the surface of the clip plane, save it off.
					check(ClippedEdges < 2);
					if (ClippedEdges == 0)
					{
						NewClipEdge.V0 = InterpVert.Position;
					}
					else
					{
						NewClipEdge.V1 = InterpVert.Position;
					}

					ClippedEdges++;
				}
			}

			// Triangulate the clipped polygon.
			for (int32 VertexIndex = 2; VertexIndex < NumFinalVerts; VertexIndex++)
			{
				NewMeshData->AddIndex(FinalVerts[0]);
				NewMeshData->AddIndex(FinalVerts[VertexIndex - 1]);
				NewMeshData->AddIndex(FinalVerts[VertexIndex]);
			}

			// If we are making the other half, triangulate that as well
			if (bShouldCreateOtherHalf)
			{
				for (int32 VertexIndex = 2; VertexIndex < NumOtherFinalVerts; VertexIndex++)
				{
					OtherMeshData->AddIndex(OtherFinalVerts[0]);
					OtherMeshData->AddIndex(OtherFinalVerts[VertexIndex - 1]);
					OtherMeshData->AddIndex(OtherFinalVerts[VertexIndex]);
				}
			}

			check(ClippedEdges != 1); // Should never clip just one edge of the triangle

									  // If we created a new edge, save that off here as well
			if (ClippedEdges == 2)
			{
				ClipEdges.Add(NewClipEdge);
			}
		}
	}

	// Add the mesh data to the other mesh if we're building the other mesh and have valid geometry.
	if (bShouldCreateOtherHalf && OtherMeshData->NumVertices() > 0 && OtherMeshData->NumIndices() > 0)
	{
		OutOtherHalf->CreateMeshSection(SectionIndex, MoveTemp(OtherMeshData));
	}

	// Update this runtime mesh, or clear it if we have no geometry
	if (NewMeshData->NumVertices() > 0 && NewMeshData->NumIndices() > 0)
	{
		InRuntimeMesh->UpdateMeshSection(SectionIndex, MoveTemp(NewMeshData));
	}
	else
	{
		InRuntimeMesh->ClearMeshSection(SectionIndex);
	}
}

int32 URuntimeMeshSlicer::CapMeshSlice(const FRuntimeMeshDataPtr& InRuntimeMesh, const FRuntimeMeshDataPtr& OutOtherHalf, TArray<FUtilEdge3D>& ClipEdges, const FPlane& SlicePlane, FVector PlaneNormal, ERuntimeMeshSlicerCapOption CapOption)
{
	bool bShouldCreateOtherHalf = OutOtherHalf.IsValid();

	int32 NewCapSectionIndex = InRuntimeMesh->GetLastSectionIndex() + 1;
	if (bShouldCreateOtherHalf)
	{
		NewCapSectionIndex = FMath::Max(NewCapSectionIndex, OutOtherHalf->GetLastSectionIndex() + 1);
	}

	// Create cap geometry (if some edges to create it from)
	if (CapOption != ERuntimeMeshSlicerCapOption::NoCap && ClipEdges.Num() > 0)
	{
		TSharedPtr<FRuntimeMeshBuilder> CapSection;
		int32 CapSectionIndex = INDEX_NONE;

		// If using an existing section, copy that info first
		if (CapOption == ERuntimeMeshSlicerCapOption::UseLastSectionForCap)
		{
			CapSectionIndex = InRuntimeMesh->GetLastSectionIndex();
			auto ExistingMesh = InRuntimeMesh->GetSectionReadonly(NewCapSectionIndex);
			CapSection = MakeRuntimeMeshBuilder(*ExistingMesh.Get());
			ExistingMesh->CopyTo(CapSection);
		}
		// Adding new section for cap
		else
		{
			CapSection = MakeRuntimeMeshBuilder<FRuntimeMeshTangents, FVector2DHalf, uint16>();
			CapSectionIndex = NewCapSectionIndex;
		}

		// Project 3D edges onto slice plane to form 2D edges
		TArray<FUtilEdge2D> Edges2D;
		FUtilPoly2DSet PolySet;
		FGeomTools::ProjectEdges(Edges2D, PolySet.PolyToWorld, ClipEdges, SlicePlane);

		// Find 2D closed polygons from this edge soup
		FGeomTools::Buid2DPolysFromEdges(PolySet.Polys, Edges2D, FColor(255, 255, 255, 255));

		// Remember start point for vert and index buffer before adding and cap geom
		int32 CapVertBase = CapSection->NumVertices();
		int32 CapIndexBase = CapSection->NumIndices();

		// Triangulate each poly
		for (int32 PolyIdx = 0; PolyIdx < PolySet.Polys.Num(); PolyIdx++)
		{
			// Generate UVs for the 2D polygon.
			FGeomTools::GeneratePlanarTilingPolyUVs(PolySet.Polys[PolyIdx], 64.f);

			// Remember start of vert buffer before adding triangles for this poly
			int32 PolyVertBase = CapSection->NumVertices();

			// Transform from 2D poly verts to 3D
			Transform2DPolygonTo3D(PolySet.Polys[PolyIdx], PolySet.PolyToWorld, CapSection);

			// Triangulate this polygon
			TriangulatePoly(CapSection, PolyVertBase, PlaneNormal);
		}

		// Set geom for cap section
		if (CapOption == ERuntimeMeshSlicerCapOption::UseLastSectionForCap)
		{
			InRuntimeMesh->UpdateMeshSection(CapSectionIndex, MoveTemp(CapSection));
		}
		else
		{
			InRuntimeMesh->CreateMeshSection(CapSectionIndex, MoveTemp(CapSection));
		}

		// If creating the other half, copy cap geom into other half sections
		if (bShouldCreateOtherHalf)
		{
			TSharedPtr<FRuntimeMeshBuilder> OtherCapSection;
			int32 OtherCapSectionIndex = INDEX_NONE;

			// If using an existing section, copy that info first
			if (CapOption == ERuntimeMeshSlicerCapOption::UseLastSectionForCap)
			{
				OtherCapSectionIndex = OutOtherHalf->GetLastSectionIndex();
				auto ExistingMesh = OutOtherHalf->GetSectionReadonly(CapSectionIndex);
				OtherCapSection = MakeRuntimeMeshBuilder(*ExistingMesh.Get());
				ExistingMesh->CopyTo(OtherCapSection);
			}
			// Adding new section for cap
			else
			{
				OtherCapSection = MakeRuntimeMeshBuilder<FRuntimeMeshTangents, FVector2DHalf, uint16>();
				OtherCapSectionIndex = NewCapSectionIndex;
			}

			// Remember current base index for verts in 'other cap section'
			int32 OtherCapVertBase = OtherCapSection->NumVertices();

			// Copy verts from cap section into other cap section
			int32 CapSectionLength = CapSection->NumVertices();
			for (int32 VertIdx = CapVertBase; VertIdx < CapSectionLength; VertIdx++)
			{
				FRuntimeMeshAccessorVertex OtherCapVert = CapSection->GetVertex(VertIdx);

				// Flip normal and tangent TODO: FlipY?
				float Sign = OtherCapVert.Normal.W;
				OtherCapVert.Normal *= -1.f;
				OtherCapVert.Normal.W = Sign;
				OtherCapVert.Tangent *= -1.f;

				// Add to other cap v buffer
				OtherCapSection->AddVertex(OtherCapVert);
			}

			// Find offset between main cap verts and other cap verts
			int32 VertOffset = OtherCapVertBase - CapVertBase;

			// Copy indices over as well
			int32 NumCapIndices = CapSection->NumIndices();
			for (int32 IndexIdx = CapIndexBase; IndexIdx < NumCapIndices; IndexIdx += 3)
			{
				// Need to offset and change winding
				OtherCapSection->AddIndex(CapSection->GetIndex(IndexIdx + 0) + VertOffset);
				OtherCapSection->AddIndex(CapSection->GetIndex(IndexIdx + 2) + VertOffset);
				OtherCapSection->AddIndex(CapSection->GetIndex(IndexIdx + 1) + VertOffset);
			}

			// Set geom for cap section
			if (CapOption == ERuntimeMeshSlicerCapOption::UseLastSectionForCap)
			{
				OutOtherHalf->UpdateMeshSection(OtherCapSectionIndex, MoveTemp(OtherCapSection));
			}
			else
			{
				OutOtherHalf->CreateMeshSection(OtherCapSectionIndex, MoveTemp(OtherCapSection));
			}
		}
	}
	return NewCapSectionIndex;
}


void URuntimeMeshSlicer::SliceRuntimeMesh(URuntimeMesh* InRuntimeMesh, FVector PlanePosition, FVector PlaneNormal, URuntimeMesh* OutOtherHalf, ERuntimeMeshSlicerCapOption CapOption, UMaterialInterface* CapMaterial)
{
	if (InRuntimeMesh)
	{
		FRuntimeMeshDataPtr InMeshData = InRuntimeMesh->GetRuntimeMeshData();
		FRuntimeMeshDataPtr OtherHalfData = OutOtherHalf ? OutOtherHalf->GetRuntimeMeshData() : FRuntimeMeshDataPtr(nullptr);


		PlaneNormal.Normalize();
		FPlane SlicePlane(PlanePosition, PlaneNormal);

		bool bShouldCreateOtherHalf = OutOtherHalf != nullptr;

		// Set of new edges created by clipping polys by plane
		TArray<FUtilEdge3D> ClipEdges;

		for (int32 SectionIndex = 0; SectionIndex < InRuntimeMesh->GetNumSections(); SectionIndex++)
		{
			// Skip if the section doesn't exist
			if (!InRuntimeMesh->DoesSectionExist(SectionIndex))
			{
				continue;
			}

			auto MeshData = InRuntimeMesh->GetSectionReadonly(SectionIndex);

			// Skip if we don't have mesh data
			if (MeshData->NumVertices() < 3 || MeshData->NumIndices() < 3)
			{
				continue;
			}

			// Compare bounding box of section with slicing plane
			int32 BoxCompare = CompareBoxPlane(InRuntimeMesh->GetSectionBoundingBox(SectionIndex), SlicePlane);

			if (BoxCompare == 1)
			{
				// Box totally on the close side of the plane, do nothing
				continue;
			}

			if (BoxCompare == -1)
			{
				// Box totally on the far side of the plane, move the entire section to the other RMC if it exists
				if (bShouldCreateOtherHalf)
				{
					auto SourceMeshData = InRuntimeMesh->GetSectionReadonly(SectionIndex);

					auto NewBuilder = MakeRuntimeMeshBuilder(*SourceMeshData.Get());
					SourceMeshData->CopyTo(NewBuilder);

					OutOtherHalf->CreateMeshSection(SectionIndex, MoveTemp(NewBuilder));
					OutOtherHalf->SetSectionMaterial(SectionIndex, InRuntimeMesh->GetSectionMaterial(SectionIndex));
				}

				InRuntimeMesh->ClearMeshSection(SectionIndex);
				continue;
			}

			check(BoxCompare == 0);

			UMaterialInterface* Material = InRuntimeMesh->GetSectionMaterial(SectionIndex);

			SliceRuntimeMeshSection(InMeshData, OtherHalfData, SectionIndex, SlicePlane, ClipEdges);

			if (bShouldCreateOtherHalf && OtherHalfData->DoesSectionExist(SectionIndex))
			{
				OutOtherHalf->SetSectionMaterial(SectionIndex, Material);
			}
		}

		int32 CapSectionIndex = CapMeshSlice(InMeshData, OtherHalfData, ClipEdges, SlicePlane, PlaneNormal, CapOption);

		if (CapOption == ERuntimeMeshSlicerCapOption::CreateNewSectionForCap)
		{
			InRuntimeMesh->SetSectionMaterial(CapSectionIndex, CapMaterial);
			if (bShouldCreateOtherHalf)
			{
				OutOtherHalf->SetSectionMaterial(CapSectionIndex, CapMaterial);
			}
		}
		
		// Slice the collision
		SliceRuntimeMeshConvexCollision(InRuntimeMesh, OutOtherHalf, PlanePosition, PlaneNormal);
	}
}

void URuntimeMeshSlicer::SliceRuntimeMeshComponent(URuntimeMeshComponent* InRuntimeMesh, FVector PlanePosition, FVector PlaneNormal, bool bCreateOtherHalf, URuntimeMeshComponent*& OutOtherHalf, ERuntimeMeshSlicerCapOption CapOption, UMaterialInterface* CapMaterial)
{
	if (InRuntimeMesh && InRuntimeMesh->GetRuntimeMesh())
	{
		// Transform plane from world to local space
		FTransform ComponentToWorld = InRuntimeMesh->GetComponentToWorld();
		FVector LocalPlanePos = ComponentToWorld.InverseTransformPosition(PlanePosition);
		FVector LocalPlaneNormal = ComponentToWorld.InverseTransformVectorNoScale(PlaneNormal);
		LocalPlaneNormal = LocalPlaneNormal.GetSafeNormal(); // Ensure normalized


		if (bCreateOtherHalf)
		{
			OutOtherHalf = NewObject<URuntimeMeshComponent>(InRuntimeMesh->GetOuter());
			OutOtherHalf->SetWorldTransform(InRuntimeMesh->GetComponentTransform());
		}

		SliceRuntimeMesh(InRuntimeMesh->GetRuntimeMesh(), LocalPlanePos, LocalPlaneNormal, bCreateOtherHalf ? OutOtherHalf->GetOrCreateRuntimeMesh() : nullptr, CapOption, CapMaterial);


		if (bCreateOtherHalf)
		{
			if (OutOtherHalf->GetNumSections() > 0)
			{
				OutOtherHalf->SetCollisionProfileName(InRuntimeMesh->GetCollisionProfileName());
				OutOtherHalf->SetCollisionEnabled(InRuntimeMesh->GetCollisionEnabled());
				OutOtherHalf->SetCollisionUseComplexAsSimple(InRuntimeMesh->IsCollisionUsingComplexAsSimple());

				// Copy overridden materials
				for (int32 Index = 0; Index < InRuntimeMesh->GetNumOverrideMaterials(); Index++)
				{
					if (UMaterialInterface* Material = InRuntimeMesh->GetOverrideMaterial(Index))
					{
						OutOtherHalf->SetMaterial(Index, Material);
					}
				}

				OutOtherHalf->RegisterComponent();
			}
			else
			{
				OutOtherHalf->DestroyComponent();
			}
		}

	}
}
