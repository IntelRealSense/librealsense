// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshLibrary.h"
#include "RuntimeMeshComponentPlugin.h"
#include "RuntimeMeshComponent.h"
#include "EngineGlobals.h"
#include "Logging/TokenizedMessage.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#include "StaticMeshResources.h"
#include "Engine/StaticMesh.h"
#include "RuntimeMeshInternalUtilities.h"
#include "RuntimeMeshTessellationUtilities.h"
#include "PhysicsEngine/BodySetup.h"

#define LOCTEXT_NAMESPACE "RuntimeMeshLibrary"

DECLARE_CYCLE_STAT(TEXT("RML - Copy Static Mesh to Runtime Mesh"), STAT_RuntimeMeshLibrary_CopyStaticMeshToRuntimeMesh, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RML - Calculate Tangents For Mesh"), STAT_RuntimeMeshLibrary_CalculateTangentsForMesh, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RML - Get Static Mesh Section"), STAT_RuntimeMeshLibrary_GetStaticMeshSection, STATGROUP_RuntimeMesh);


void URuntimeMeshLibrary::CalculateTangentsForMesh(const TArray<FVector>& Vertices, const TArray<int32>& Triangles, TArray<FVector>& Normals, 
	const TArray<FVector2D>& UVs, TArray<FRuntimeMeshTangent>& Tangents, bool bCreateSmoothNormals)
{
	Normals.SetNum(Vertices.Num());
	Tangents.SetNum(Vertices.Num());
	CalculateTangentsForMesh(
		[&Triangles](int32 Index) -> int32 { return Triangles[Index]; },
		[&Vertices](int32 Index) -> FVector { return Vertices[Index]; },
		[&UVs](int32 Index) -> FVector2D { return UVs[Index]; },
		[&Normals, &Tangents](int32 Index, FVector TangentX, FVector TangentY, FVector TangentZ)
		{
			Normals[Index] = TangentZ;
			Tangents[Index].TangentX = TangentX;
			Tangents[Index].bFlipTangentY = GetBasisDeterminantSign(TangentX, TangentY, TangentZ) < 0;
		},
		Vertices.Num(), UVs.Num(), Triangles.Num(), bCreateSmoothNormals);
}

void URuntimeMeshLibrary::CalculateTangentsForMeshPacked(TArray<FRuntimeMeshBlueprintVertexSimple>& Vertices, const TArray<int32>& Triangles, bool bCreateSmoothNormals)
{
	CalculateTangentsForMesh(
		[&Triangles](int32 Index) -> int32 { return Triangles[Index]; },
		[&Vertices](int32 Index) -> FVector { return Vertices[Index].Position; },
		[&Vertices](int32 Index) -> FVector2D { return Vertices[Index].UV0; },
		[&Vertices](int32 Index, FVector TangentX, FVector TangentY, FVector TangentZ)
		{
			Vertices[Index].Normal = TangentZ;
			Vertices[Index].Tangent.TangentX = TangentX;
			Vertices[Index].Tangent.bFlipTangentY = GetBasisDeterminantSign(TangentX, TangentY, TangentZ) < 0;
		},
		Vertices.Num(), Vertices.Num(), Triangles.Num(), bCreateSmoothNormals);
}

void URuntimeMeshLibrary::CalculateTangentsForMesh(TArray<FRuntimeMeshVertexSimple>& Vertices, const TArray<int32>& Triangles, bool bCreateSmoothNormals)
{
	CalculateTangentsForMesh(
		[&Triangles](int32 Index) -> int32 { return Triangles[Index]; },
		[&Vertices](int32 Index) -> FVector { return Vertices[Index].Position; },
		[&Vertices](int32 Index) -> FVector2D { return Vertices[Index].UV0; },
		[&Vertices](int32 Index, FVector TangentX, FVector TangentY, FVector TangentZ)
		{
			Vertices[Index].Normal = TangentZ;
			Vertices[Index].SetTangents(TangentX, TangentY, TangentZ);
		},
		Vertices.Num(), Vertices.Num(), Triangles.Num(), bCreateSmoothNormals);
}

void URuntimeMeshLibrary::CalculateTangentsForMesh(const TSharedPtr<FRuntimeMeshAccessor>& MeshAccessor, bool bCreateSmoothNormals)
{
	CalculateTangentsForMesh(
		[MeshAccessor](int32 Index) -> int32 { return MeshAccessor->GetIndex(Index); },
		[MeshAccessor](int32 Index) -> FVector { return MeshAccessor->GetPosition(Index); },
		[MeshAccessor](int32 Index) -> FVector2D { return MeshAccessor->GetUV(Index); },
		[MeshAccessor](int32 Index, FVector TangentX, FVector TangentY, FVector TangentZ)
		{
			MeshAccessor->SetTangents(Index, TangentX, TangentY, TangentZ);
		},
		MeshAccessor->NumVertices(), MeshAccessor->NumVertices(), MeshAccessor->NumIndices(), bCreateSmoothNormals);
}




TArray<int32> URuntimeMeshLibrary::GenerateTessellationIndexBuffer(const TArray<FVector>& Vertices, const TArray<int32>& Triangles, TArray<FVector>& Normals, const TArray<FVector2D>& UVs, TArray<FRuntimeMeshTangent>& Tangents)
{
	TArray<int32> TessellationTriangles;
	FTessellationUtilities::CalculateTessellationIndices(Vertices.Num(), Triangles.Num(),
		[&Vertices](int32 Index) -> FVector { return Vertices[Index]; },
		[&UVs](int32 Index) -> FVector2D { return UVs.Num() > Index ? UVs[Index] : FVector2D::ZeroVector; },
		[&Triangles](int32 Index) -> int32 { return Triangles[Index]; },
		[&TessellationTriangles](int32 NewSize) { TessellationTriangles.SetNum(NewSize); },
		[&TessellationTriangles]() -> int32 { return TessellationTriangles.Num(); },
		[&TessellationTriangles](int32 Index, int32 NewValue) { TessellationTriangles[Index] = NewValue; },
		[&TessellationTriangles](int32 Index) -> int32 { return TessellationTriangles[Index]; });
	return TessellationTriangles;
}

TArray<int32> URuntimeMeshLibrary::GenerateTessellationIndexBufferPacked(const TArray<FRuntimeMeshBlueprintVertexSimple>& Vertices, const TArray<int32>& Triangles)
{
	TArray<int32> TessellationTriangles;
	FTessellationUtilities::CalculateTessellationIndices(Vertices.Num(), Triangles.Num(),
		[&Vertices](int32 Index) -> FVector { return Vertices[Index].Position; },
		[&Vertices](int32 Index) -> FVector2D { return Vertices[Index].UV0; },
		[&Triangles](int32 Index) -> int32 { return Triangles[Index]; },
		[&TessellationTriangles](int32 NewSize) { TessellationTriangles.SetNum(NewSize); },
		[&TessellationTriangles]() -> int32 { return TessellationTriangles.Num(); },
		[&TessellationTriangles](int32 Index, int32 NewValue) { TessellationTriangles[Index] = NewValue; },
		[&TessellationTriangles](int32 Index) -> int32 { return TessellationTriangles[Index]; });
	return TessellationTriangles;
}

TArray<int32> URuntimeMeshLibrary::GenerateTessellationIndexBuffer(const TArray<FRuntimeMeshVertexSimple>& Vertices, const TArray<int32>& Triangles)
{
	TArray<int32> TessellationTriangles;
	FTessellationUtilities::CalculateTessellationIndices(Vertices.Num(), Triangles.Num(),
		[&Vertices](int32 Index) -> FVector { return Vertices[Index].Position; },
		[&Vertices](int32 Index) -> FVector2D { return Vertices[Index].UV0; },
		[&Triangles](int32 Index) -> int32 { return Triangles[Index]; },
		[&TessellationTriangles](int32 NewSize) { TessellationTriangles.SetNum(NewSize); },
		[&TessellationTriangles]() -> int32 { return TessellationTriangles.Num(); },
		[&TessellationTriangles](int32 Index, int32 NewValue) { TessellationTriangles[Index] = NewValue; },
		[&TessellationTriangles](int32 Index) -> int32 { return TessellationTriangles[Index]; });
	return TessellationTriangles;
}

TArray<int32> URuntimeMeshLibrary::GenerateTessellationIndexBuffer(const TSharedPtr<FRuntimeMeshAccessor>& MeshAccessor)
{
	TArray<int32> TessellationTriangles;
	FTessellationUtilities::CalculateTessellationIndices(MeshAccessor->NumVertices(), MeshAccessor->NumIndices(),
		[&MeshAccessor](int32 Index) -> FVector { return MeshAccessor->GetPosition(Index); },
		[&MeshAccessor](int32 Index) -> FVector2D { return MeshAccessor->GetUV(Index); },
		[&MeshAccessor](int32 Index) -> int32 { return MeshAccessor->GetIndex(Index); },
		[&TessellationTriangles](int32 NewSize) { TessellationTriangles.SetNum(NewSize); },
		[&TessellationTriangles]() -> int32 { return TessellationTriangles.Num(); },
		[&TessellationTriangles](int32 Index, int32 NewValue) { TessellationTriangles[Index] = NewValue; },
		[&TessellationTriangles](int32 Index) -> int32 { return TessellationTriangles[Index]; });
	return TessellationTriangles;
}

void URuntimeMeshLibrary::GenerateTessellationIndexBuffer(const TSharedPtr<FRuntimeMeshAccessor>& MeshAccessor, const TSharedPtr<FRuntimeMeshIndicesAccessor>& OutTessellationIndices)
{
	OutTessellationIndices->EmptyIndices();
	FTessellationUtilities::CalculateTessellationIndices(MeshAccessor->NumVertices(), MeshAccessor->NumIndices(),
		[&MeshAccessor](int32 Index) -> FVector { return MeshAccessor->GetPosition(Index); },
		[&MeshAccessor](int32 Index) -> FVector2D { return MeshAccessor->GetUV(Index); },
		[&MeshAccessor](int32 Index) -> int32 { return MeshAccessor->GetIndex(Index); },
		[&OutTessellationIndices](int32 NewSize) { OutTessellationIndices->SetNumIndices(NewSize); },
		[&OutTessellationIndices]() -> int32 { return OutTessellationIndices->NumIndices(); },
		[&OutTessellationIndices](int32 Index, int32 NewValue) { OutTessellationIndices->SetIndex(Index, NewValue); },
		[&OutTessellationIndices](int32 Index) -> int32 { return OutTessellationIndices->GetIndex(Index); });
}




void URuntimeMeshLibrary::GetStaticMeshSection(UStaticMesh* InMesh, int32 LODIndex, int32 SectionIndex, TArray<FVector>& Vertices, TArray<int32>& Triangles,
	TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FColor>& Colors, TArray<FRuntimeMeshTangent>& Tangents)
{
	Vertices.Empty();
	Triangles.Empty();
	Normals.Empty();
	UVs.Empty();
	Colors.Empty();
	Tangents.Empty();

	GetStaticMeshSection(InMesh, LODIndex, SectionIndex, 1,
		[&Vertices, &Normals, &Tangents](FVector Position, FVector TangentX, FVector TangentY, FVector TangentZ) -> int32
		{
			int32 NewIndex = Vertices.Add(Position);
			Normals.Add(TangentZ);
			Tangents.Add(FRuntimeMeshTangent(TangentX, GetBasisDeterminantSign(TangentX, TangentY, TangentZ) < 0));
			return NewIndex;
		},
		[&UVs](int32 Index, int32 UVIndex, FVector2D UV)
		{
			check(UVIndex == 0);
			UVs.SetNum(Index + 1);
			UVs[Index] = UV;
		},
		[&Colors](int32 Index, FColor Color)
		{
			Colors.SetNum(Index + 1);
			Colors[Index] = Color;
		},
		[&Triangles](int32 Index)
		{
			Triangles.Add(Index);
		},
		[](int32 Index)
		{
		});
}

void URuntimeMeshLibrary::GetStaticMeshSectionPacked(UStaticMesh* InMesh, int32 LODIndex, int32 SectionIndex, TArray<FRuntimeMeshBlueprintVertexSimple>& Vertices, TArray<int32>& Triangles)
{
	Vertices.Empty();
	Triangles.Empty();

	GetStaticMeshSection(InMesh, LODIndex, SectionIndex, 1,
		[&Vertices](FVector Position, FVector TangentX, FVector TangentY, FVector TangentZ) -> int32
		{
			return Vertices.Add(FRuntimeMeshBlueprintVertexSimple(Position, TangentZ, FRuntimeMeshTangent(TangentX, GetBasisDeterminantSign(TangentX, TangentY, TangentZ) < 0), FVector2D::ZeroVector));
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
}

void URuntimeMeshLibrary::GetStaticMeshSection(UStaticMesh* InMesh, int32 LODIndex, int32 SectionIndex, TArray<FRuntimeMeshVertexSimple>& Vertices, TArray<int32>& Triangles)
{
	Vertices.Empty();
	Triangles.Empty();

	GetStaticMeshSection(InMesh, LODIndex, SectionIndex, 1,
		[&Vertices](FVector Position, FVector TangentX, FVector TangentY, FVector TangentZ) -> int32
		{
			return Vertices.Add(FRuntimeMeshVertexSimple(Position, TangentX, TangentY, TangentZ));
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
}

void URuntimeMeshLibrary::GetStaticMeshSection(UStaticMesh* InMesh, int32 LODIndex, int32 SectionIndex, TArray<FRuntimeMeshVertexSimple>& Vertices, TArray<int32>& Triangles, TArray<int32>& AdjacencyTriangles)
{
	Vertices.Empty();
	Triangles.Empty();
	AdjacencyTriangles.Empty();

	GetStaticMeshSection(InMesh, LODIndex, SectionIndex, 1,
		[&Vertices](FVector Position, FVector TangentX, FVector TangentY, FVector TangentZ) -> int32
		{
			return Vertices.Add(FRuntimeMeshVertexSimple(Position, TangentX, TangentY, TangentZ));
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
		[&AdjacencyTriangles](int32 Index)
		{
			AdjacencyTriangles.Add(Index);
		});
}

void URuntimeMeshLibrary::GetStaticMeshSection(UStaticMesh* InMesh, int32 LODIndex, int32 SectionIndex, const TSharedPtr<FRuntimeMeshAccessor>& MeshAccessor)
{
	MeshAccessor->EmptyVertices();
	MeshAccessor->EmptyIndices();

	GetStaticMeshSection(InMesh, LODIndex, SectionIndex, MeshAccessor->NumUVChannels(),
		[&MeshAccessor](FVector Position, FVector TangentX, FVector TangentY, FVector TangentZ) -> int32
		{
			int32 NewIndex = MeshAccessor->AddVertex(Position);
			MeshAccessor->SetTangents(NewIndex, TangentX, TangentY, TangentZ);
			return NewIndex;
		},
		[&MeshAccessor](int32 Index, int32 UVIndex, FVector2D UV)
		{
			MeshAccessor->SetUV(Index, UVIndex, UV);
		},
		[&MeshAccessor](int32 Index, FColor Color)
		{
			MeshAccessor->SetColor(Index, Color);
		},
		[&MeshAccessor](int32 Index)
		{
			MeshAccessor->AddIndex(Index);
		},
		[](int32 Index)
		{
		});
}

void URuntimeMeshLibrary::GetStaticMeshSection(UStaticMesh* InMesh, int32 LODIndex, int32 SectionIndex, const TSharedPtr<FRuntimeMeshAccessor>& MeshAccessor, const TSharedPtr<FRuntimeMeshIndicesAccessor>& TessellationIndicesAccessor)
{
	MeshAccessor->EmptyVertices();
	MeshAccessor->EmptyIndices();
	TessellationIndicesAccessor->EmptyIndices();

	GetStaticMeshSection(InMesh, LODIndex, SectionIndex, MeshAccessor->NumUVChannels(),
		[&MeshAccessor](FVector Position, FVector TangentX, FVector TangentY, FVector TangentZ) -> int32
		{
			int32 NewIndex = MeshAccessor->AddVertex(Position);
			MeshAccessor->SetTangents(NewIndex, TangentX, TangentY, TangentZ);
			return NewIndex;
		},
		[&MeshAccessor](int32 Index, int32 UVIndex, FVector2D UV)
		{
			MeshAccessor->SetUV(Index, UVIndex, UV);
		},
		[&MeshAccessor](int32 Index, FColor Color)
		{
			MeshAccessor->SetColor(Index, Color);
		},
		[&MeshAccessor](int32 Index)
		{
			MeshAccessor->AddIndex(Index);
		},
		[&TessellationIndicesAccessor](int32 Index)
		{
			TessellationIndicesAccessor->AddIndex(Index);
		});
}





void URuntimeMeshLibrary::CopyStaticMeshToRuntimeMesh(UStaticMeshComponent* StaticMeshComponent, int32 LODIndex, URuntimeMesh* RuntimeMesh, bool bCreateCollision)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshLibrary_CopyStaticMeshToRuntimeMesh);
	if (StaticMeshComponent != nullptr &&
		StaticMeshComponent->GetStaticMesh() != nullptr &&
		RuntimeMesh != nullptr)
	{
		UStaticMesh* StaticMesh = StaticMeshComponent->GetStaticMesh();

		RuntimeMesh->ClearAllMeshSections();

		//// MESH DATA

		int32 NumSections = StaticMesh->GetNumSections(LODIndex);
		for (int32 SectionIndex = 0; SectionIndex < NumSections; SectionIndex++)
		{
			// Buffers for copying geom data
			TSharedPtr<FRuntimeMeshBuilder> MeshData = MakeRuntimeMeshBuilder<FRuntimeMeshTangents, FVector2DHalf, uint32>();
			TArray<uint8> AdjacencyTrianglesData;
			TSharedPtr<FRuntimeMeshIndicesAccessor> AdjacencyTrianglesAccessor = MakeShared<FRuntimeMeshIndicesAccessor>(true, &AdjacencyTrianglesData);
			TArray<uint32> AdjacencyTriangles;
			AdjacencyTriangles.SetNum(AdjacencyTrianglesData.Num() / 4);
			FMemory::Memcpy(AdjacencyTriangles.GetData(), AdjacencyTrianglesData.GetData(), AdjacencyTrianglesData.Num());


			// TODO: Make this smarter to setup a mesh section to match the static mesh vertex configuration
			// This will let it pick up the additional UVs or high pri normals/tangents/uvs

			// Get geom data from static mesh
			GetStaticMeshSection(StaticMesh, LODIndex, SectionIndex, MeshData, AdjacencyTrianglesAccessor);

			// Create RuntimeMesh
			RuntimeMesh->CreateMeshSection(SectionIndex, MeshData, bCreateCollision);
			RuntimeMesh->SetSectionTessellationTriangles(SectionIndex, AdjacencyTriangles);
		}

		//// SIMPLE COLLISION
		CopyCollisionFromStaticMesh(StaticMesh, RuntimeMesh);


		//// MATERIALS

		for (int32 MatIndex = 0; MatIndex < StaticMeshComponent->GetNumMaterials(); MatIndex++)
		{
			RuntimeMesh->SetSectionMaterial(MatIndex, StaticMeshComponent->GetMaterial(MatIndex));
		}
	}
}


void URuntimeMeshLibrary::CopyStaticMeshToRuntimeMeshComponent(UStaticMeshComponent* StaticMeshComponent, int32 LODIndex, URuntimeMeshComponent* RuntimeMeshComponent, bool bCreateCollision)
{
	CopyStaticMeshToRuntimeMesh(StaticMeshComponent, LODIndex, RuntimeMeshComponent->GetOrCreateRuntimeMesh(), bCreateCollision);
}

void URuntimeMeshLibrary::CopyCollisionFromStaticMesh(UStaticMesh* StaticMesh, URuntimeMesh* RuntimeMesh)
{
	// Clear any existing collision hulls
	RuntimeMesh->ClearAllConvexCollisionSections();

	if (StaticMesh->BodySetup != nullptr)
	{
		// Iterate over all convex hulls on static mesh..
		const int32 NumConvex = StaticMesh->BodySetup->AggGeom.ConvexElems.Num();
		for (int ConvexIndex = 0; ConvexIndex < NumConvex; ConvexIndex++)
		{
			// Copy convex verts
			FKConvexElem& MeshConvex = StaticMesh->BodySetup->AggGeom.ConvexElems[ConvexIndex];

			RuntimeMesh->AddConvexCollisionSection(MeshConvex.VertexData);
		}
	}
}

void URuntimeMeshLibrary::CalculateTangentsForMesh(TFunction<int32(int32 Index)> IndexAccessor, TFunction<FVector(int32 Index)> VertexAccessor, TFunction<FVector2D(int32 Index)> UVAccessor,
	TFunction<void(int32 Index, FVector TangentX, FVector TangentY, FVector TangentZ)> TangentSetter, int32 NumVertices, int32 NumUVs, int32 NumIndices, bool bCreateSmoothNormals)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshLibrary_CalculateTangentsForMesh);

	if (NumVertices == 0 || NumIndices == 0)
	{
		return;
	}

	// Calculate the duplicate vertices map if we're wanting smooth normals.  Don't find duplicates if we don't want smooth normals
	// that will cause it to only smooth across faces sharing a common vertex, not across faces with vertices of common position
	const TMultiMap<uint32, uint32> DuplicateVertexMap = bCreateSmoothNormals ? FRuntimeMeshInternalUtilities::FindDuplicateVerticesMap(VertexAccessor, NumVertices) : TMultiMap<uint32, uint32>();


	// Number of triangles
	const int32 NumTris = NumIndices / 3;

	// Map of vertex to triangles in Triangles array
	TMultiMap<uint32, uint32> VertToTriMap;
	// Map of vertex to triangles to consider for normal calculation
	TMultiMap<uint32, uint32> VertToTriSmoothMap;

	// Normal/tangents for each face
	TArray<FVector> FaceTangentX, FaceTangentY, FaceTangentZ;
	FaceTangentX.AddUninitialized(NumTris);
	FaceTangentY.AddUninitialized(NumTris);
	FaceTangentZ.AddUninitialized(NumTris);

	// Iterate over triangles
	for (int TriIdx = 0; TriIdx < NumTris; TriIdx++)
	{
		uint32 CornerIndex[3];
		FVector P[3];

		for (int32 CornerIdx = 0; CornerIdx < 3; CornerIdx++)
		{
			// Find vert index (clamped within range)
			uint32 VertIndex = FMath::Min(IndexAccessor((TriIdx * 3) + CornerIdx), NumVertices - 1);

			CornerIndex[CornerIdx] = VertIndex;
			P[CornerIdx] = VertexAccessor(VertIndex);

			// Find/add this vert to index buffer
			TArray<uint32> VertOverlaps;
			DuplicateVertexMap.MultiFind(VertIndex, VertOverlaps);

			// Remember which triangles map to this vert
			VertToTriMap.AddUnique(VertIndex, TriIdx);
			VertToTriSmoothMap.AddUnique(VertIndex, TriIdx);

			// Also update map of triangles that 'overlap' this vert (ie don't match UV, but do match smoothing) and should be considered when calculating normal
			for (int32 OverlapIdx = 0; OverlapIdx < VertOverlaps.Num(); OverlapIdx++)
			{
				// For each vert we overlap..
				int32 OverlapVertIdx = VertOverlaps[OverlapIdx];

				// Add this triangle to that vert
				VertToTriSmoothMap.AddUnique(OverlapVertIdx, TriIdx);

				// And add all of its triangles to us
				TArray<uint32> OverlapTris;
				VertToTriMap.MultiFind(OverlapVertIdx, OverlapTris);
				for (int32 OverlapTriIdx = 0; OverlapTriIdx < OverlapTris.Num(); OverlapTriIdx++)
				{
					VertToTriSmoothMap.AddUnique(VertIndex, OverlapTris[OverlapTriIdx]);
				}
			}
		}

		// Calculate triangle edge vectors and normal
		const FVector Edge21 = P[1] - P[2];
		const FVector Edge20 = P[0] - P[2];
		const FVector TriNormal = (Edge21 ^ Edge20).GetSafeNormal();

		// If we have UVs, use those to calculate
		if (NumUVs == NumVertices)
		{
			const FVector2D T1 = UVAccessor(CornerIndex[0]);
			const FVector2D T2 = UVAccessor(CornerIndex[1]);
			const FVector2D T3 = UVAccessor(CornerIndex[2]);

// 			float X1 = P[1].X - P[0].X;
// 			float X2 = P[2].X - P[0].X;
// 			float Y1 = P[1].Y - P[0].Y;
// 			float Y2 = P[2].Y - P[0].Y;
// 			float Z1 = P[1].Z - P[0].Z;
// 			float Z2 = P[2].Z - P[0].Z;
// 
// 			float S1 = U1.X - U0.X;
// 			float S2 = U2.X - U0.X;
// 			float T1 = U1.Y - U0.Y;
// 			float T2 = U2.Y - U0.Y;
// 
// 			float R = 1.0f / (S1 * T2 - S2 * T1);
// 			FaceTangentX[TriIdx] = FVector((T2 * X1 - T1 * X2) * R, (T2 * Y1 - T1 * Y2) * R,
// 				(T2 * Z1 - T1 * Z2) * R);
// 			FaceTangentY[TriIdx] = FVector((S1 * X2 - S2 * X1) * R, (S1 * Y2 - S2 * Y1) * R,
// 				(S1 * Z2 - S2 * Z1) * R);




			FMatrix	ParameterToLocal(
				FPlane(P[1].X - P[0].X, P[1].Y - P[0].Y, P[1].Z - P[0].Z, 0),
				FPlane(P[2].X - P[0].X, P[2].Y - P[0].Y, P[2].Z - P[0].Z, 0),
				FPlane(P[0].X, P[0].Y, P[0].Z, 0),
				FPlane(0, 0, 0, 1)
			);

			FMatrix ParameterToTexture(
				FPlane(T2.X - T1.X, T2.Y - T1.Y, 0, 0),
				FPlane(T3.X - T1.X, T3.Y - T1.Y, 0, 0),
				FPlane(T1.X, T1.Y, 1, 0),
				FPlane(0, 0, 0, 1)
			);

			// Use InverseSlow to catch singular matrices.  Inverse can miss this sometimes.
			const FMatrix TextureToLocal = ParameterToTexture.Inverse() * ParameterToLocal;

			FaceTangentX[TriIdx] = TextureToLocal.TransformVector(FVector(1, 0, 0)).GetSafeNormal();
			FaceTangentY[TriIdx] = TextureToLocal.TransformVector(FVector(0, 1, 0)).GetSafeNormal();
		}
		else
		{
			FaceTangentX[TriIdx] = Edge20.GetSafeNormal();
			FaceTangentY[TriIdx] = (FaceTangentX[TriIdx] ^ TriNormal).GetSafeNormal();
		}

		FaceTangentZ[TriIdx] = TriNormal;
	}


	// Arrays to accumulate tangents into
	TArray<FVector> VertexTangentXSum, VertexTangentYSum, VertexTangentZSum;
	VertexTangentXSum.AddZeroed(NumVertices);
	VertexTangentYSum.AddZeroed(NumVertices);
	VertexTangentZSum.AddZeroed(NumVertices);

	// For each vertex..
	for (int VertxIdx = 0; VertxIdx < NumVertices; VertxIdx++)
	{
		// Find relevant triangles for normal
		TArray<uint32> SmoothTris;
		VertToTriSmoothMap.MultiFind(VertxIdx, SmoothTris);

		for (int i = 0; i < SmoothTris.Num(); i++)
		{
			uint32 TriIdx = SmoothTris[i];
			VertexTangentZSum[VertxIdx] += FaceTangentZ[TriIdx];
		}

		// Find relevant triangles for tangents
		TArray<uint32> TangentTris;
		VertToTriMap.MultiFind(VertxIdx, TangentTris);

		for (int i = 0; i < TangentTris.Num(); i++)
		{
			uint32 TriIdx = TangentTris[i];
			VertexTangentXSum[VertxIdx] += FaceTangentX[TriIdx];
			VertexTangentYSum[VertxIdx] += FaceTangentY[TriIdx];
		}
	}

	// Finally, normalize tangents and build output arrays	
	for (int VertxIdx = 0; VertxIdx < NumVertices; VertxIdx++)
	{
		FVector& TangentX = VertexTangentXSum[VertxIdx];
		FVector& TangentY = VertexTangentYSum[VertxIdx];
		FVector& TangentZ = VertexTangentZSum[VertxIdx];

		TangentX.Normalize();
		//TangentY.Normalize();
		TangentZ.Normalize();

		// Use Gram-Schmidt orthogonalization to make sure X is orthonormal with Z
		TangentX -= TangentZ * (TangentZ | TangentX);
		TangentX.Normalize();
		TangentY.Normalize();


		TangentSetter(VertxIdx, TangentX, TangentY, TangentZ);
	}
}

int32 URuntimeMeshLibrary::GetNewIndexForOldVertIndex(const FPositionVertexBuffer* PosBuffer, const FStaticMeshVertexBuffer* VertBuffer, const FColorVertexBuffer* ColorBuffer,
	TMap<int32, int32>& MeshToSectionVertMap, int32 VertexIndex, int32 NumUVChannels, TFunction<int32(FVector Position, FVector TangentX, FVector TangentY, FVector TangentZ)> VertexCreator,
	TFunction<void(int32 VertexIndex, int32 UVIndex, FVector2D UV)> UVSetter, TFunction<void(int32 VertexIndex, FColor Color)> ColorSetter)
{
	int32* FoundIndex = MeshToSectionVertMap.Find(VertexIndex);
	if (FoundIndex != nullptr)
	{
		return *FoundIndex;
	}
	else
	{
		int32 NewVertexIndex = VertexCreator(
			PosBuffer->VertexPosition(VertexIndex),
			VertBuffer->VertexTangentX(VertexIndex),
			VertBuffer->VertexTangentY(VertexIndex),
			VertBuffer->VertexTangentZ(VertexIndex));

		if (ColorBuffer && ColorBuffer->GetNumVertices() > (uint32)NewVertexIndex)
		{
			ColorSetter(NewVertexIndex, ColorBuffer->VertexColor(VertexIndex));
		}

		int32 NumTexCoordsToCopy = FMath::Min(NumUVChannels, (int32)VertBuffer->GetNumTexCoords());
		for (int32 Index = 0; Index < NumTexCoordsToCopy; Index++)
		{
			UVSetter(NewVertexIndex, Index, VertBuffer->GetVertexUV(VertexIndex, Index));
		}

		MeshToSectionVertMap.Add(VertexIndex, NewVertexIndex);
		return NewVertexIndex;
	}
}

void URuntimeMeshLibrary::GetStaticMeshSection(UStaticMesh* InMesh, int32 LODIndex, int32 SectionIndex, int32 NumSupportedUVs,
	TFunction<int32(FVector Position, FVector TangentX, FVector TangentY, FVector TangentZ)> VertexCreator,
	TFunction<void(int32 VertexIndex, int32 UVIndex, FVector2D UV)> UVSetter, TFunction<void(int32 VertexIndex, FColor Color)> ColorSetter, TFunction<void(int32)> IndexCreator, TFunction<void(int32)> AdjacencyIndexCreator)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshLibrary_GetStaticMeshSection);

	if (InMesh != nullptr)
	{
		// Log a PIE warning if this will fail in a cooked build
		if (!InMesh->bAllowCPUAccess)
		{
			FMessageLog("PIE").Warning()
				->AddToken(FTextToken::Create(LOCTEXT("GetSectionFromStaticMeshStart", "Calling GetSectionFromStaticMesh on")))
				->AddToken(FUObjectToken::Create(InMesh))
				->AddToken(FTextToken::Create(LOCTEXT("GetSectionFromStaticMeshEnd", "but 'Allow CPU Access' is not enabled. This is required for converting StaticMesh to ProceduralMeshComponent in cooked builds.")));
		}

		if ((InMesh->bAllowCPUAccess || GIsEditor) && InMesh->RenderData != nullptr && InMesh->RenderData->LODResources.IsValidIndex(LODIndex))
		{
			const FStaticMeshLODResources& LOD = InMesh->RenderData->LODResources[LODIndex];
			if (LOD.Sections.IsValidIndex(SectionIndex))
			{
				// Map from vert buffer for whole mesh to vert buffer for section of interest
				TMap<int32, int32> MeshToSectionVertMap;

				const FStaticMeshSection& Section = LOD.Sections[SectionIndex];
				const uint32 OnePastLastIndex = Section.FirstIndex + Section.NumTriangles * 3;
				FIndexArrayView Indices = LOD.IndexBuffer.GetArrayView();

				// Iterate over section index buffer, copying verts as needed
				for (uint32 i = Section.FirstIndex; i < OnePastLastIndex; i++)
				{
					uint32 MeshVertIndex = Indices[i];

					// See if we have this vert already in our section vert buffer, and copy vert in if not 

#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 19
					int32 SectionVertIndex = GetNewIndexForOldVertIndex(&LOD.VertexBuffers.PositionVertexBuffer, &LOD.VertexBuffers.StaticMeshVertexBuffer, &LOD.VertexBuffers.ColorVertexBuffer, MeshToSectionVertMap, MeshVertIndex, NumSupportedUVs, VertexCreator, UVSetter, ColorSetter);
#else
					int32 SectionVertIndex = GetNewIndexForOldVertIndex(&LOD.PositionVertexBuffer, &LOD.VertexBuffer, &LOD.ColorVertexBuffer, MeshToSectionVertMap, MeshVertIndex, NumSupportedUVs, VertexCreator, UVSetter, ColorSetter);
#endif

					// Add to index buffer
					IndexCreator(SectionVertIndex);
				}

				// Lets copy the adjacency information too for tessellation 
				// At this point all vertices should be copied so it should work to just copy/convert the indices.
				if (LOD.bHasAdjacencyInfo && LOD.AdjacencyIndexBuffer.GetNumIndices() > 0)
				{
					FIndexArrayView AdjacencyIndices = LOD.AdjacencyIndexBuffer.GetArrayView();

					// We multiply these by 4 as the adjacency data is 12 indices per triangle instead of the normal 3
					uint32 StartIndex = Section.FirstIndex * 4;
					uint32 NumIndices = Section.NumTriangles * 3 * 4;

					for (uint32 Index = 0; Index < NumIndices; Index++)
					{
						AdjacencyIndexCreator(MeshToSectionVertMap[AdjacencyIndices[Index]]);
					}
				}
			}
		}
	}
}


#undef LOCTEXT_NAMESPACE