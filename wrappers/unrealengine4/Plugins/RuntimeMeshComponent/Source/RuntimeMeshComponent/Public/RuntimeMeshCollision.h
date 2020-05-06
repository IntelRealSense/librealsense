// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "Engine/Engine.h"
#include "Components/MeshComponent.h"
#include "RuntimeMeshCollision.generated.h"

struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshCollisionSection
{
	TArray<FVector> VertexBuffer;

	TArray<int32> IndexBuffer;

	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshCollisionSection& Section)
	{
		Ar << Section.VertexBuffer;
		Ar << Section.IndexBuffer;
		return Ar;
	}
};

struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshCollisionConvexMesh
{
	TArray<FVector> VertexBuffer;
	FBox BoundingBox;

	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshCollisionConvexMesh& Section)
	{
		Ar << Section.VertexBuffer;
		Ar << Section.BoundingBox;
		return Ar;
	}
};


USTRUCT(BlueprintType)
struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshCollisionSphere
{
	GENERATED_USTRUCT_BODY()

	/** Position of the sphere's origin */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RuntimeMeshCollisionSphere)
	FVector Center;

	/** Radius of the sphere */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RuntimeMeshCollisionSphere)
	float Radius;

	FRuntimeMeshCollisionSphere()
		: Center(FVector::ZeroVector)
		, Radius(1)
	{

	}

	FRuntimeMeshCollisionSphere(float r)
		: Center(FVector::ZeroVector)
		, Radius(r)
	{

	}

	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshCollisionSphere& Sphere)
	{
		Ar << Sphere.Center;
		Ar << Sphere.Radius;
		return Ar;
	}
};


USTRUCT(BlueprintType)
struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshCollisionBox
{
	GENERATED_USTRUCT_BODY()

	/** Position of the box's origin */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RuntimeMeshCollisionBox)
	FVector Center;

	/** Rotation of the box */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RuntimeMeshCollisionBox)
	FRotator Rotation;

	/** Extents of the box */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RuntimeMeshCollisionBox)
	FVector Extents;


	FRuntimeMeshCollisionBox()
		: Center(FVector::ZeroVector)
		, Rotation(FRotator::ZeroRotator)
		, Extents(1, 1, 1)
	{

	}

	FRuntimeMeshCollisionBox(float s)
		: Center(FVector::ZeroVector)
		, Rotation(FRotator::ZeroRotator)
		, Extents(s, s, s)
	{

	}

	FRuntimeMeshCollisionBox(float InX, float InY, float InZ)
		: Center(FVector::ZeroVector)
		, Rotation(FRotator::ZeroRotator)
		, Extents(InX, InY, InZ)

	{

	}

	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshCollisionBox& Box)
	{
		Ar << Box.Center;
		Ar << Box.Rotation;
		Ar << Box.Extents;
		return Ar;
	}
};

USTRUCT(BlueprintType)
struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshCollisionCapsule
{
	GENERATED_USTRUCT_BODY()

	/** Position of the capsule's origin */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RuntimeMeshCollisionCapsule)
	FVector Center;

	/** Rotation of the capsule */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RuntimeMeshCollisionCapsule)
	FRotator Rotation;

	/** Radius of the capsule */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RuntimeMeshCollisionCapsule)
	float Radius;

	/** This is of line-segment ie. add Radius to both ends to find total length. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RuntimeMeshCollisionCapsule)
	float Length;

	FRuntimeMeshCollisionCapsule()
		: Center(FVector::ZeroVector)
		, Rotation(FRotator::ZeroRotator)
		, Radius(1), Length(1)

	{

	}

	FRuntimeMeshCollisionCapsule(float InRadius, float InLength)
		: Center(FVector::ZeroVector)
		, Rotation(FRotator::ZeroRotator)
		, Radius(InRadius), Length(InLength)
	{

	}

	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshCollisionCapsule& Capsule)
	{
		Ar << Capsule.Center;
		Ar << Capsule.Rotation;
		Ar << Capsule.Radius;
		Ar << Capsule.Length;
		return Ar;
	}
};