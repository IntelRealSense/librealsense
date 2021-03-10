// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshCore.h"
#include "RuntimeMeshGenericVertex.h"

class FRuntimeMeshData;
using FRuntimeMeshDataPtr = TSharedPtr<FRuntimeMeshData, ESPMode::ThreadSafe>;
class FRuntimeMeshSection;
using FRuntimeMeshSectionPtr = TSharedPtr<FRuntimeMeshSection, ESPMode::ThreadSafe>;


struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshAccessorVertex
{
	FVector Position;
	FVector4 Normal;
	FVector Tangent;
	FColor Color;

	TArray<FVector2D, TInlineAllocator<RUNTIMEMESH_MAXTEXCOORDS>> UVs;
};

class RUNTIMEMESHCOMPONENT_API FRuntimeMeshVerticesAccessor
{
	bool bIsInitialized;
	bool bIsReadonly;
	TArray<uint8>* PositionStream;
	static const int32 PositionStride = 12;
	TArray<uint8>* TangentStream;
	const bool bTangentHighPrecision;
	const int32 TangentSize;
	const int32 TangentStride;
	TArray<uint8>* UVStream;
	const bool bUVHighPrecision;
	const int32 UVChannelCount;
	const int32 UVSize;
	const int32 UVStride;
	TArray<uint8>* ColorStream;
	static const int32 ColorStride = 4;

public:

	FRuntimeMeshVerticesAccessor(bool bInTangentsHighPrecision, bool bInUVsHighPrecision, int32 bInUVCount,
		TArray<uint8>* PositionStreamData, TArray<uint8>* TangentStreamData, TArray<uint8>* UVStreamData, TArray<uint8>* ColorStreamData, bool bInIsReadonly = false);
	virtual ~FRuntimeMeshVerticesAccessor();

protected:
	FRuntimeMeshVerticesAccessor(TArray<uint8>* PositionStreamData, TArray<uint8>* TangentStreamData, TArray<uint8>* UVStreamData, TArray<uint8>* ColorStreamData, bool bInIsReadonly);

	void Initialize(bool bInTangentsHighPrecision, bool bInUVsHighPrecision, int32 bInUVCount);

public:
	const bool IsUsingHighPrecisionTangents() const { return bTangentHighPrecision; }
	const bool IsUsingHighPrecisionUVs() const { return bUVHighPrecision; }

	const bool IsReadonly() const { return bIsReadonly; }

	int32 NumVertices() const;
	int32 NumUVChannels() const;

	void EmptyVertices(int32 Slack = 0);
	void SetNumVertices(int32 NewNum);

	int32 AddVertex(FVector InPosition);

	FVector GetPosition(int32 Index) const;
	FVector4 GetNormal(int32 Index) const;
	FVector GetTangent(int32 Index) const;
	FColor GetColor(int32 Index) const;
	FVector2D GetUV(int32 Index, int32 Channel = 0) const;


	void SetPosition(int32 Index, const FVector& Value);
	bool SetPositions(const int32 InsertAtIndex, const TArray<FVector>& Positions, const int32 Count, const bool bSizeToFit);
	bool SetPositions(const int32 InsertAtIndex, const FVector *const Positions, const int32 Count, const bool bSizeToFit);
	void SetNormal(int32 Index, const FVector4& Value);
	void SetTangent(int32 Index, const FVector& Value);
	void SetTangent(int32 Index, const FRuntimeMeshTangent& Value);
	void SetColor(int32 Index, const FColor& Value);
	bool SetColors(const int32 InsertAtIndex, const TArray<FColor>& Colors, const int32 Count, const bool bSizeToFit);
	bool SetColors(const int32 InsertAtIndex, const FColor *const Colors, const int32 Count, const bool bSizeToFit);
	void SetUV(int32 Index, const FVector2D& Value);
	void SetUV(int32 Index, int32 Channel, const FVector2D& Value);

	/**
	*	UV arrangement for 3 uv channels:
	*	UVs[0]: vertex 0:uvChannel[0]
	*	UVs[1]: vertex 0:uvChannel[1]
	*	UVs[2]: vertex 0:uvChannel[2]
	*	UVs[3]: vertex 1:uvChannel[0]
	*	UVs[4]: vertex 1:uvChannel[1]
	*	UVs[5]: vertex 1:uvChannel[2]
	*	UVs[6]: vertex 2:uvChannel[0]
	*	...
	*
	*	@param CountVertices			The number of vertices you want to add UV data for!
	*/
	bool SetUVs(const int32 InsertAtVertexIndex, const TArray<FVector2D>& UVs, const int32 CountVertices, const bool bSizeToFit);
	bool SetUVs(const int32 InsertAtVertexIndex, const TArray<FVector2DHalf>& UVs, const int32 CountVertices, const bool bSizeToFit);

	/**
	*	UV arrangement for 3 uv channels:
	*	UVs[0]: vertex 0:uvChannel[0]
	*	UVs[1]: vertex 0:uvChannel[1]
	*	UVs[2]: vertex 0:uvChannel[2]
	*	UVs[3]: vertex 1:uvChannel[0]
	*	UVs[4]: vertex 1:uvChannel[1]
	*	UVs[5]: vertex 1:uvChannel[2]
	*	UVs[6]: vertex 2:uvChannel[0]
	*	...
	*
	*	@param UVs						Must point to the start of the uv data for a vertex!
	*	@param CountVertices			The number of vertices you want to add UV data for!
	*/
	bool SetUVs(const int32 InsertAtVertexIndex, const FVector2D *const UVs, const int32 CountVertices, const bool bSizeToFit);
	bool SetUVs(const int32 InsertAtVertexIndex, const FVector2DHalf *const UVs, const int32 CountVertices, const bool bSizeToFit);

	void SetNormalTangent(int32 Index, FVector Normal, FRuntimeMeshTangent Tangent);
	void SetTangents(int32 Index, FVector TangentX, FVector TangentY, FVector TangentZ);

	FRuntimeMeshAccessorVertex GetVertex(int32 Index) const;
	void SetVertex(int32 Index, const FRuntimeMeshAccessorVertex& Vertex);
	int32 AddVertex(const FRuntimeMeshAccessorVertex& Vertex);


private:

	template<typename Type>
	FVector4 ConvertPackedToNormal(const Type& Input)
	{
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 20
		return Input.ToFVector4();
#else
		return Input;
#endif
	}

	template<typename Type>
	FVector ConvertPackedToTangent(const Type& Input)
	{
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 20
		return Input.ToFVector();
#else
		return Input;
#endif
	}

	template<typename Type>
	typename TEnableIf<FRuntimeMeshVertexTraits<Type>::HasPosition>::Type
		SetPositionValue(int32 Index, const Type& Vertex)
	{
		SetPosition(Index, Vertex.Position);
	}

	template<typename Type>
	typename TEnableIf<!FRuntimeMeshVertexTraits<Type>::HasPosition>::Type
		SetPositionValue(int32 Index, const Type& Vertex)
	{
	}

	template<typename Type>
	typename TEnableIf<FRuntimeMeshVertexTraits<Type>::HasNormal>::Type
		SetNormalValue(int32 Index, const Type& Vertex)
	{
		SetNormal(Index, ConvertPackedToNormal(Vertex.Normal));
	}

	template<typename Type>
	typename TEnableIf<!FRuntimeMeshVertexTraits<Type>::HasNormal>::Type
		SetNormalValue(int32 Index, const Type& Vertex)
	{
	}

	template<typename Type>
	typename TEnableIf<FRuntimeMeshVertexTraits<Type>::HasTangent>::Type
		SetTangentValue(int32 Index, const Type& Vertex)
	{
		SetTangent(Index, ConvertPackedToTangent(Vertex.Tangent));
	}

	template<typename Type>
	typename TEnableIf<!FRuntimeMeshVertexTraits<Type>::HasTangent>::Type
		SetTangentValue(int32 Index, const Type& Vertex)
	{
	}

	template<typename Type>
	typename TEnableIf<FRuntimeMeshVertexTraits<Type>::HasColor>::Type
		SetColorValue(int32 Index, const Type& Vertex)
	{
		SetColor(Index, Vertex.Color);
	}

	template<typename Type>
	typename TEnableIf<!FRuntimeMeshVertexTraits<Type>::HasColor>::Type
		SetColorValue(int32 Index, const Type& Vertex)
	{
	}




	template<typename Type>
	typename TEnableIf<FRuntimeMeshVertexTraits<Type>::HasUV0>::Type
		SetUV0Value(int32 Index, const Type& Vertex)
	{
		SetUV(Index, 0, Vertex.UV0);
	}

	template<typename Type>
	typename TEnableIf<!FRuntimeMeshVertexTraits<Type>::HasUV0>::Type
		SetUV0Value(int32 Index, const Type& Vertex)
	{
	}

	template<typename Type>
	typename TEnableIf<FRuntimeMeshVertexTraits<Type>::HasUV1>::Type
		SetUV1Value(int32 Index, const Type& Vertex)
	{
		SetUV(Index, 1, Vertex.UV1);
	}

	template<typename Type>
	typename TEnableIf<!FRuntimeMeshVertexTraits<Type>::HasUV1>::Type
		SetUV1Value(int32 Index, const Type& Vertex)
	{
	}

	template<typename Type>
	typename TEnableIf<FRuntimeMeshVertexTraits<Type>::HasUV2>::Type
		SetUV2Value(int32 Index, const Type& Vertex)
	{
		SetUV(Index, 2, Vertex.UV2);
	}

	template<typename Type>
	typename TEnableIf<!FRuntimeMeshVertexTraits<Type>::HasUV2>::Type
		SetUV2Value(int32 Index, const Type& Vertex)
	{
	}

	template<typename Type>
	typename TEnableIf<FRuntimeMeshVertexTraits<Type>::HasUV3>::Type
		SetUV3Value(int32 Index, const Type& Vertex)
	{
		SetUV(Index, 3, Vertex.UV3);
	}

	template<typename Type>
	typename TEnableIf<!FRuntimeMeshVertexTraits<Type>::HasUV3>::Type
		SetUV3Value(int32 Index, const Type& Vertex)
	{
	}

	template<typename Type>
	typename TEnableIf<FRuntimeMeshVertexTraits<Type>::HasUV4>::Type
		SetUV4Value(int32 Index, const Type& Vertex)
	{
		SetUV(Index, 4, Vertex.UV4);
	}

	template<typename Type>
	typename TEnableIf<!FRuntimeMeshVertexTraits<Type>::HasUV4>::Type
		SetUV4Value(int32 Index, const Type& Vertex)
	{
	}

	template<typename Type>
	typename TEnableIf<FRuntimeMeshVertexTraits<Type>::HasUV5>::Type
		SetUV5Value(int32 Index, const Type& Vertex)
	{
		SetUV(Index, 5, Vertex.UV5);
	}

	template<typename Type>
	typename TEnableIf<!FRuntimeMeshVertexTraits<Type>::HasUV5>::Type
		SetUV5Value(int32 Index, const Type& Vertex)
	{
	}

	template<typename Type>
	typename TEnableIf<FRuntimeMeshVertexTraits<Type>::HasUV6>::Type
		SetUV6Value(int32 Index, const Type& Vertex)
	{
		SetUV(Index, 6, Vertex.UV6);
	}

	template<typename Type>
	typename TEnableIf<!FRuntimeMeshVertexTraits<Type>::HasUV6>::Type
		SetUV6Value(int32 Index, const Type& Vertex)
	{
	}

	template<typename Type>
	typename TEnableIf<FRuntimeMeshVertexTraits<Type>::HasUV7>::Type
		SetUV7Value(int32 Index, const Type& Vertex)
	{
		SetUV(Index, 7, Vertex.UV7);
	}

	template<typename Type>
	typename TEnableIf<!FRuntimeMeshVertexTraits<Type>::HasUV7>::Type
		SetUV7Value(int32 Index, const Type& Vertex)
	{
	}




public:
	// Helper for setting vertex properties from the old style packed vertices like the generic vertex
	template<typename VertexType>
	void SetVertexProperties(int32 Index, const VertexType& Vertex)
	{
		SetPositionValue(Index, Vertex);
		SetNormalValue(Index, Vertex);
		SetTangentValue(Index, Vertex);
		SetColorValue(Index, Vertex);

		SetUV0Value(Index, Vertex);
		SetUV1Value(Index, Vertex);
		SetUV2Value(Index, Vertex);
		SetUV3Value(Index, Vertex);
		SetUV4Value(Index, Vertex);
		SetUV5Value(Index, Vertex);
		SetUV6Value(Index, Vertex);
		SetUV7Value(Index, Vertex);
	}

	template<typename VertexType0>
	void AddVertexByProperties(const VertexType0& Vertex0)
	{
		int32 NewIndex = AddSingleVertex();
		SetVertexProperties(NewIndex, Vertex0);
	}

	template<typename VertexType0, typename VertexType1>
	void AddVertexByProperties(const VertexType0& Vertex0, const VertexType1& Vertex1)
	{
		int32 NewIndex = AddSingleVertex();
		SetVertexProperties(NewIndex, Vertex0);
		SetVertexProperties(NewIndex, Vertex1);
	}

	template<typename VertexType0, typename VertexType1, typename VertexType2>
	void AddVertexByProperties(const VertexType0& Vertex0, const VertexType1& Vertex1, const VertexType2& Vertex2)
	{
		int32 NewIndex = AddSingleVertex();
		SetVertexProperties(NewIndex, Vertex0);
		SetVertexProperties(NewIndex, Vertex1);
		SetVertexProperties(NewIndex, Vertex2);
	}


protected:

	void Unlink()
	{
		bIsInitialized = false;
		PositionStream = nullptr;
		TangentStream = nullptr;
		UVStream = nullptr;
		ColorStream = nullptr;
	}

	int32 AddSingleVertex();

};

// ISO C++ (IS 14.7.2/6) and Clang want template specializations to occur outside of the class scope
template<>
inline FVector4 FRuntimeMeshVerticesAccessor::ConvertPackedToNormal<FVector4>(const FVector4& Input)
{
	return Input;
}
template<>
inline FVector FRuntimeMeshVerticesAccessor::ConvertPackedToTangent<FVector>(const FVector& Input)
{
	return Input;
}

class RUNTIMEMESHCOMPONENT_API FRuntimeMeshIndicesAccessor
{
	bool bIsInitialized;
	bool bIsReadonly;
	TArray<uint8>* IndexStream;
	bool b32BitIndices;

public:

	FRuntimeMeshIndicesAccessor(bool bIn32BitIndices, TArray<uint8>* IndexStreamData, bool bInIsReadonly = false);
	virtual ~FRuntimeMeshIndicesAccessor();

protected:
	FRuntimeMeshIndicesAccessor(TArray<uint8>* IndexStreamData, bool bInIsReadonly);

	void Initialize(bool bIn32BitIndices);

public:
	bool IsUsing32BitIndices() const { return b32BitIndices; }

	const bool IsReadonly() const { return bIsReadonly; }


	int32 NumIndices() const;
	void EmptyIndices(int32 Slack = 0);
	void SetNumIndices(int32 NewNum);
	int32 AddIndex(int32 NewIndex);
	int32 AddTriangle(int32 Index0, int32 Index1, int32 Index2);

	int32 GetIndex(int32 Index) const;
	void SetIndex(int32 Index, int32 Value);
	bool SetIndices(const int32 InsertAtIndex, const TArray<uint16>& Indices, const int32 Count, const bool bSizeToFit);
	bool SetIndices(const int32 InsertAtIndex, const uint16 *const Indices, const int32 Count, const bool bSizeToFit);
	bool SetIndices(const int32 InsertAtIndex, const TArray<int32>& Indices, const int32 Count, const bool bSizeToFit);
	bool SetIndices(const int32 InsertAtIndex, const int32 *const Indices, const int32 Count, const bool bSizeToFit);

protected:

	FORCEINLINE int32 GetIndexStride() const { return b32BitIndices ? sizeof(int32) : sizeof(uint16); }

	void Unlink()
	{
		bIsInitialized = false;
		IndexStream = nullptr;
	}
};


/**
*
*/
class RUNTIMEMESHCOMPONENT_API FRuntimeMeshAccessor : public FRuntimeMeshVerticesAccessor, public FRuntimeMeshIndicesAccessor
{


public:

	FRuntimeMeshAccessor(bool bInTangentsHighPrecision, bool bInUVsHighPrecision, int32 bInUVCount, bool bIn32BitIndices, TArray<uint8>* PositionStreamData,
		TArray<uint8>* TangentStreamData, TArray<uint8>* UVStreamData, TArray<uint8>* ColorStreamData, TArray<uint8>* IndexStreamData, bool bInIsReadonly = false);
	virtual ~FRuntimeMeshAccessor() override;

protected:
	FRuntimeMeshAccessor(TArray<uint8>* PositionStreamData, TArray<uint8>* TangentStreamData, TArray<uint8>* UVStreamData, TArray<uint8>* ColorStreamData, TArray<uint8>* IndexStreamData, bool bInIsReadonly);

	void Initialize(bool bInTangentsHighPrecision, bool bInUVsHighPrecision, int32 bInUVCount, bool bIn32BitIndices);

public:
	bool IsReadonly() const { return FRuntimeMeshVerticesAccessor::IsReadonly() || FRuntimeMeshIndicesAccessor::IsReadonly(); }

	void CopyTo(const TSharedPtr<FRuntimeMeshAccessor>& Other, bool bClearDestination = false) const;

	void Unlink()
	{
		FRuntimeMeshVerticesAccessor::Unlink();
		FRuntimeMeshIndicesAccessor::Unlink();
	}

};


/**
 * Generic mesh builder. Can work on any valid stream configuration.
 * Wraps FRuntimeMeshAccessor to provide standalone operation for creating new mesh data.
 */
class RUNTIMEMESHCOMPONENT_API FRuntimeMeshBuilder : public FRuntimeMeshAccessor
{
	TArray<uint8> PositionStream;
	TArray<uint8> TangentStream;
	TArray<uint8> UVStream;
	TArray<uint8> ColorStream;

	TArray<uint8> IndexStream;

public:
	FRuntimeMeshBuilder(bool bInTangentsHighPrecision, bool bInUVsHighPrecision, int32 bInUVCount, bool bIn32BitIndices);

	virtual ~FRuntimeMeshBuilder() override;
	
	TArray<uint8>& GetPositionStream() { return PositionStream; }
	TArray<uint8>& GetTangentStream() { return TangentStream; }
	TArray<uint8>& GetUVStream() { return UVStream; }
	TArray<uint8>& GetColorStream() { return ColorStream; }
	TArray<uint8>& GetIndexStream() { return IndexStream; }
};


class RUNTIMEMESHCOMPONENT_API FRuntimeMeshScopedUpdater : public FRuntimeMeshAccessor, private FRuntimeMeshScopeLock
{
	FRuntimeMeshDataPtr	LinkedMeshData;
	int32 SectionIndex;
	ESectionUpdateFlags UpdateFlags;
	
private:
	FRuntimeMeshScopedUpdater(const FRuntimeMeshDataPtr& InLinkedMeshData, int32 InSectionIndex, ESectionUpdateFlags InUpdateFlags, bool bInTangentsHighPrecision, bool bInUVsHighPrecision, int32 bInUVCount, bool bIn32BitIndices, TArray<uint8>* PositionStreamData,
		TArray<uint8>* TangentStreamData, TArray<uint8>* UVStreamData, TArray<uint8>* ColorStreamData, TArray<uint8>* IndexStreamData, FRuntimeMeshLockProvider* InSyncObject, bool bIsReadonly);
	
public:
	~FRuntimeMeshScopedUpdater();

	void Commit(bool bNeedsPositionUpdate = true, bool bNeedsNormalTangentUpdate = true, bool bNeedsColorUpdate = true, bool bNeedsUVUpdate = true, bool bNeedsIndexUpdate = true);
	void Commit(const FBox& BoundingBox, bool bNeedsPositionUpdate = true, bool bNeedsNormalTangentUpdate = true, bool bNeedsColorUpdate = true, bool bNeedsUVUpdate = true, bool bNeedsIndexUpdate = true);
	void Cancel();

	friend class FRuntimeMeshData;
	friend class FRuntimeMeshSection;
};



template<typename TangentType, typename UVType, typename IndexType>
FORCEINLINE TSharedRef<FRuntimeMeshBuilder> MakeRuntimeMeshBuilder()
{
	bool bIsUsingHighPrecisionUVs;
	int32 NumUVChannels;
	GetUVVertexProperties<UVType>(bIsUsingHighPrecisionUVs, NumUVChannels);

	return MakeShared<FRuntimeMeshBuilder>(GetTangentIsHighPrecision<TangentType>(), bIsUsingHighPrecisionUVs, NumUVChannels, (bool)FRuntimeMeshIndexTraits<IndexType>::Is32Bit);
}

FORCEINLINE TSharedRef<FRuntimeMeshBuilder> MakeRuntimeMeshBuilder(bool bUsingHighPrecisionTangents, bool bUsingHighPrecisionUVs, int32 NumUVs, bool bUsing32BitIndices)
{
	return MakeShared<FRuntimeMeshBuilder>(bUsingHighPrecisionTangents, bUsingHighPrecisionUVs, NumUVs, bUsing32BitIndices);
}

FORCEINLINE TSharedRef<FRuntimeMeshBuilder> MakeRuntimeMeshBuilder(const TSharedRef<const FRuntimeMeshAccessor>& StructureToCopy)
{
	return MakeShared<FRuntimeMeshBuilder>(StructureToCopy->IsUsingHighPrecisionTangents(), StructureToCopy->IsUsingHighPrecisionUVs(), StructureToCopy->NumUVChannels(), StructureToCopy->IsUsing32BitIndices());
}

FORCEINLINE TSharedRef<FRuntimeMeshBuilder> MakeRuntimeMeshBuilder(const TUniquePtr<const FRuntimeMeshAccessor>& StructureToCopy)
{
	return MakeShared<FRuntimeMeshBuilder>(StructureToCopy->IsUsingHighPrecisionTangents(), StructureToCopy->IsUsingHighPrecisionUVs(), StructureToCopy->NumUVChannels(), StructureToCopy->IsUsing32BitIndices());
}

FORCEINLINE TSharedRef<FRuntimeMeshBuilder> MakeRuntimeMeshBuilder(const FRuntimeMeshAccessor& StructureToCopy)
{
	return MakeShared<FRuntimeMeshBuilder>(StructureToCopy.IsUsingHighPrecisionTangents(), StructureToCopy.IsUsingHighPrecisionUVs(), StructureToCopy.NumUVChannels(), StructureToCopy.IsUsing32BitIndices());
}
