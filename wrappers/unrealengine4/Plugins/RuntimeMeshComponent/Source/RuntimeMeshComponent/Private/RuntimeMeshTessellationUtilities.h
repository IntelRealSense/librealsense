// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshBuilder.h"


/**
*
*/
class FTessellationUtilities
{
public:
	static void CalculateTessellationIndices(int32 NumVertices, int32 NumIndices,
		TFunction<FVector(int32)> PositionAccessor, TFunction<FVector2D(int32)> UVAccessor, TFunction<int32(int32)> IndexAccessor,
		TFunction<void(int32)> OutIndicesSizeSetter, TFunction<int32()> OutIndicesSizeGetter, TFunction<void(int32, int32)> OutIndicesWriter, TFunction<int32(int32)> OutIndicesReader);




private:
	struct Vertex
	{
		FVector Position;
		FVector2D TexCoord;

		Vertex() { }
		Vertex(const FVector& InPosition, const FVector2D& InTexCoord)
			: Position(InPosition), TexCoord(InTexCoord)
		{ }

		FORCEINLINE bool operator==(const Vertex& Other) const
		{
			return Position == Other.Position;
		}
		FORCEINLINE bool operator<(const Vertex& Other) const
		{
			return Position.X < Other.Position.X
				|| Position.Y < Other.Position.Y
				|| Position.Z < Other.Position.Z;
		}
	};

	static FORCEINLINE uint32 HashValue(const FVector& Vec)
	{
		return 31337 * GetTypeHash(Vec.X) + 13 * GetTypeHash(Vec.Y) + 3 * GetTypeHash(Vec.Z);
	}

	static FORCEINLINE uint32 HashValue(const Vertex& Vert)
	{
		return HashValue(Vert.Position);
	}

	struct Edge
	{
	private:
		uint32 IndexFrom;
		uint32 IndexTo;

		Vertex VertexFrom;
		Vertex VertexTo;

		uint32 CachedHash;

	public:
		Edge() : CachedHash(0) { }
		Edge(uint32 InIndexFrom, uint32 InIndexTo, const Vertex& InVertexFrom, const Vertex& InVertexTo)
			: IndexFrom(InIndexFrom), IndexTo(InIndexTo), VertexFrom(InVertexFrom), VertexTo(InVertexTo)
		{
			// Hash should only consider position, not index. 
			// We want values with different indices to compare true.
			CachedHash = 7 * HashValue(VertexFrom) + 2 * HashValue(VertexTo);
		}

		Vertex GetVertex(uint32 I) const
		{
			switch (I)
			{
			case 0:
				return VertexFrom;
			case 1:
				return VertexTo;
			default:
				checkNoEntry();
				return Vertex();
			}
		}

		uint32 GetIndex(uint32 I) const
		{
			switch (I)
			{
			case 0:
				return IndexFrom;
			case 1:
				return IndexTo;
			default:
				checkNoEntry();
				return 0;
			}
		}

		Edge GetReverse() const
		{
			return Edge(IndexTo, IndexFrom, VertexTo, VertexFrom);
		}

		FORCEINLINE bool operator<(const Edge& Other) const
		{
			// Quick out, otherwise we have to compare vertices
			if (IndexFrom == Other.IndexFrom && IndexTo == Other.IndexTo)
			{
				return false;
			}

			return VertexFrom < Other.VertexFrom || VertexTo < Other.VertexTo;
		}

		FORCEINLINE bool operator==(const Edge& Other) const
		{
			return (IndexFrom == Other.IndexFrom && IndexTo == Other.IndexTo) ||
				(VertexFrom == Other.VertexFrom && VertexTo == Other.VertexTo);
		}

		friend FORCEINLINE uint32 GetTypeHash(const Edge& E)
		{
			return E.CachedHash;
		}
	};

	struct Corner
	{
		uint32 Index;
		FVector2D TexCoord;

		Corner() : Index(0) { }
		Corner(uint32 InIndex, FVector2D InTexCoord)
			: Index(InIndex), TexCoord(InTexCoord)
		{ }
	};

	class Triangle
	{
		Edge Edge0;
		Edge Edge1;
		Edge Edge2;

	public:
		Triangle(uint32 Index0, uint32 Index1, uint32 Index2, const Vertex& Vertex0, const Vertex& Vertex1, const Vertex& Vertex2)
			: Edge0(Index0, Index1, Vertex0, Vertex1)
			, Edge1(Index1, Index2, Vertex1, Vertex2)
			, Edge2(Index2, Index0, Vertex2, Vertex0)
		{ }

		FORCEINLINE bool operator<(const Triangle& Other) const
		{
			return Edge0 < Other.Edge0 || Edge1 < Other.Edge1 || Edge2 < Other.Edge2;
		}

		FORCEINLINE const Edge& GetEdge(uint32 I)
		{
			return ((Edge*)&Edge0)[I];
		}
		FORCEINLINE uint32 GetIndex(uint32 I)
		{
			return GetEdge(I).GetIndex(0);
		}

	};
	using EdgeDictionary = TMap<Edge, Edge>;
	using PositionDictionary = TMap<FVector, Corner>;

	static void AddIfLeastUV(PositionDictionary& PosDict, const Vertex& Vert, uint32 Index);

	static void ReplacePlaceholderIndices(int32 NumVertices, int32 NumIndices,
		TFunction<FVector(int32)> PositionAccessor, TFunction<FVector2D(int32)> UVAccessor, TFunction<int32(int32)> IndexAccessor,
		TFunction<void(int32)> OutIndicesSizeSetter, TFunction<int32()> OutIndicesSizeGetter, TFunction<void(int32, int32)> OutIndicesWriter, TFunction<int32(int32)> OutIndicesReader,
		EdgeDictionary& EdgeDict, PositionDictionary& PosDict);

	static void ExpandIB(int32 NumVertices, int32 NumIndices,
		TFunction<FVector(int32)> PositionAccessor, TFunction<FVector2D(int32)> UVAccessor, TFunction<int32(int32)> IndexAccessor,
		TFunction<void(int32)> OutIndicesSizeSetter, TFunction<int32()> OutIndicesSizeGetter, TFunction<void(int32, int32)> OutIndicesWriter, TFunction<int32(int32)> OutIndicesReader,
		EdgeDictionary& OutEdgeDict, PositionDictionary& OutPosDict);
};