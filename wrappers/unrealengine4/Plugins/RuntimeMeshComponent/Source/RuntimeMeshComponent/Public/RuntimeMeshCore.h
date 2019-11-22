// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "Engine/Engine.h"
#include "Components/MeshComponent.h"
#include "Runtime/Launch/Resources/Version.h"
#include "Stats/Stats.h"
#include "HAL/CriticalSection.h"
#include "RuntimeMeshCore.generated.h"

DECLARE_STATS_GROUP(TEXT("RuntimeMesh"), STATGROUP_RuntimeMesh, STATCAT_Advanced);


#define RUNTIMEMESH_MAXTEXCOORDS MAX_TEXCOORDS

#define RUNTIMEMESH_ENABLE_DEBUG_RENDERING (!(UE_BUILD_SHIPPING || UE_BUILD_TEST) || WITH_EDITOR)

// Custom version for runtime mesh serialization
namespace FRuntimeMeshVersion
{
	enum Type
	{
		Initial = 0,
		TemplatedVertexFix = 1,
		SerializationOptional = 2,
		DualVertexBuffer = 3,
		SerializationV2 = 4,

		// This was a total overhaul of the component, and with it the serialization
		RuntimeMeshComponentV3 = 5,

		// This was the 4.19 RHI upgrades requiring an internal restructure
		RuntimeMeshComponentUE4_19 = 6,

		SerializationUpgradeToConfigurable = 7,

		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};

	// The GUID for this custom version
	const static FGuid GUID = FGuid(0xEE48714B, 0x8A2C4652, 0x98BE40E6, 0xCB7EF0E6);
};


class FRuntimeMeshVertexFactory;
struct FRuntimeMeshVertexStreamStructure;

 template<typename T>
 struct FRuntimeMeshIndexTraits
 {
 	enum { IsValidIndexType = false };
 	enum { Is32Bit = false };
 };
 
 template<>
 struct FRuntimeMeshIndexTraits<uint16>
 {
 	enum { IsValidIndexType = true };
 	enum { Is32Bit = false };
 };
 
 template<>
 struct FRuntimeMeshIndexTraits<uint32>
 {
 	enum { IsValidIndexType = true };
 	enum { Is32Bit = true };
 };
 
 template<>
 struct FRuntimeMeshIndexTraits<int32>
 {
 	enum { IsValidIndexType = true };
 	enum { Is32Bit = true };
 };


enum class ERuntimeMeshBuffersToUpdate : uint8
{
	None = 0x0,
	PositionBuffer = 0x1,
	TangentBuffer = 0x2,
	UVBuffer = 0x4,
	ColorBuffer = 0x8, 

	AllVertexBuffers = PositionBuffer | TangentBuffer | UVBuffer | ColorBuffer,

	IndexBuffer = 0x10,
	AdjacencyIndexBuffer = 0x20
};
ENUM_CLASS_FLAGS(ERuntimeMeshBuffersToUpdate);

/* Mobility status for a RuntimeMeshComponent */
UENUM(BlueprintType)
enum class ERuntimeMeshMobility : uint8
{
	Movable,
	Stationary,
	Static
};

/* Update frequency for a section. Used to optimize for update or render speed*/
UENUM(BlueprintType)
enum class EUpdateFrequency : uint8
{
	/* Tries to skip recreating the scene proxy if possible. */
	Average UMETA(DisplayName = "Average"),
	/* Tries to skip recreating the scene proxy if possible and optimizes the buffers for frequent updates. */
	Frequent UMETA(DisplayName = "Frequent"),
	/* If the component is static it will try to use the static rendering path (this will force a recreate of the scene proxy) */
	Infrequent UMETA(DisplayName = "Infrequent")
};

/* Control flags for update actions */
enum class ESectionUpdateFlags
{
	None = 0x0,

	// 	/** 
	// 	*	This will use move-assignment when copying the supplied vertices/triangles into the section.
	// 	*	This is faster as it doesn't require copying the data.
	// 	*
	// 	*	CAUTION: This means that your copy of the arrays will be cleared!
	// 	*/
	// 	MoveArrays = 0x1,

	/**
	*	Should the normals and tangents be calculated automatically?
	*	To do this manually see RuntimeMeshLibrary::CalculateTangentsForMesh()
	*	This version calculates smooth tangents, so it will smooth across vertices sharing position
	*/
	CalculateNormalTangent = 0x2,

	/**
	*	Should the normals and tangents be calculated automatically?
	*	To do this manually see RuntimeMeshLibrary::CalculateTangentsForMesh()
	*	This version calculates hard tangents, so it will not smooth across vertices sharing position
	*/
	CalculateNormalTangentHard = 0x4,

	/**
	*	Should the tessellation indices be calculated to support tessellation?
	*	To do this manually see RuntimeMeshLibrary::GenerateTessellationIndexBuffer()
	*/
	CalculateTessellationIndices = 0x8,

};
ENUM_CLASS_FLAGS(ESectionUpdateFlags)

/*
*	Configuration flag for the collision cooking to prioritize cooking speed or collision performance.
*/
UENUM(BlueprintType)
enum class ERuntimeMeshCollisionCookingMode : uint8
{
	/*
	*	Favors runtime collision performance of cooking speed.
	*	This means that cooking a new mesh will be slower, but collision will be faster.
	*/
	CollisionPerformance UMETA(DisplayName = "Collision Performance"),

	/*
	*	Favors cooking speed over collision performance.
	*	This means that cooking a new mesh will be faster, but collision will be slower.
	*/
	CookingPerformance UMETA(DisplayName = "Cooking Performance"),
};



/**
*	Struct used to specify a tangent vector for a vertex
*	The Y tangent is computed from the cross product of the vertex normal (Tangent Z) and the TangentX member.
*/
USTRUCT(BlueprintType)
struct FRuntimeMeshTangent
{
	GENERATED_BODY()

	/** Direction of X tangent for this vertex */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tangent)
	FVector TangentX;

	/** Bool that indicates whether we should flip the Y tangent when we compute it using cross product */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Tangent)
	bool bFlipTangentY;

	FRuntimeMeshTangent()
		: TangentX(1.f, 0.f, 0.f)
		, bFlipTangentY(false)
	{}

	FRuntimeMeshTangent(float X, float Y, float Z, bool bInFlipTangentY = false)
		: TangentX(X, Y, Z)
		, bFlipTangentY(bInFlipTangentY)
	{}

	FRuntimeMeshTangent(FVector InTangentX, bool bInFlipTangentY = false)
		: TangentX(InTangentX)
		, bFlipTangentY(bInFlipTangentY)
	{}

	void ModifyNormal(FVector4& Normal) const { Normal.W = bFlipTangentY ? -1 : 1; }
	void ModifyNormal(FPackedNormal& Normal) const { Normal.Vector.W = bFlipTangentY ? 0 : MAX_uint8; }
	void ModifyNormal(FPackedRGBA16N& Normal) const { Normal.W = bFlipTangentY ? 0 : MAX_uint16; }
};


















struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshVertexStreamStructureElement
{
	uint8 Offset;
	uint8 Stride;
	EVertexElementType Type;

	FRuntimeMeshVertexStreamStructureElement() : Offset(-1), Stride(-1), Type(EVertexElementType::VET_None) { }
	FRuntimeMeshVertexStreamStructureElement(uint8 InOffset, uint8 InStride, EVertexElementType InType)
		: Offset(InOffset), Stride(InStride), Type(InType) { }

	bool IsValid() const { return Offset >= 0 && Stride >= 0 && Type != EVertexElementType::VET_None; }

	bool operator==(const FRuntimeMeshVertexStreamStructureElement& Other) const;

	bool operator!=(const FRuntimeMeshVertexStreamStructureElement& Other) const;

	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshVertexStreamStructureElement& Element)
	{
		Ar << Element.Offset;
		Ar << Element.Stride;

		int32 TypeValue = static_cast<int32>(Element.Type);
		Ar << TypeValue;
		Element.Type = static_cast<EVertexElementType>(TypeValue);

		return Ar;
	}
};

#define RUNTIMEMESH_VERTEXSTREAMCOMPONENT(VertexType, Member, MemberType) \
	FRuntimeMeshVertexStreamStructureElement(STRUCT_OFFSET(VertexType,Member),sizeof(VertexType),MemberType)

struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshVertexStreamStructure
{
	FRuntimeMeshVertexStreamStructureElement Position;
	FRuntimeMeshVertexStreamStructureElement Normal;
	FRuntimeMeshVertexStreamStructureElement Tangent;
	FRuntimeMeshVertexStreamStructureElement Color;
	TArray<FRuntimeMeshVertexStreamStructureElement, TInlineAllocator<RUNTIMEMESH_MAXTEXCOORDS>> UVs;

	bool operator==(const FRuntimeMeshVertexStreamStructure& Other) const;

	bool operator!=(const FRuntimeMeshVertexStreamStructure& Other) const;

	bool HasAnyElements() const;

	bool HasUVs() const;

	uint8 CalculateStride() const;

	bool IsValid() const;

	bool HasNoOverlap(const FRuntimeMeshVertexStreamStructure& Other) const;

	static bool ValidTripleStream(const FRuntimeMeshVertexStreamStructure& Stream1, const FRuntimeMeshVertexStreamStructure& Stream2, const FRuntimeMeshVertexStreamStructure& Stream3);

	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshVertexStreamStructure& Structure)
	{
		Ar << Structure.Position;
		Ar << Structure.Normal;
		Ar << Structure.Tangent;
		Ar << Structure.Color;
		Ar << Structure.UVs;

		return Ar;
	}
};


struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshNullVertex
{
	static const FRuntimeMeshVertexStreamStructure GetVertexStructure()
	{
		return FRuntimeMeshVertexStreamStructure();
	}
};

template<typename VertexType>
inline FRuntimeMeshVertexStreamStructure GetStreamStructure()
{
	return VertexType::GetVertexStructure();
}

template<>
inline FRuntimeMeshVertexStreamStructure GetStreamStructure<FVector>()
{
	FRuntimeMeshVertexStreamStructure Structure;
	Structure.Position = FRuntimeMeshVertexStreamStructureElement(0, sizeof(FVector), VET_Float3);
	return Structure;
}

template<>
inline FRuntimeMeshVertexStreamStructure GetStreamStructure<FColor>()
{
	FRuntimeMeshVertexStreamStructure Structure;
	Structure.Color = FRuntimeMeshVertexStreamStructureElement(0, sizeof(FColor), VET_Color);
	return Structure;
}





struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshLockProvider
{
	virtual ~FRuntimeMeshLockProvider() { }
	virtual void Lock(bool bIgnoreThreadIfNullLock = false) = 0;
	virtual void Unlock() = 0;
	virtual bool IsThreadSafe() const { return false; }
};



struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshNullLockProvider : public FRuntimeMeshLockProvider
{
private:
	bool bIsIgnoringLock;
public:
	FRuntimeMeshNullLockProvider() { check(IsInGameThread()); }
	virtual ~FRuntimeMeshNullLockProvider() { }
	virtual void Lock(bool bIgnoreThreadIfNullLock = false) override 
	{ 
		bIsIgnoringLock = bIgnoreThreadIfNullLock;
		if (!bIgnoreThreadIfNullLock) 
		{ 
			check(IsInGameThread()); 
		} 
	}
	virtual void Unlock() override 
	{ 
		if (!bIsIgnoringLock)
		{
			check(IsInGameThread());
		}
	}
};

struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshMutexLockProvider : public FRuntimeMeshLockProvider
{
private:
	FCriticalSection SyncObject;

public:

	FRuntimeMeshMutexLockProvider() { check(IsInGameThread()); }
	virtual ~FRuntimeMeshMutexLockProvider() { }
	virtual void Lock(bool bIgnoreThreadIfNullLock = false) override { SyncObject.Lock(); }
	virtual void Unlock() override { SyncObject.Unlock(); }
	virtual bool IsThreadSafe() const override { return true; }
};


class RUNTIMEMESHCOMPONENT_API FRuntimeMeshScopeLock
{

private:
	// Holds the synchronization object to aggregate and scope manage.
	FRuntimeMeshLockProvider* SynchObject;

public:

	/**
	* Constructor that performs a lock on the synchronization object
	*
	* @param InSynchObject The synchronization object to manage
	*/
	FRuntimeMeshScopeLock(const FRuntimeMeshLockProvider* InSyncObject, bool bIsAlreadyLocked = false, bool bIgnoreThreadIfNullLock = false)
		: SynchObject(const_cast<FRuntimeMeshLockProvider*>(InSyncObject))
	{
		check(SynchObject);
		if (!bIsAlreadyLocked)
		{
			SynchObject->Lock(bIgnoreThreadIfNullLock);
		}
	}

	FRuntimeMeshScopeLock(const TUniquePtr<FRuntimeMeshLockProvider>& InSyncObject, bool bIsAlreadyLocked = false, bool bIgnoreThreadIfNullLock = false)
		: SynchObject(InSyncObject.Get())
	{
		check(SynchObject);
		if (!bIsAlreadyLocked)
		{
			SynchObject->Lock(bIgnoreThreadIfNullLock);
		}
	}

	/** Destructor that performs a release on the synchronization object. */
	~FRuntimeMeshScopeLock()
	{
		Unlock();
	}

	void Unlock()
	{
		if (SynchObject)
		{
			SynchObject->Unlock();
			SynchObject = nullptr;
		}
	}
private:

	/** Default constructor (hidden on purpose). */
	FRuntimeMeshScopeLock();

	/** Copy constructor( hidden on purpose). */
	FRuntimeMeshScopeLock(const FRuntimeMeshScopeLock& InScopeLock);

	/** Assignment operator (hidden on purpose). */
	FRuntimeMeshScopeLock& operator=(FRuntimeMeshScopeLock& InScopeLock)
	{
		return *this;
	}
};




template<typename T>
struct FRuntimeMeshVertexTraits
{
private:
	template<typename C, C> struct ChT;

	struct FallbackPosition { FVector Position; };
	struct DerivedPosition : T, FallbackPosition { };
	template<typename C> static char(&PositionCheck(ChT<FVector FallbackPosition::*, &C::Position>*))[1];
	template<typename C> static char(&PositionCheck(...))[2];

	struct FallbackNormal { FPackedRGBA16N Normal; };
	struct DerivedNormal : T, FallbackNormal { };
	template<typename C> static char(&NormalCheck(ChT<FPackedRGBA16N FallbackNormal::*, &C::Normal>*))[1];
	template<typename C> static char(&NormalCheck(...))[2];

	struct FallbackTangent { FPackedRGBA16N Tangent; };
	struct DerivedTangent : T, FallbackTangent { };
	template<typename C> static char(&TangentCheck(ChT<FPackedRGBA16N FallbackTangent::*, &C::Tangent>*))[1];
	template<typename C> static char(&TangentCheck(...))[2];

	struct FallbackColor { FColor Color; };
	struct DerivedColor : T, FallbackColor { };
	template<typename C> static char(&ColorCheck(ChT<FColor FallbackColor::*, &C::Color>*))[1];
	template<typename C> static char(&ColorCheck(...))[2];

	struct FallbackUV0 { FVector2D UV0; };
	struct DerivedUV0 : T, FallbackUV0 { };
	template<typename C> static char(&UV0Check(ChT<FVector2D FallbackUV0::*, &C::UV0>*))[1];
	template<typename C> static char(&UV0Check(...))[2];

	struct FallbackUV1 { FVector2D UV1; };
	struct DerivedUV1 : T, FallbackUV1 { };
	template<typename C> static char(&UV1Check(ChT<FVector2D FallbackUV1::*, &C::UV1>*))[1];
	template<typename C> static char(&UV1Check(...))[2];

	struct FallbackUV2 { FVector2D UV2; };
	struct DerivedUV2 : T, FallbackUV2 { };
	template<typename C> static char(&UV2Check(ChT<FVector2D FallbackUV2::*, &C::UV2>*))[1];
	template<typename C> static char(&UV2Check(...))[2];

	struct FallbackUV3 { FVector2D UV3; };
	struct DerivedUV3 : T, FallbackUV3 { };
	template<typename C> static char(&UV3Check(ChT<FVector2D FallbackUV3::*, &C::UV3>*))[1];
	template<typename C> static char(&UV3Check(...))[2];

	struct FallbackUV4 { FVector2D UV4; };
	struct DerivedUV4 : T, FallbackUV4 { };
	template<typename C> static char(&UV4Check(ChT<FVector2D FallbackUV4::*, &C::UV4>*))[1];
	template<typename C> static char(&UV4Check(...))[2];

	struct FallbackUV5 { FVector2D UV5; };
	struct DerivedUV5 : T, FallbackUV5 { };
	template<typename C> static char(&UV5Check(ChT<FVector2D FallbackUV5::*, &C::UV5>*))[1];
	template<typename C> static char(&UV5Check(...))[2];

	struct FallbackUV6 { FVector2D UV6; };
	struct DerivedUV6 : T, FallbackUV6 { };
	template<typename C> static char(&UV6Check(ChT<FVector2D FallbackUV6::*, &C::UV6>*))[1];
	template<typename C> static char(&UV6Check(...))[2];

	struct FallbackUV7 { FVector2D UV7; };
	struct DerivedUV7 : T, FallbackUV7 { };
	template<typename C> static char(&UV7Check(ChT<FVector2D FallbackUV7::*, &C::UV7>*))[1];
	template<typename C> static char(&UV7Check(...))[2];

	template<typename A, typename B>
	struct IsSameType
	{
		static const bool Value = false;
	};

	template<typename A>
	struct IsSameType<A, A>
	{
		static const bool Value = true;
	};

	template<bool HasNormal, bool HasTangent, typename Type>
	struct TangentBasisHighPrecisionDetector
	{
		static const bool Value = false;
	};

	template<typename Type>
	struct TangentBasisHighPrecisionDetector<true, false, Type>
	{
		static const bool Value = IsSameType<decltype(DeclVal<T>().Normal), FPackedRGBA16N>::Value;
	};

	template<bool HasNormal, typename Type>
	struct TangentBasisHighPrecisionDetector<HasNormal, true, Type>
	{
		static const bool Value = IsSameType<decltype(DeclVal<T>().Tangent), FPackedRGBA16N>::Value;
	};

	template<bool HasUV0, typename Type>
	struct UVChannelHighPrecisionDetector
	{
		static const bool Value = false;
	};

	template<typename Type>
	struct UVChannelHighPrecisionDetector<true, Type>
	{
		static const bool Value = IsSameType<decltype(DeclVal<T>().UV0), FVector2D>::Value;
	};


public:
	static const bool HasPosition = sizeof(PositionCheck<DerivedPosition>(0)) == 2;
	static const bool HasNormal = sizeof(NormalCheck<DerivedNormal>(0)) == 2;
	static const bool HasTangent = sizeof(TangentCheck<DerivedTangent>(0)) == 2;
	static const bool HasColor = sizeof(ColorCheck<DerivedColor>(0)) == 2;
	static const bool HasUV0 = sizeof(UV0Check<DerivedUV0>(0)) == 2;
	static const bool HasUV1 = sizeof(UV1Check<DerivedUV1>(0)) == 2;
	static const bool HasUV2 = sizeof(UV2Check<DerivedUV2>(0)) == 2;
	static const bool HasUV3 = sizeof(UV3Check<DerivedUV3>(0)) == 2;
	static const bool HasUV4 = sizeof(UV4Check<DerivedUV4>(0)) == 2;
	static const bool HasUV5 = sizeof(UV5Check<DerivedUV5>(0)) == 2;
	static const bool HasUV6 = sizeof(UV6Check<DerivedUV6>(0)) == 2;
	static const bool HasUV7 = sizeof(UV7Check<DerivedUV7>(0)) == 2;
	static const int32 NumUVChannels =
		(HasUV0 ? 1 : 0) +
		(HasUV1 ? 1 : 0) +
		(HasUV2 ? 1 : 0) +
		(HasUV3 ? 1 : 0) +
		(HasUV4 ? 1 : 0) +
		(HasUV5 ? 1 : 0) +
		(HasUV6 ? 1 : 0) +
		(HasUV7 ? 1 : 0);



	static const bool HasHighPrecisionNormals = TangentBasisHighPrecisionDetector<HasNormal, HasTangent, T>::Value;
	static const bool HasHighPrecisionUVs = UVChannelHighPrecisionDetector<HasUV0, T>::Value;
};


struct FRuntimeMeshVertexTypeTraitsAggregator
{
	template<typename VertexType0>
	static bool IsUsingHighPrecisionTangents()
	{
		return FRuntimeMeshVertexTraits<VertexType0>::HasHighPrecisionNormals;
	}
	template<typename VertexType0, typename VertexType1>
	static bool IsUsingHighPrecisionTangents()
	{
		return FRuntimeMeshVertexTraits<VertexType0>::HasHighPrecisionNormals | FRuntimeMeshVertexTraits<VertexType1>::HasHighPrecisionNormals;
	}
	template<typename VertexType0, typename VertexType1, typename VertexType2>
	static bool IsUsingHighPrecisionTangents()
	{
		return FRuntimeMeshVertexTraits<VertexType0>::HasHighPrecisionNormals | FRuntimeMeshVertexTraits<VertexType1>::HasHighPrecisionNormals | FRuntimeMeshVertexTraits<VertexType2>::HasHighPrecisionNormals;
	}



	template<typename VertexType0>
	static bool IsUsingHighPrecisionUVs()
	{
		return FRuntimeMeshVertexTraits<VertexType0>::HasHighPrecisionUVs;
	}
	template<typename VertexType0, typename VertexType1>
	static bool IsUsingHighPrecisionUVs()
	{
		return FRuntimeMeshVertexTraits<VertexType0>::HasHighPrecisionUVs | FRuntimeMeshVertexTraits<VertexType1>::HasHighPrecisionUVs;
	}
	template<typename VertexType0, typename VertexType1, typename VertexType2>
	static bool IsUsingHighPrecisionUVs()
	{
		return FRuntimeMeshVertexTraits<VertexType0>::HasHighPrecisionUVs | FRuntimeMeshVertexTraits<VertexType1>::HasHighPrecisionUVs | FRuntimeMeshVertexTraits<VertexType2>::HasHighPrecisionUVs;
	}



	template<typename VertexType0>
	static int32 NumUVs()
	{
		return FRuntimeMeshVertexTraits<VertexType0>::NumUVChannels;
	}
	template<typename VertexType0, typename VertexType1>
	static int32 NumUVs()
	{
		return FMath::Max(FRuntimeMeshVertexTraits<VertexType0>::NumUVChannels, FRuntimeMeshVertexTraits<VertexType1>::NumUVChannels);
	}
	template<typename VertexType0, typename VertexType1, typename VertexType2>
	static int32 NumUVs()
	{
		return FMath::Max(FRuntimeMeshVertexTraits<VertexType0>::NumUVChannels, FMath::Max(FRuntimeMeshVertexTraits<VertexType1>::NumUVChannels, FRuntimeMeshVertexTraits<VertexType2>::NumUVChannels));
	}
};
