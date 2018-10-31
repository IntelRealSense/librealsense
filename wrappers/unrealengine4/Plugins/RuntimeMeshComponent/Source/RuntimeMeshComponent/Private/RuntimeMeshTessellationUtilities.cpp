// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshTessellationUtilities.h"
#include "RuntimeMeshComponentPlugin.h"

const uint32 EdgesPerTriangle = 3;
const uint32 IndicesPerTriangle = 3;
const uint32 VerticesPerTriangle = 3;
const uint32 DuplicateIndexCount = 3;

const uint32 PnAenDomCorner_IndicesPerPatch = 12;

DECLARE_CYCLE_STAT(TEXT("RML - Calculate Tessellation Indices"), STAT_RuntimeMeshLibrary_CalculateTessellationIndices, STATGROUP_RuntimeMesh);

void FTessellationUtilities::AddIfLeastUV(PositionDictionary& PosDict, const Vertex& Vert, uint32 Index)
{
	auto* Pos = PosDict.Find(Vert.Position);
	if (Pos == nullptr)
	{
		PosDict.Add(Vert.Position, Corner(Index, Vert.TexCoord));
	}
	else if (Vert.TexCoord < Pos->TexCoord)
	{
		PosDict[Vert.Position] = Corner(Index, Vert.TexCoord);
	}
}


void FTessellationUtilities::CalculateTessellationIndices(int32 NumVertices, int32 NumIndices,
	TFunction<FVector(int32)> PositionAccessor, TFunction<FVector2D(int32)> UVAccessor, TFunction<int32(int32)> IndexAccessor,
	TFunction<void(int32)> OutIndicesSizeSetter, TFunction<int32()> OutIndicesSizeGetter, TFunction<void(int32, int32)> OutIndicesWriter, TFunction<int32(int32)> OutIndicesReader)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshLibrary_CalculateTessellationIndices);

	EdgeDictionary EdgeDict;
	EdgeDict.Reserve(NumIndices);
	PositionDictionary PosDict;
	PosDict.Reserve(NumIndices);

	OutIndicesSizeSetter(PnAenDomCorner_IndicesPerPatch * (NumIndices / IndicesPerTriangle));

	ExpandIB(NumVertices, NumIndices, PositionAccessor, UVAccessor, IndexAccessor, OutIndicesSizeSetter, OutIndicesSizeGetter, OutIndicesWriter, OutIndicesReader, EdgeDict, PosDict);

	ReplacePlaceholderIndices(NumVertices, NumIndices, PositionAccessor, UVAccessor, IndexAccessor, OutIndicesSizeSetter, OutIndicesSizeGetter, OutIndicesWriter, OutIndicesReader, EdgeDict, PosDict);
}


void FTessellationUtilities::ExpandIB(int32 NumVertices, int32 NumIndices,
	TFunction<FVector(int32)> PositionAccessor, TFunction<FVector2D(int32)> UVAccessor, TFunction<int32(int32)> IndexAccessor,
	TFunction<void(int32)> OutIndicesSizeSetter, TFunction<int32()> OutIndicesSizeGetter, TFunction<void(int32, int32)> OutIndicesWriter, TFunction<int32(int32)> OutIndicesReader,
	EdgeDictionary& OutEdgeDict, PositionDictionary& OutPosDict)
{
	const uint32 TriangleCount = NumIndices / IndicesPerTriangle;

	for (uint32 U = 0; U < TriangleCount; U++)
	{
		const uint32 StartInIndex = U * IndicesPerTriangle;
		const uint32 StartOutIndex = U * PnAenDomCorner_IndicesPerPatch;

		const uint32 Index0 = IndexAccessor(U * 3 + 0);
		const uint32 Index1 = IndexAccessor(U * 3 + 1);
		const uint32 Index2 = IndexAccessor(U * 3 + 2);

		const Vertex Vertex0(PositionAccessor(Index0), UVAccessor(Index0));
		const Vertex Vertex1(PositionAccessor(Index1), UVAccessor(Index1));
		const Vertex Vertex2(PositionAccessor(Index2), UVAccessor(Index2));

		Triangle Tri(Index0, Index1, Index2, Vertex0, Vertex1, Vertex2);

		if ((uint32)OutIndicesSizeGetter() <= (StartOutIndex + PnAenDomCorner_IndicesPerPatch))
		{
			OutIndicesSizeSetter((StartOutIndex + PnAenDomCorner_IndicesPerPatch) + 1);
		}

		OutIndicesWriter(StartOutIndex + 0, Tri.GetIndex(0));
		OutIndicesWriter(StartOutIndex + 1, Tri.GetIndex(1));
		OutIndicesWriter(StartOutIndex + 2, Tri.GetIndex(2));

		OutIndicesWriter(StartOutIndex + 3, Tri.GetIndex(0));
		OutIndicesWriter(StartOutIndex + 4, Tri.GetIndex(1));

		OutIndicesWriter(StartOutIndex + 5, Tri.GetIndex(1));
		OutIndicesWriter(StartOutIndex + 6, Tri.GetIndex(2));

		OutIndicesWriter(StartOutIndex + 7, Tri.GetIndex(2));
		OutIndicesWriter(StartOutIndex + 8, Tri.GetIndex(0));

		OutIndicesWriter(StartOutIndex + 9, Tri.GetIndex(0));
		OutIndicesWriter(StartOutIndex + 10, Tri.GetIndex(1));
		OutIndicesWriter(StartOutIndex + 11, Tri.GetIndex(2));


		Edge Rev0 = Tri.GetEdge(0).GetReverse();
		Edge Rev1 = Tri.GetEdge(1).GetReverse();
		Edge Rev2 = Tri.GetEdge(2).GetReverse();

		OutEdgeDict.Add(Rev0, Rev0);
		OutEdgeDict.Add(Rev1, Rev1);
		OutEdgeDict.Add(Rev2, Rev2);

		AddIfLeastUV(OutPosDict, Vertex0, Index0);
		AddIfLeastUV(OutPosDict, Vertex1, Index1);
		AddIfLeastUV(OutPosDict, Vertex2, Index2);
	}
}



void FTessellationUtilities::ReplacePlaceholderIndices(int32 NumVertices, int32 NumIndices,
	TFunction<FVector(int32)> PositionAccessor, TFunction<FVector2D(int32)> UVAccessor, TFunction<int32(int32)> IndexAccessor,
	TFunction<void(int32)> OutIndicesSizeSetter, TFunction<int32()> OutIndicesSizeGetter, TFunction<void(int32, int32)> OutIndicesWriter, TFunction<int32(int32)> OutIndicesReader,
	EdgeDictionary& EdgeDict, PositionDictionary& PosDict)
{
	const uint32 TriangleCount = NumIndices / PnAenDomCorner_IndicesPerPatch;

	for (uint32 U = 0; U < TriangleCount; U++)
	{
		const uint32 StartOutIndex = U * PnAenDomCorner_IndicesPerPatch;

		const uint32 Index0 = OutIndicesReader(StartOutIndex + 0);
		const uint32 Index1 = OutIndicesReader(StartOutIndex + 1);
		const uint32 Index2 = OutIndicesReader(StartOutIndex + 2);

		const Vertex Vertex0(PositionAccessor(Index0), UVAccessor(Index0));
		const Vertex Vertex1(PositionAccessor(Index1), UVAccessor(Index1));
		const Vertex Vertex2(PositionAccessor(Index2), UVAccessor(Index2));

		Triangle Tri(Index0, Index1, Index2, Vertex0, Vertex1, Vertex2);

		Edge* Ed = EdgeDict.Find(Tri.GetEdge(0));
		if (Ed != nullptr)
		{
			OutIndicesWriter(StartOutIndex + 3, Ed->GetIndex(0));
			OutIndicesWriter(StartOutIndex + 4, Ed->GetIndex(1));
		}

		Ed = EdgeDict.Find(Tri.GetEdge(1));
		if (Ed != nullptr)
		{
			OutIndicesWriter(StartOutIndex + 5, Ed->GetIndex(0));
			OutIndicesWriter(StartOutIndex + 6, Ed->GetIndex(1));
		}

		Ed = EdgeDict.Find(Tri.GetEdge(2));
		if (Ed != nullptr)
		{
			OutIndicesWriter(StartOutIndex + 7, Ed->GetIndex(0));
			OutIndicesWriter(StartOutIndex + 8, Ed->GetIndex(1));
		}

		// Deal with dominant positions.
		for (uint32 V = 0; V < VerticesPerTriangle; V++)
		{
			Corner* Corn = PosDict.Find(Tri.GetEdge(V).GetVertex(0).Position);
			if (Corn != nullptr)
			{
				OutIndicesWriter(StartOutIndex + 9 + V, Corn->Index);
			}
		}
	}
}
