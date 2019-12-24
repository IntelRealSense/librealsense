// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshLegacySerialization.h"
#include "RuntimeMeshComponentPlugin.h"
#include "RuntimeMeshCore.h"


struct FRuntimeMeshCollisionSection_OLD
{
	TArray<FVector> VertexBuffer;
	TArray<int32> IndexBuffer;

	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshCollisionSection_OLD& Section)
	{
		Ar << Section.VertexBuffer;
		Ar << Section.IndexBuffer;
		return Ar;
	}
};

struct FRuntimeConvexCollisionSection_OLD
{
	TArray<FVector> VertexBuffer;
	FBox BoundingBox;

	friend FArchive& operator <<(FArchive& Ar, FRuntimeConvexCollisionSection_OLD& Section)
	{
		Ar << Section.VertexBuffer;
		Ar << Section.BoundingBox;
		return Ar;
	}
};


bool FRuntimeMeshComponentLegacySerialization::Serialize(FArchive& Ar)
{

	Ar.UsingCustomVersion(FRuntimeMeshVersion::GUID);

	// We bail here if this is a v3 file as the RMC doesn't serialize anything here
	if (Ar.CustomVer(FRuntimeMeshVersion::GUID) >= FRuntimeMeshVersion::RuntimeMeshComponentV3)
	{
		return false;
	}

	check(Ar.IsLoading());

	if (Ar.CustomVer(FRuntimeMeshVersion::GUID) >= FRuntimeMeshVersion::SerializationV2)
	{
		SerializeV2(Ar);
	}
	else
	{
		SerializeV1(Ar);
	}

	return true;
}


void FRuntimeMeshComponentLegacySerialization::SerializeV1(FArchive& Ar)
{
	if (Ar.CustomVer(FRuntimeMeshVersion::GUID) >= FRuntimeMeshVersion::Initial)
	{
		int32 SectionsCount;
		Ar << SectionsCount;

		for (int32 Index = 0; Index < SectionsCount; Index++)
		{
			bool IsSectionValid;
			Ar << IsSectionValid;

			if (IsSectionValid)
			{
				if (Ar.CustomVer(FRuntimeMeshVersion::GUID) >= FRuntimeMeshVersion::TemplatedVertexFix)
				{
					int32 NumUVChannels;
					bool WantsHalfPrecisionUVs;

					Ar << NumUVChannels;
					Ar << WantsHalfPrecisionUVs;


				}
				else
				{
					bool bWantsNormal;
					bool bWantsTangent;
					bool bWantsColor;
					int32 TextureChannels;

					Ar << bWantsNormal;
					Ar << bWantsTangent;
					Ar << bWantsColor;
					Ar << TextureChannels;
				}

				SectionSerialize(Ar, false);
			}
		}
	}

	if (Ar.CustomVer(FRuntimeMeshVersion::GUID) >= FRuntimeMeshVersion::SerializationOptional)
	{
		// Serialize the collision data
		TArray<FRuntimeMeshCollisionSection_OLD> MeshCollisionSections;
		Ar << MeshCollisionSections;
		TArray<FRuntimeConvexCollisionSection_OLD> ConvexCollisionSections;
		Ar << ConvexCollisionSections;
	}
}


void FRuntimeMeshComponentLegacySerialization::SerializeV2(FArchive& Ar)
{
	bool bSerializeMeshData;
	bool bUseComplexAsSimpleCollision;

	// Serialize basic settings
	Ar << bSerializeMeshData;
	Ar << bUseComplexAsSimpleCollision;

	// Serialize the number of sections...
	int32 NumSections;
	Ar << NumSections;

	// Next serialize all the sections...
	for (int32 Index = 0; Index < NumSections; Index++)
	{
		SerializeRMCSection(Ar, Index);
	}


	// Serialize the collision data
	TArray<FRuntimeMeshCollisionSection_OLD> MeshCollisionSections;
	Ar << MeshCollisionSections;
	TArray<FRuntimeConvexCollisionSection_OLD> ConvexCollisionSections;
	Ar << ConvexCollisionSections;
}



void FRuntimeMeshComponentLegacySerialization::SerializeRMCSection(FArchive& Ar, int32 SectionIndex)
{
	// Serialize the section validity (default it to section valid + type known for saving reasons)
	bool bSectionIsValid;
	Ar << bSectionIsValid;

	// If section is invalid, skip
	if (!bSectionIsValid)
	{
		return;
	}

	// Serialize section type info
	FGuid TypeGuid;
	bool bHasSeparatePositionBuffer;

	Ar << TypeGuid;
	Ar << bHasSeparatePositionBuffer;

	// This is the raw section data, but we do nothing with it here since we don't have a good conversion path
	TArray<uint8> SectionData;
	Ar << SectionData;
}

void FRuntimeMeshComponentLegacySerialization::SectionSerialize(FArchive& Ar, bool bNeedsPositionOnlyBuffer)
{
	if (Ar.CustomVer(FRuntimeMeshVersion::GUID) >= FRuntimeMeshVersion::SerializationV2)
	{
		if (bNeedsPositionOnlyBuffer)
		{
			TArray<FVector> PositionVertexBuffer;
			Ar << PositionVertexBuffer;
		}
		TArray<int32> IndexBuffer;
		Ar << IndexBuffer;
		TArray<int32> TessellationIndexBuffer;
		Ar << TessellationIndexBuffer;
		FBox LocalBoundingBox;
		Ar << LocalBoundingBox;
		bool CollisionEnabled;
		Ar << CollisionEnabled;
		bool bIsVisible;
		Ar << bIsVisible;
		bool bCastsShadow;
		Ar << bCastsShadow;
		bool bShouldUseAdjacencyIndexBuffer;
		Ar << bShouldUseAdjacencyIndexBuffer;

		// Serialize the update frequency as an int32
		int32 UpdateFreq;
		Ar << UpdateFreq;

		bool bIsLegacySectionType;
		Ar << bIsLegacySectionType;
	}
	else
	{
		if (Ar.CustomVer(FRuntimeMeshVersion::GUID) >= FRuntimeMeshVersion::DualVertexBuffer)
		{
			TArray<FVector> PositionVertexBuffer;
			Ar << PositionVertexBuffer;
		}
		TArray<int32> IndexBuffer;
		Ar << IndexBuffer;
		FBox LocalBoundingBox;
		Ar << LocalBoundingBox;
		bool CollisionEnabled;
		Ar << CollisionEnabled;
		bool bIsVisible;
		Ar << bIsVisible;
		int32 UpdateFreq;
		Ar << UpdateFreq;
	}
}
