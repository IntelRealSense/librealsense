// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshBuilder.h"
#include "RuntimeMeshComponentPlugin.h"
#include "RuntimeMeshData.h"


template<typename TYPE>
FORCEINLINE static TYPE& GetStreamAccess(TArray<uint8>* Data, int32 Index, int32 Stride, int32 Offset)
{
	int32 StartPosition = (Index * Stride + Offset);
	return *((TYPE*)&(*Data)[StartPosition]);
}

FORCEINLINE static uint8* GetStreamAccessPointer(TArray<uint8>* Data, int32 Index, int32 Stride, int32 Offset)
{
    int32 StartPosition = (Index * Stride + Offset);
    return &(*Data)[StartPosition];
}

template<typename TYPE>
struct FRuntimeMeshEightUV
{
	TStaticArray<TYPE, 8> UVs;
};

static_assert(sizeof(FRuntimeMeshEightUV<FVector2D>) == (8 * sizeof(FVector2D)), "Incorrect size for 8 UV struct");
static_assert(sizeof(FRuntimeMeshEightUV<FVector2DHalf>) == (8 * sizeof(FVector2DHalf)), "Incorrect size for 8 UV struct");


//////////////////////////////////////////////////////////////////////////
//	FRuntimeMeshVerticesAccessor

FRuntimeMeshVerticesAccessor::FRuntimeMeshVerticesAccessor(TArray<uint8>* PositionStreamData, TArray<uint8>* TangentStreamData, TArray<uint8>* UVStreamData, TArray<uint8>* ColorStreamData, bool bInIsReadonly)
	: bIsInitialized(false)
	, bIsReadonly(bInIsReadonly)
	, PositionStream(PositionStreamData)
	, TangentStream(TangentStreamData), bTangentHighPrecision(false), TangentSize(0), TangentStride(0)
	, UVStream(UVStreamData), bUVHighPrecision(false), UVChannelCount(0), UVSize(0), UVStride(0)
	, ColorStream(ColorStreamData)
{
}

FRuntimeMeshVerticesAccessor::FRuntimeMeshVerticesAccessor(bool bInTangentsHighPrecision, bool bInUVsHighPrecision, int32 bInUVCount,
	TArray<uint8>* PositionStreamData, TArray<uint8>* TangentStreamData, TArray<uint8>* UVStreamData, TArray<uint8>* ColorStreamData, bool bInIsReadonly)
	: bIsInitialized(false)
	, bIsReadonly(bInIsReadonly)
	, PositionStream(PositionStreamData)
	, TangentStream(TangentStreamData), bTangentHighPrecision(false), TangentSize(0), TangentStride(0)
	, UVStream(UVStreamData), bUVHighPrecision(false), UVChannelCount(0), UVSize(0), UVStride(0)
	, ColorStream(ColorStreamData)
{
	Initialize(bInTangentsHighPrecision, bInUVsHighPrecision, bInUVCount);
}

FRuntimeMeshVerticesAccessor::~FRuntimeMeshVerticesAccessor()
{
}

void FRuntimeMeshVerticesAccessor::Initialize(bool bInTangentsHighPrecision, bool bInUVsHighPrecision, int32 bInUVCount)
{
	bIsInitialized = true;
	const_cast<bool&>(bTangentHighPrecision) = bInTangentsHighPrecision;
	const_cast<int32&>(TangentSize) = (bTangentHighPrecision ? sizeof(FPackedRGBA16N) : sizeof(FPackedNormal));
	const_cast<int32&>(TangentStride) = TangentSize * 2;

	const_cast<bool&>(bUVHighPrecision) = bInUVsHighPrecision;
	const_cast<int32&>(UVChannelCount) = bInUVCount;
	const_cast<int32&>(UVSize) = (bUVHighPrecision ? sizeof(FVector2D) : sizeof(FVector2DHalf));
	const_cast<int32&>(UVStride) = UVSize * UVChannelCount;
}

int32 FRuntimeMeshVerticesAccessor::NumVertices() const
{
	check(bIsInitialized);
	int32 Count = PositionStream->Num() / PositionStride;
	return Count;
}

int32 FRuntimeMeshVerticesAccessor::NumUVChannels() const
{
	check(bIsInitialized);
	return UVChannelCount;
}

void FRuntimeMeshVerticesAccessor::EmptyVertices(int32 Slack /*= 0*/)
{
	check(bIsInitialized);
	check(!bIsReadonly);
	PositionStream->Empty(Slack * PositionStride);
	TangentStream->Empty(Slack * TangentStride);
	UVStream->Empty(Slack * UVSize);
	ColorStream->Empty(Slack * ColorStride);
}

void FRuntimeMeshVerticesAccessor::SetNumVertices(int32 NewNum)
{
	check(bIsInitialized);
	check(!bIsReadonly);
	PositionStream->SetNumZeroed(NewNum * PositionStride);
	TangentStream->SetNumZeroed(NewNum * TangentStride);
	UVStream->SetNumZeroed(NewNum * UVStride);
	ColorStream->SetNumZeroed(NewNum * ColorStride);
}

int32 FRuntimeMeshVerticesAccessor::AddVertex(FVector InPosition)
{
	check(bIsInitialized);
	check(!bIsReadonly);
	int32 NewIndex = AddSingleVertex();

	SetPosition(NewIndex, InPosition);

	return NewIndex;
}

FVector FRuntimeMeshVerticesAccessor::GetPosition(int32 Index) const
{
	check(bIsInitialized);
	return GetStreamAccess<FVector>(PositionStream, Index, PositionStride, 0);
}

FVector4 FRuntimeMeshVerticesAccessor::GetNormal(int32 Index) const
{
	check(bIsInitialized);
	if (bTangentHighPrecision)
	{
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 20
		return GetStreamAccess<FRuntimeMeshTangentsHighPrecision>(TangentStream, Index, TangentStride, 0).Normal.ToFVector4();
#else
		return GetStreamAccess<FRuntimeMeshTangentsHighPrecision>(TangentStream, Index, TangentStride, 0).Normal;
#endif
	}
	else
	{
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 20
		return GetStreamAccess<FRuntimeMeshTangents>(TangentStream, Index, TangentStride, 0).Normal.ToFVector4();
#else
		return GetStreamAccess<FRuntimeMeshTangents>(TangentStream, Index, TangentStride, 0).Normal;
#endif
	}
}

FVector FRuntimeMeshVerticesAccessor::GetTangent(int32 Index) const
{
	check(bIsInitialized);
	if (bTangentHighPrecision)
	{
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 20
		return GetStreamAccess<FRuntimeMeshTangentsHighPrecision>(TangentStream, Index, TangentStride, 0).Tangent.ToFVector4();
#else
		return GetStreamAccess<FRuntimeMeshTangentsHighPrecision>(TangentStream, Index, TangentStride, 0).Tangent;
#endif
	}
	else
	{
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 20
		return GetStreamAccess<FRuntimeMeshTangents>(TangentStream, Index, TangentStride, 0).Tangent.ToFVector4();
#else
		return GetStreamAccess<FRuntimeMeshTangents>(TangentStream, Index, TangentStride, 0).Tangent;
#endif
	}
}

FColor FRuntimeMeshVerticesAccessor::GetColor(int32 Index) const
{
	check(bIsInitialized);
	return GetStreamAccess<FColor>(ColorStream, Index, ColorStride, 0);
}

FVector2D FRuntimeMeshVerticesAccessor::GetUV(int32 Index, int32 Channel) const
{
	check(bIsInitialized);
	check(Channel >= 0 && Channel < UVChannelCount);
	if (bUVHighPrecision)
	{
		return GetStreamAccess<FRuntimeMeshEightUV<FVector2D>>(UVStream, Index, UVStride, 0).UVs[Channel];
	}
	else
	{
		return GetStreamAccess<FRuntimeMeshEightUV<FVector2DHalf>>(UVStream, Index, UVStride, 0).UVs[Channel];
	}
}

void FRuntimeMeshVerticesAccessor::SetPosition(int32 Index, const FVector& Value)
{
	check(bIsInitialized);
	GetStreamAccess<FVector>(PositionStream, Index, PositionStride, 0) = Value;
}

bool FRuntimeMeshVerticesAccessor::SetPositions(const int32 InsertAtIndex, const TArray<FVector>& Positions, const int32 Count, const bool bSizeToFit)
{
	int32 CountClamped = FMath::Clamp(Count, 0, Positions.Num());
	return SetPositions(InsertAtIndex, Positions.GetData(), CountClamped, bSizeToFit);
}

bool FRuntimeMeshVerticesAccessor::SetPositions(const int32 InsertAtIndex, const FVector *const Positions, const int32 Count, const bool bSizeToFit)
{
	check(bIsInitialized);
	check(!bIsReadonly);
	if (InsertAtIndex + Count > NumVertices())
	{
		if (bSizeToFit)
		{
			SetNumVertices(InsertAtIndex + Count);
		}
		else
		{
			return false;
		}
	}

	FMemory::Memcpy(GetStreamAccessPointer(PositionStream, InsertAtIndex, PositionStride, 0), Positions, Count * sizeof(*Positions));
	return true;
}

void FRuntimeMeshVerticesAccessor::SetNormal(int32 Index, const FVector4& Value)
{
	check(bIsInitialized);
	check(!bIsReadonly);
	if (bTangentHighPrecision)
	{
		GetStreamAccess<FRuntimeMeshTangentsHighPrecision>(TangentStream, Index, TangentStride, 0).Normal = Value;
	}
	else
	{
		GetStreamAccess<FRuntimeMeshTangents>(TangentStream, Index, TangentStride, 0).Normal = Value;
	}
}

void FRuntimeMeshVerticesAccessor::SetTangent(int32 Index, const FVector& Value)
{
	check(bIsInitialized);
	check(!bIsReadonly);
	if (bTangentHighPrecision)
	{
		GetStreamAccess<FRuntimeMeshTangentsHighPrecision>(TangentStream, Index, TangentStride, 0).Tangent = Value;
	}
	else
	{
		GetStreamAccess<FRuntimeMeshTangents>(TangentStream, Index, TangentStride, 0).Tangent = Value;
	}
}

void FRuntimeMeshVerticesAccessor::SetTangent(int32 Index, const FRuntimeMeshTangent& Value)
{
	check(bIsInitialized);
	check(!bIsReadonly);
	if (bTangentHighPrecision)
	{
		FRuntimeMeshTangentsHighPrecision& Tangents = GetStreamAccess<FRuntimeMeshTangentsHighPrecision>(TangentStream, Index, TangentStride, 0);
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 20
		FVector4 NewNormal = Tangents.Normal.ToFVector4();
#else
		FVector4 NewNormal = Tangents.Normal;
#endif
		NewNormal.W = Value.bFlipTangentY ? -1.0f : 1.0f;
		Tangents.Normal = NewNormal;
		Tangents.Tangent = Value.TangentX;
	}
	else
	{
		FRuntimeMeshTangents& Tangents = GetStreamAccess<FRuntimeMeshTangents>(TangentStream, Index, TangentStride, 0);
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 20
		FVector4 NewNormal = Tangents.Normal.ToFVector4();
#else
		FVector4 NewNormal = Tangents.Normal;
#endif
		NewNormal.W = Value.bFlipTangentY ? -1.0f : 1.0f;
		Tangents.Normal = NewNormal;
		Tangents.Tangent = Value.TangentX;
	}
}

void FRuntimeMeshVerticesAccessor::SetColor(int32 Index, const FColor& Value)
{
	check(bIsInitialized);
	check(!bIsReadonly);
	GetStreamAccess<FColor>(ColorStream, Index, ColorStride, 0) = Value;
}

bool FRuntimeMeshVerticesAccessor::SetColors(const int32 InsertAtIndex, const TArray<FColor>& Colors, const int32 Count, const bool bSizeToFit)
{
	int32 CountClamped = FMath::Clamp(Count, 0, Colors.Num());
	return SetColors(InsertAtIndex, Colors.GetData(), CountClamped, bSizeToFit);
}

bool FRuntimeMeshVerticesAccessor::SetColors(const int32 InsertAtIndex, const FColor *const Colors, const int32 Count, const bool bSizeToFit)
{
	check(bIsInitialized);
	check(!bIsReadonly);
	if (InsertAtIndex + Count > NumVertices())
	{
		if (bSizeToFit)
		{
			SetNumVertices(InsertAtIndex + Count);
		}
		else
		{
			return false;
		}
	}

	FMemory::Memcpy(GetStreamAccessPointer(ColorStream, InsertAtIndex, ColorStride, 0), Colors, Count * sizeof(*Colors));
	return true;
}

void FRuntimeMeshVerticesAccessor::SetUV(int32 Index, const FVector2D& Value)
{
	check(bIsInitialized);
	check(!bIsReadonly);
	check(UVChannelCount > 0);
	if (bUVHighPrecision)
	{
		GetStreamAccess<FRuntimeMeshEightUV<FVector2D>>(UVStream, Index, UVStride, 0).UVs[0] = Value;
	}
	else
	{
		GetStreamAccess<FRuntimeMeshEightUV<FVector2DHalf>>(UVStream, Index, UVStride, 0).UVs[0] = Value;
	}
}

void FRuntimeMeshVerticesAccessor::SetUV(int32 Index, int32 Channel, const FVector2D& Value)
{
	check(bIsInitialized);
	check(!bIsReadonly);
	check(Channel >= 0 && Channel < UVChannelCount);
	if (bUVHighPrecision)
	{
		GetStreamAccess<FRuntimeMeshEightUV<FVector2D>>(UVStream, Index, UVStride, 0).UVs[Channel] = Value;
	}
	else
	{
		GetStreamAccess<FRuntimeMeshEightUV<FVector2DHalf>>(UVStream, Index, UVStride, 0).UVs[Channel] = Value;
	}
}

bool FRuntimeMeshVerticesAccessor::SetUVs(const int32 InsertAtVertexIndex, const TArray<FVector2D>& UVs, const int32 CountVertices, const bool bSizeToFit)
{
	check(FMath::IsNearlyZero(FMath::Fractional((float)UVs.Num() / (float)UVChannelCount))); // If this triggers, the your uv data are out of alignment.
	int32 CountClamped = FMath::Clamp(CountVertices, 0, UVs.Num());
	return SetUVs(InsertAtVertexIndex, UVs.GetData(), CountClamped, bSizeToFit);
}


bool FRuntimeMeshVerticesAccessor::SetUVs(const int32 InsertAtVertexIndex, const TArray<FVector2DHalf>& UVs, const int32 CountVertices, const bool bSizeToFit)
{
	check(FMath::IsNearlyZero(FMath::Fractional((float)UVs.Num() / (float)UVChannelCount))); // If this triggers, the your uv data are out of alignment.
	int32 CountClamped = FMath::Clamp(CountVertices, 0, UVs.Num());
	return SetUVs(InsertAtVertexIndex, UVs.GetData(), CountClamped, bSizeToFit);

}

bool FRuntimeMeshVerticesAccessor::SetUVs(const int32 InsertAtVertexIndex, const FVector2D *const UVs, const int32 CountVertices, const bool bSizeToFit)
{
	check(bIsInitialized);
	check(!bIsReadonly);
	check(bUVHighPrecision); // Half precision is not supported by this function
	if (InsertAtVertexIndex + CountVertices > NumVertices())
	{
		if (bSizeToFit)
		{
			SetNumVertices(InsertAtVertexIndex + CountVertices);
		}
		else
		{
			return false;
		}
	}

	FMemory::Memcpy(GetStreamAccessPointer(UVStream, InsertAtVertexIndex, UVStride, 0), UVs, CountVertices * sizeof(*UVs) * UVChannelCount);
	return true;
}

bool FRuntimeMeshVerticesAccessor::SetUVs(const int32 InsertAtVertexIndex, const FVector2DHalf *const UVs, const int32 CountVertices, const bool bSizeToFit)
{
	check(bIsInitialized);
	check(!bIsReadonly);
	check(!bUVHighPrecision); // Only Half precision is supported by this function
	if (InsertAtVertexIndex + CountVertices > NumVertices())
	{
		if (bSizeToFit)
		{
			SetNumVertices(InsertAtVertexIndex + CountVertices);
		}
		else
		{
			return false;
		}
	}

	FMemory::Memcpy(GetStreamAccessPointer(UVStream, InsertAtVertexIndex, UVStride, 0), UVs, CountVertices * sizeof(*UVs) * UVChannelCount);
	return true;
}

void FRuntimeMeshVerticesAccessor::SetNormalTangent(int32 Index, FVector Normal, FRuntimeMeshTangent Tangent)
{
	check(bIsInitialized);
	check(!bIsReadonly);
	if (bTangentHighPrecision)
	{
		FRuntimeMeshTangentsHighPrecision& Tangents = GetStreamAccess<FRuntimeMeshTangentsHighPrecision>(TangentStream, Index, TangentStride, 0);
		Tangents.Normal = FVector4(Normal, Tangent.bFlipTangentY ? -1 : 1);
		Tangents.Tangent = Tangent.TangentX;
	}
	else
	{
		FRuntimeMeshTangents& Tangents = GetStreamAccess<FRuntimeMeshTangents>(TangentStream, Index, TangentStride, 0);
		Tangents.Normal = FVector4(Normal, Tangent.bFlipTangentY ? -1 : 1);
		Tangents.Tangent = Tangent.TangentX;
	}
}

void FRuntimeMeshVerticesAccessor::SetTangents(int32 Index, FVector TangentX, FVector TangentY, FVector TangentZ)
{
	check(bIsInitialized);
	check(!bIsReadonly);
	if (bTangentHighPrecision)
	{
		FRuntimeMeshTangentsHighPrecision& Tangents = GetStreamAccess<FRuntimeMeshTangentsHighPrecision>(TangentStream, Index, TangentStride, 0);
		Tangents.Normal = FVector4(TangentZ, GetBasisDeterminantSign(TangentX, TangentY, TangentZ));
		Tangents.Tangent = TangentX;
	}
	else
	{
		FRuntimeMeshTangents& Tangents = GetStreamAccess<FRuntimeMeshTangents>(TangentStream, Index, TangentStride, 0);
		Tangents.Normal = FVector4(TangentZ, GetBasisDeterminantSign(TangentX, TangentY, TangentZ));
		Tangents.Tangent = TangentX;
	}
}





FRuntimeMeshAccessorVertex FRuntimeMeshVerticesAccessor::GetVertex(int32 Index) const
{
	check(bIsInitialized);
	FRuntimeMeshAccessorVertex Vertex;

	Vertex.Position = GetStreamAccess<FVector>(PositionStream, Index, PositionStride, 0);

	if (bTangentHighPrecision)
	{
		FRuntimeMeshTangentsHighPrecision& Tangents = GetStreamAccess<FRuntimeMeshTangentsHighPrecision>(TangentStream, Index, TangentStride, 0); 
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 20
		Vertex.Normal = Tangents.Normal.ToFVector4();
		Vertex.Tangent = Tangents.Tangent.ToFVector();
#else
		Vertex.Normal = Tangents.Normal;
		Vertex.Tangent = Tangents.Tangent;
#endif
	}
	else
	{
		FRuntimeMeshTangents& Tangents = GetStreamAccess<FRuntimeMeshTangents>(TangentStream, Index, TangentStride, 0);
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 20
		Vertex.Normal = Tangents.Normal.ToFVector4();
		Vertex.Tangent = Tangents.Tangent.ToFVector();
#else
		Vertex.Normal = Tangents.Normal;
		Vertex.Tangent = Tangents.Tangent;
#endif
	}

	Vertex.Color = GetStreamAccess<FColor>(ColorStream, Index, ColorStride, 0);
			
	Vertex.UVs.SetNum(NumUVChannels());
	if (bUVHighPrecision)
	{
		FRuntimeMeshEightUV<FVector2D>& UVs = GetStreamAccess<FRuntimeMeshEightUV<FVector2D>>(UVStream, Index, UVStride, 0);
		for (int32 UVIndex = 0; UVIndex < Vertex.UVs.Num(); UVIndex++)
		{
			Vertex.UVs[UVIndex] = UVs.UVs[UVIndex];
		}
	}
	else
	{
		FRuntimeMeshEightUV<FVector2DHalf>& UVs = GetStreamAccess<FRuntimeMeshEightUV<FVector2DHalf>>(UVStream, Index, UVStride, 0);
		for (int32 UVIndex = 0; UVIndex < Vertex.UVs.Num(); UVIndex++)
		{
			Vertex.UVs[UVIndex] = UVs.UVs[UVIndex];
		}
	}

	return Vertex;
}

void FRuntimeMeshVerticesAccessor::SetVertex(int32 Index, const FRuntimeMeshAccessorVertex& Vertex)
{
	check(bIsInitialized);
	check(!bIsReadonly);

	GetStreamAccess<FVector>(PositionStream, Index, PositionStride, 0) = Vertex.Position;

	if (bTangentHighPrecision)
	{
		FRuntimeMeshTangentsHighPrecision& Tangents = GetStreamAccess<FRuntimeMeshTangentsHighPrecision>(TangentStream, Index, TangentStride, 0);
		Tangents.Normal = Vertex.Normal;
		Tangents.Tangent = Vertex.Tangent;
	}
	else
	{
		FRuntimeMeshTangents& Tangents = GetStreamAccess<FRuntimeMeshTangents>(TangentStream, Index, TangentStride, 0);
		Tangents.Normal = Vertex.Normal;
		Tangents.Tangent = Vertex.Tangent;
	}

	GetStreamAccess<FColor>(ColorStream, Index, ColorStride, 0) = Vertex.Color;

	if (bUVHighPrecision)
	{
		FRuntimeMeshEightUV<FVector2D>& UVs = GetStreamAccess<FRuntimeMeshEightUV<FVector2D>>(UVStream, Index, UVStride, 0);
		for (int32 UVIndex = 0; UVIndex < Vertex.UVs.Num(); UVIndex++)
		{
			UVs.UVs[UVIndex] = Vertex.UVs[UVIndex];
		}
	}
	else
	{
		FRuntimeMeshEightUV<FVector2DHalf>& UVs = GetStreamAccess<FRuntimeMeshEightUV<FVector2DHalf>>(UVStream, Index, UVStride, 0);
		for (int32 UVIndex = 0; UVIndex < Vertex.UVs.Num(); UVIndex++)
		{
			UVs.UVs[UVIndex] = Vertex.UVs[UVIndex];
		}
	}
}

int32 FRuntimeMeshVerticesAccessor::AddVertex(const FRuntimeMeshAccessorVertex& Vertex)
{
	check(bIsInitialized);
	check(!bIsReadonly);
	int32 NewIndex = AddSingleVertex();
	SetVertex(NewIndex, Vertex);
	return NewIndex;
}



int32 FRuntimeMeshVerticesAccessor::AddSingleVertex()
{
	int32 NewIndex = NumVertices();

	PositionStream->AddZeroed(PositionStride);
	TangentStream->AddZeroed(TangentStride);
	UVStream->AddZeroed(UVStride);
	ColorStream->AddZeroed(ColorStride);

	return NewIndex;
}





//////////////////////////////////////////////////////////////////////////
//	FRuntimeMeshIndicesAccessor

FRuntimeMeshIndicesAccessor::FRuntimeMeshIndicesAccessor(TArray<uint8>* IndexStreamData, bool bInIsReadonly)
	: bIsInitialized(false)
	, bIsReadonly(bInIsReadonly)
	, IndexStream(IndexStreamData), b32BitIndices(false)
{
}

FRuntimeMeshIndicesAccessor::FRuntimeMeshIndicesAccessor(bool bIn32BitIndices, TArray<uint8>* IndexStreamData, bool bInIsReadonly)
	: bIsInitialized(false)
	, bIsReadonly(bInIsReadonly)
	, IndexStream(IndexStreamData), b32BitIndices(false)
{
	Initialize(bIn32BitIndices);
}

FRuntimeMeshIndicesAccessor::~FRuntimeMeshIndicesAccessor()
{
}

void FRuntimeMeshIndicesAccessor::Initialize(bool bIn32BitIndices)
{
	bIsInitialized = true;
	b32BitIndices = bIn32BitIndices;

	check((IndexStream->Num() % (b32BitIndices ? 4 : 2)) == 0);
}

int32 FRuntimeMeshIndicesAccessor::NumIndices() const
{
	check(bIsInitialized);
	return IndexStream->Num() / GetIndexStride();
}

void FRuntimeMeshIndicesAccessor::EmptyIndices(int32 Slack)
{
	check(bIsInitialized);
	check(!bIsReadonly);
	IndexStream->Empty(Slack * GetIndexStride());
}

void FRuntimeMeshIndicesAccessor::SetNumIndices(int32 NewNum)
{
	check(bIsInitialized);
	check(!bIsReadonly);
	IndexStream->SetNumZeroed(NewNum * GetIndexStride());
}

int32 FRuntimeMeshIndicesAccessor::AddIndex(int32 NewIndex)
{
	check(bIsInitialized);
	check(!bIsReadonly);
	int32 NewPosition = NumIndices();
	IndexStream->AddZeroed(GetIndexStride());
	SetIndex(NewPosition, NewIndex);
	return NewPosition;
}

int32 FRuntimeMeshIndicesAccessor::AddTriangle(int32 Index0, int32 Index1, int32 Index2)
{
	check(bIsInitialized);
	check(!bIsReadonly);
	int32 NewPosition = NumIndices();
	IndexStream->AddZeroed(GetIndexStride() * 3);
	SetIndex(NewPosition + 0, Index0);
	SetIndex(NewPosition + 1, Index1);
	SetIndex(NewPosition + 2, Index2);
	return NewPosition;
}

int32 FRuntimeMeshIndicesAccessor::GetIndex(int32 Index) const
{
	check(bIsInitialized);
	if (b32BitIndices)
	{
		return GetStreamAccess<int32>(IndexStream, Index, sizeof(int32), 0);
	}
	else
	{
		return GetStreamAccess<uint16>(IndexStream, Index, sizeof(uint16), 0);
	}
}

void FRuntimeMeshIndicesAccessor::SetIndex(int32 Index, int32 Value)
{
	check(bIsInitialized);
	check(!bIsReadonly);
	if (b32BitIndices)
	{
		GetStreamAccess<int32>(IndexStream, Index, sizeof(int32), 0) = Value;
	}
	else
	{
		GetStreamAccess<uint16>(IndexStream, Index, sizeof(uint16), 0) = Value;
	}
}

bool FRuntimeMeshIndicesAccessor::SetIndices(const int32 InsertAtIndex, const TArray<uint16>& Indices, const int32 Count, const bool bSizeToFit)
{
	int32 CountClamped = FMath::Clamp(Count, 0, Indices.Num());
	return SetIndices(InsertAtIndex, Indices.GetData(), CountClamped, bSizeToFit);
}

bool FRuntimeMeshIndicesAccessor::SetIndices(const int32 InsertAtIndex, const uint16 *const Indices, const int32 Count, const bool bSizeToFit)
{
	check(!b32BitIndices);
	check(bIsInitialized);
	check(!bIsReadonly);
	if (InsertAtIndex + Count > NumIndices())
	{
		if (bSizeToFit)
		{
			SetNumIndices(InsertAtIndex + Count);
		}
		else
		{
			return false;
		}
	}

	FMemory::Memcpy(GetStreamAccessPointer(IndexStream, InsertAtIndex, sizeof(uint16), 0), Indices, Count * sizeof(*Indices));
	return true;
}

bool FRuntimeMeshIndicesAccessor::SetIndices(const int32 InsertAtIndex, const TArray<int32>& Indices, const int32 Count, const bool bSizeToFit)
{
	int32 CountClamped = FMath::Clamp(Count, 0, Indices.Num());
	return SetIndices(InsertAtIndex, Indices.GetData(), CountClamped, bSizeToFit);
}

bool FRuntimeMeshIndicesAccessor::SetIndices(const int32 InsertAtIndex, const int32 *const Indices, const int32 Count, const bool bSizeToFit)
{
	check(b32BitIndices);
	check(bIsInitialized);
	check(!bIsReadonly);
	if (InsertAtIndex + Count > NumIndices())
	{
		if (bSizeToFit)
		{
			SetNumIndices(InsertAtIndex + Count);
		}
		else
		{
			return false;
		}
	}

	FMemory::Memcpy(GetStreamAccessPointer(IndexStream, InsertAtIndex, sizeof(int32), 0), Indices, Count * sizeof(*Indices));
	return true;
}

//////////////////////////////////////////////////////////////////////////
//	FRuntimeMeshAccessor

FRuntimeMeshAccessor::FRuntimeMeshAccessor(bool bInTangentsHighPrecision, bool bInUVsHighPrecision, int32 bInUVCount, bool bIn32BitIndices, TArray<uint8>* PositionStreamData,
	TArray<uint8>* TangentStreamData, TArray<uint8>* UVStreamData, TArray<uint8>* ColorStreamData, TArray<uint8>* IndexStreamData, bool bInIsReadonly)
	: FRuntimeMeshVerticesAccessor(bInTangentsHighPrecision, bInUVsHighPrecision, bInUVCount, PositionStreamData, TangentStreamData, UVStreamData, ColorStreamData, bInIsReadonly)
	, FRuntimeMeshIndicesAccessor(bIn32BitIndices, IndexStreamData, bInIsReadonly)
{

}

FRuntimeMeshAccessor::~FRuntimeMeshAccessor()
{
}

FRuntimeMeshAccessor::FRuntimeMeshAccessor(TArray<uint8>* PositionStreamData, TArray<uint8>* TangentStreamData, TArray<uint8>* UVStreamData, TArray<uint8>* ColorStreamData, TArray<uint8>* IndexStreamData, bool bInIsReadonly)
	: FRuntimeMeshVerticesAccessor(PositionStreamData, TangentStreamData, UVStreamData, ColorStreamData, bInIsReadonly)
	, FRuntimeMeshIndicesAccessor(IndexStreamData, bInIsReadonly)
{

}

void FRuntimeMeshAccessor::Initialize(bool bInTangentsHighPrecision, bool bInUVsHighPrecision, int32 bInUVCount, bool bIn32BitIndices)
{
	FRuntimeMeshVerticesAccessor::Initialize(bInTangentsHighPrecision, bInUVsHighPrecision, bInUVCount);
	FRuntimeMeshIndicesAccessor::Initialize(bIn32BitIndices);
}




void FRuntimeMeshAccessor::CopyTo(const TSharedPtr<FRuntimeMeshAccessor>& Other, bool bClearDestination) const
{
	if (bClearDestination)
	{
		Other->EmptyVertices(NumVertices());
		Other->EmptyIndices(NumIndices());
	}

	// TODO: Make this faster using short paths when the structures are the same.

	int32 StartVertex = Other->NumVertices();
	int32 NumVerts = NumVertices();
	int32 NumUVs = FMath::Min(NumUVChannels(), Other->NumUVChannels());

	for (int32 Index = 0; Index < NumVerts; Index++)
	{
		int32 NewIndex = Other->AddVertex(GetPosition(Index));
		Other->SetNormal(NewIndex, GetNormal(Index));
		Other->SetTangent(NewIndex, GetTangent(Index));
		Other->SetColor(NewIndex, GetColor(Index));
		for (int32 UVIndex = 0; UVIndex < NumUVs; UVIndex++)
		{
			Other->SetUV(NewIndex, UVIndex, GetUV(Index, UVIndex));
		}
	}

	int32 NumInds = NumIndices();
	for (int32 Index = 0; Index < NumInds; Index++)
	{
		Other->AddIndex(GetIndex(Index) + StartVertex);
	}
}

//////////////////////////////////////////////////////////////////////////
//	FRuntimeMeshBuilder

FRuntimeMeshBuilder::FRuntimeMeshBuilder(bool bInTangentsHighPrecision, bool bInUVsHighPrecision, int32 bInUVCount, bool bIn32BitIndices)
	: FRuntimeMeshAccessor(&PositionStream, &TangentStream, &UVStream, &ColorStream, &IndexStream, false)
{
	FRuntimeMeshAccessor::Initialize(bInTangentsHighPrecision, bInUVsHighPrecision, bInUVCount, bIn32BitIndices);
}

FRuntimeMeshBuilder::~FRuntimeMeshBuilder()
{

}


//////////////////////////////////////////////////////////////////////////
//	FRuntimeMeshScopedUpdater

FRuntimeMeshScopedUpdater::FRuntimeMeshScopedUpdater(const FRuntimeMeshDataPtr& InLinkedMeshData, int32 InSectionIndex, ESectionUpdateFlags InUpdateFlags, bool bInTangentsHighPrecision, bool bInUVsHighPrecision, int32 bInUVCount, bool bIn32BitIndices,
	TArray<uint8>* PositionStreamData, TArray<uint8>* TangentStreamData, TArray<uint8>* UVStreamData, TArray<uint8>* ColorStreamData, TArray<uint8>* IndexStreamData, FRuntimeMeshLockProvider* InSyncObject, bool bIsReadonly)
	: FRuntimeMeshAccessor(bInTangentsHighPrecision, bInUVsHighPrecision, bInUVCount, bIn32BitIndices, PositionStreamData, TangentStreamData, UVStreamData, ColorStreamData, IndexStreamData, bIsReadonly)
	, FRuntimeMeshScopeLock(InSyncObject, true)
	, LinkedMeshData(InLinkedMeshData), SectionIndex(InSectionIndex), UpdateFlags(InUpdateFlags)
{

}

FRuntimeMeshScopedUpdater::~FRuntimeMeshScopedUpdater()
{
	Cancel();
}

void FRuntimeMeshScopedUpdater::Commit(bool bNeedsPositionUpdate, bool bNeedsNormalTangentUpdate, bool bNeedsColorUpdate, bool bNeedsUVUpdate, bool bNeedsIndexUpdate)
{
	check(!IsReadonly());

	ERuntimeMeshBuffersToUpdate BuffersToUpdate;
	BuffersToUpdate |= bNeedsPositionUpdate ? ERuntimeMeshBuffersToUpdate::PositionBuffer : ERuntimeMeshBuffersToUpdate::None;
	BuffersToUpdate |= bNeedsNormalTangentUpdate ? ERuntimeMeshBuffersToUpdate::TangentBuffer : ERuntimeMeshBuffersToUpdate::None;
	BuffersToUpdate |= bNeedsColorUpdate ? ERuntimeMeshBuffersToUpdate::ColorBuffer : ERuntimeMeshBuffersToUpdate::None;
	BuffersToUpdate |= bNeedsUVUpdate ? ERuntimeMeshBuffersToUpdate::UVBuffer : ERuntimeMeshBuffersToUpdate::None;
	BuffersToUpdate |= bNeedsIndexUpdate ? ERuntimeMeshBuffersToUpdate::IndexBuffer : ERuntimeMeshBuffersToUpdate::None;

	LinkedMeshData->EndSectionUpdate(this, BuffersToUpdate);

	// Release the mesh and lock.
	FRuntimeMeshAccessor::Unlink();
	FRuntimeMeshScopeLock::Unlock();
}

void FRuntimeMeshScopedUpdater::Commit(const FBox& BoundingBox, bool bNeedsPositionUpdate, bool bNeedsNormalTangentUpdate, bool bNeedsColorUpdate, bool bNeedsUVUpdate, bool bNeedsIndexUpdate)
{
	check(!IsReadonly());

	ERuntimeMeshBuffersToUpdate BuffersToUpdate;
	BuffersToUpdate |= bNeedsPositionUpdate ? ERuntimeMeshBuffersToUpdate::PositionBuffer : ERuntimeMeshBuffersToUpdate::None;
	BuffersToUpdate |= bNeedsNormalTangentUpdate ? ERuntimeMeshBuffersToUpdate::TangentBuffer : ERuntimeMeshBuffersToUpdate::None;
	BuffersToUpdate |= bNeedsColorUpdate ? ERuntimeMeshBuffersToUpdate::ColorBuffer : ERuntimeMeshBuffersToUpdate::None;
	BuffersToUpdate |= bNeedsUVUpdate ? ERuntimeMeshBuffersToUpdate::UVBuffer : ERuntimeMeshBuffersToUpdate::None;
	BuffersToUpdate |= bNeedsIndexUpdate ? ERuntimeMeshBuffersToUpdate::IndexBuffer : ERuntimeMeshBuffersToUpdate::None;

	LinkedMeshData->EndSectionUpdate(this, BuffersToUpdate, &BoundingBox);

	// Release the mesh and lock.
	FRuntimeMeshAccessor::Unlink();
	FRuntimeMeshScopeLock::Unlock();
}

void FRuntimeMeshScopedUpdater::Cancel()
{
	// Release the mesh and lock.
	FRuntimeMeshAccessor::Unlink();
	FRuntimeMeshScopeLock::Unlock();
}
