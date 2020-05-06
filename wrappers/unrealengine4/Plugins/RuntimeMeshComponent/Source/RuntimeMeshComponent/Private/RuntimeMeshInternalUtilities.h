// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FRuntimeMeshVertexSortingElement
{
	float Value;
	int32 Index;

	FRuntimeMeshVertexSortingElement() {}

	FRuntimeMeshVertexSortingElement(int32 InIndex, FVector Vector)
	{
		Value = 0.30f * Vector.X + 0.33f * Vector.Y + 0.37f * Vector.Z;
		Index = InIndex;
	}
};

struct FRuntimeMeshVertexSortingFunction
{
	FORCEINLINE bool operator()(FRuntimeMeshVertexSortingElement const& Left, FRuntimeMeshVertexSortingElement const& Right) const
	{
		return Left.Value < Right.Value;
	}
};




struct FRuntimeMeshInternalUtilities
{
	static TMultiMap<uint32, uint32> FindDuplicateVerticesMap(TFunction<FVector(int32)> VertexAccessor, int32 NumVertices, float Tollerance = 0.0)
	{
		TArray<FRuntimeMeshVertexSortingElement> VertexSorter;
		VertexSorter.Empty(NumVertices);
		for (int32 Index = 0; Index < NumVertices; Index++)
		{
			new (VertexSorter)FRuntimeMeshVertexSortingElement(Index, VertexAccessor(Index));
		}

		// Sort the list
		VertexSorter.Sort(FRuntimeMeshVertexSortingFunction());

		// Clear the index map.
		TMultiMap<uint32, uint32> IndexMap;

		// Map out the duplicates.
		for (int32 Index = 0; Index < NumVertices; Index++)
		{
			uint32 SrcVertIdx = VertexSorter[Index].Index;
			float Value = VertexSorter[Index].Value;

			// Search forward adding pairs both ways
			for (int32 SubIndex = Index + 1; SubIndex < NumVertices; SubIndex++)
			{
				if (FMath::Abs(VertexSorter[SubIndex].Value - Value) > THRESH_POINTS_ARE_SAME * 4.01f)
				{
					// No more possible duplicates
					break;
				}

				uint32 OtherVertIdx = VertexSorter[SubIndex].Index;
				if (VertexAccessor(SrcVertIdx).Equals(VertexAccessor(OtherVertIdx), Tollerance))
				{
					IndexMap.AddUnique(SrcVertIdx, OtherVertIdx);
					IndexMap.AddUnique(OtherVertIdx, SrcVertIdx);
				}
			}
		}

		return IndexMap;
	}

	static TMultiMap<uint32, uint32> FindDuplicateVerticesMap(const TArray<FVector>& Vertices, float Tollerance = 0.0)
	{
		int32 NumVertices = Vertices.Num();

		TArray<FRuntimeMeshVertexSortingElement> VertexSorter;
		VertexSorter.Empty(NumVertices);
		for (int32 Index = 0; Index < NumVertices; Index++)
		{
			new (VertexSorter)FRuntimeMeshVertexSortingElement(Index, Vertices[Index]);
		}

		// Sort the list
		VertexSorter.Sort(FRuntimeMeshVertexSortingFunction());

		// Clear the index map.
		TMultiMap<uint32, uint32> IndexMap;

		// Map out the duplicates.
		for (int32 Index = 0; Index < NumVertices; Index++)
		{
			uint32 SrcVertIdx = VertexSorter[Index].Index;
			float Value = VertexSorter[Index].Value;

			// Search forward adding pairs both ways
			for (int32 SubIndex = Index + 1; SubIndex < NumVertices; SubIndex++)
			{
				if (FMath::Abs(VertexSorter[SubIndex].Value - Value) > THRESH_POINTS_ARE_SAME * 4.01f)
				{
					// No more possible duplicates
					break;
				}

				uint32 OtherVertIdx = VertexSorter[SubIndex].Index;
				if (Vertices[SrcVertIdx].Equals(Vertices[OtherVertIdx], Tollerance))
				{
					IndexMap.AddUnique(SrcVertIdx, OtherVertIdx);
					IndexMap.AddUnique(OtherVertIdx, SrcVertIdx);
				}
			}
		}

		return IndexMap;
	}

	static TArray<uint32> FindDuplicateVertices(const TArray<FVector>& Vertices, float Tollerance = 0.0)
	{
		int32 NumVertices = Vertices.Num();

		TArray<FRuntimeMeshVertexSortingElement> VertexSorter;
		VertexSorter.Empty(NumVertices);
		for (int32 Index = 0; Index < NumVertices; Index++)
		{
			new (VertexSorter)FRuntimeMeshVertexSortingElement(Index, Vertices[Index]);
		}

		// Sort the list
		VertexSorter.Sort(FRuntimeMeshVertexSortingFunction());

		// Clear the index map.
		TArray<uint32> IndexMap;
		IndexMap.AddUninitialized(NumVertices);
		FMemory::Memset(IndexMap.GetData(), 0xFF, NumVertices * sizeof(uint32));

		// Map out the duplicates.
		for (int32 Index = 0; Index < NumVertices; Index++)
		{
			uint32 SrcVertIdx = VertexSorter[Index].Index;
			float Value = VertexSorter[Index].Value;
			IndexMap[SrcVertIdx] = FMath::Min(IndexMap[SrcVertIdx], SrcVertIdx);

			// Search forward adding pairs both ways
			for (int32 SubIndex = Index + 1; SubIndex < NumVertices; SubIndex++)
			{
				if (FMath::Abs(VertexSorter[SubIndex].Value - Value) > THRESH_POINTS_ARE_SAME * 4.01f)
				{
					// No more possible duplicates
					break;
				}

				uint32 OtherVertIdx = VertexSorter[SubIndex].Index;
				if (Vertices[SrcVertIdx].Equals(Vertices[OtherVertIdx], Tollerance))
				{
					IndexMap[SrcVertIdx] = FMath::Min(IndexMap[SrcVertIdx], OtherVertIdx);
					IndexMap[OtherVertIdx] = FMath::Min(IndexMap[OtherVertIdx], SrcVertIdx);
				}
			}
		}

		return IndexMap;
	}
};