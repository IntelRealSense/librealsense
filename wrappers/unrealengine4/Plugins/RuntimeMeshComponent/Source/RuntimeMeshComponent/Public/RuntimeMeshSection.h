// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "Engine/Engine.h"
#include "RuntimeMeshCore.h"
#include "RuntimeMeshBuilder.h"

enum class ERuntimeMeshBuffersToUpdate : uint8;
struct FRuntimeMeshSectionVertexBufferParams;
struct FRuntimeMeshSectionTangentVertexBufferParams;
struct FRuntimeMeshSectionUVVertexBufferParams;
struct FRuntimeMeshSectionIndexBufferParams;
class UMaterialInterface;

class RUNTIMEMESHCOMPONENT_API FRuntimeMeshSection
{
	struct FSectionVertexBuffer
	{
	private:
		const int32 Stride;
		TArray<uint8> Data;
	public:
		FSectionVertexBuffer(int32 InStride) : Stride(InStride)
		{

		}
		virtual ~FSectionVertexBuffer() { }

		void SetData(TArray<uint8>& InVertices, bool bUseMove)
		{
			if (bUseMove)
			{
				Data = MoveTemp(InVertices);
			}
			else
			{
				Data = InVertices;
			}
		}

		template<typename VertexType>
		void SetData(const TArray<VertexType>& InVertices)
		{
			if (InVertices.Num() == 0)
			{
				Data.Empty();
				return;
			}
			check(InVertices.GetTypeSize() == GetStride());

			Data.SetNum(InVertices.GetTypeSize() * InVertices.Num());
			FMemory::Memcpy(Data.GetData(), InVertices.GetData(), Data.Num());
		}

		int32 GetStride() const
		{
			return Stride;
		}

		int32 GetNumVertices() const
		{
			return Stride > 0 ? Data.Num() / Stride : 0;
		}

		TArray<uint8>& GetData() { return Data; }

		void FillUpdateParams(FRuntimeMeshSectionVertexBufferParams& Params);

		friend FArchive& operator <<(FArchive& Ar, FSectionVertexBuffer& Buffer)
		{
			Buffer.Serialize(Ar);
			return Ar;
		}

	protected:
		virtual void Serialize(FArchive& Ar)
		{
			if (Ar.CustomVer(FRuntimeMeshVersion::GUID) < FRuntimeMeshVersion::RuntimeMeshComponentUE4_19)
			{
				FRuntimeMeshVertexStreamStructure VertexStructure;
				Ar << const_cast<FRuntimeMeshVertexStreamStructure&>(VertexStructure);
			}
			Ar << const_cast<int32&>(Stride);
			Ar << Data;
		}
	};

	struct FSectionPositionVertexBuffer : public FSectionVertexBuffer
	{
		FSectionPositionVertexBuffer()
			: FSectionVertexBuffer(sizeof(FVector))
		{

		}
	};

	struct FSectionTangentsVertexBuffer : public FSectionVertexBuffer
	{
	private:
		bool bUseHighPrecision;

	public:
		FSectionTangentsVertexBuffer(bool bInUseHighPrecision)
			: FSectionVertexBuffer(bInUseHighPrecision ? (sizeof(FPackedRGBA16N) * 2) : (sizeof(FPackedNormal) * 2))
			, bUseHighPrecision(bInUseHighPrecision)
		{

		}

		bool IsUsingHighPrecision() const { return bUseHighPrecision; }

		void FillUpdateParams(FRuntimeMeshSectionTangentVertexBufferParams& Params);

		virtual void Serialize(FArchive& Ar) override
		{
			if (Ar.CustomVer(FRuntimeMeshVersion::GUID) >= FRuntimeMeshVersion::RuntimeMeshComponentUE4_19)
			{
				Ar << bUseHighPrecision;
			}
			FSectionVertexBuffer::Serialize(Ar);
		}
	};

	struct FSectionUVsVertexBuffer : public FSectionVertexBuffer
	{
	private:
		bool bUseHighPrecision;
		int32 UVCount;

	public:

		FSectionUVsVertexBuffer(bool bInUseHighPrecision, int32 InUVCount)
			: FSectionVertexBuffer((bInUseHighPrecision ? sizeof(FVector2D) : sizeof(FVector2DHalf)) * InUVCount)
			, bUseHighPrecision(bInUseHighPrecision), UVCount(InUVCount)
		{

		}

		bool IsUsingHighPrecision() const { return bUseHighPrecision; }

		int32 NumUVs() const { return UVCount; }

		void FillUpdateParams(FRuntimeMeshSectionUVVertexBufferParams& Params);

		virtual void Serialize(FArchive& Ar) override
		{
			if (Ar.CustomVer(FRuntimeMeshVersion::GUID) >= FRuntimeMeshVersion::RuntimeMeshComponentUE4_19)
			{
				Ar << bUseHighPrecision;
				Ar << UVCount;
			}
			FSectionVertexBuffer::Serialize(Ar);
		}
	};

	struct FSectionColorVertexBuffer : public FSectionVertexBuffer
	{
		FSectionColorVertexBuffer()
			: FSectionVertexBuffer(sizeof(FColor))
		{

		}
	};

	struct FSectionIndexBuffer
	{
	private:
		const bool b32BitIndices;
		TArray<uint8> Data;
	public:
		FSectionIndexBuffer(bool bIn32BitIndices)
			: b32BitIndices(bIn32BitIndices)
		{

		}

		void SetData(TArray<uint8>& InIndices, bool bUseMove)
		{
			if (bUseMove)
			{
				Data = MoveTemp(InIndices);
			}
			else
			{
				Data = InIndices;
			}
		}

		template<typename IndexType>
		void SetData(const TArray<IndexType>& InIndices)
		{
			check(InIndices.GetTypeSize() == GetStride());

			Data.SetNum(InIndices.GetTypeSize() * InIndices.Num());
			FMemory::Memcpy(Data.GetData(), InIndices.GetData(), Data.Num());
		}

		int32 GetStride() const
		{
			return b32BitIndices ? 4 : 2;
		}

		bool Is32BitIndices() const
		{
			return b32BitIndices;
		}

		int32 GetNumIndices() const
		{
			return Data.Num() / GetStride();
		}

		TArray<uint8>& GetData() { return Data; }

		void FillUpdateParams(FRuntimeMeshSectionIndexBufferParams& Params);

		friend FArchive& operator <<(FArchive& Ar, FSectionIndexBuffer& Buffer)
		{
			Ar << const_cast<bool&>(Buffer.b32BitIndices);
			Ar << Buffer.Data;
			return Ar;
		}
	};


	const EUpdateFrequency UpdateFrequency;

	/** Vertex buffer containing the positions for this section */
	FSectionPositionVertexBuffer PositionBuffer;

	/** Vertex buffer containing the tangents for this section */
	FSectionTangentsVertexBuffer TangentsBuffer;

	/** Vertex buffer containing the UVs for this section */
	FSectionUVsVertexBuffer UVsBuffer;

	/** Vertex buffer containing the colors for this section */
	FSectionColorVertexBuffer ColorBuffer;

	FSectionIndexBuffer IndexBuffer;
	FSectionIndexBuffer AdjacencyIndexBuffer;

	FBox LocalBoundingBox;

	bool bCollisionEnabled;

	bool bIsVisible;

	bool bCastsShadow;

	//	TUniquePtr<FRuntimeMeshLockProvider> SyncRoot;
public:
	FRuntimeMeshSection(FArchive& Ar);
	FRuntimeMeshSection(bool bInUseHighPrecisionTangents, bool bInUseHighPrecisionUVs, int32 InNumUVs, bool b32BitIndices, EUpdateFrequency InUpdateFrequency/*, FRuntimeMeshLockProvider* InSyncRoot*/);

	// 	void SetNewLockProvider(FRuntimeMeshLockProvider* NewSyncRoot)
	// 	{
	// 		SyncRoot.Reset(NewSyncRoot);
	// 	}

	//	FRuntimeMeshLockProvider GetSyncRoot() { return SyncRoot->Get(); }

	bool IsCollisionEnabled() const { return bCollisionEnabled; }
	bool IsVisible() const { return bIsVisible; }
	bool ShouldRender() const { return IsVisible() && HasValidMeshData(); }
	bool CastsShadow() const { return bCastsShadow; }
	EUpdateFrequency GetUpdateFrequency() const { return UpdateFrequency; }
	FBox GetBoundingBox() const { return LocalBoundingBox; }

	int32 GetNumVertices() const { return PositionBuffer.GetNumVertices(); }
	int32 GetNumIndices() const { return IndexBuffer.GetNumIndices(); }

	bool HasValidMeshData() const {
		if (IndexBuffer.GetNumIndices() <= 0)
			return false;
		if (PositionBuffer.GetNumVertices() <= 0)
			return false;
		if (TangentsBuffer.GetNumVertices() != 0 && TangentsBuffer.GetNumVertices() != PositionBuffer.GetNumVertices())
			return false;
		if (UVsBuffer.GetNumVertices() != 0 && UVsBuffer.GetNumVertices() != PositionBuffer.GetNumVertices())
			return false;
		if (ColorBuffer.GetNumVertices() != 0 && ColorBuffer.GetNumVertices() != PositionBuffer.GetNumVertices())
			return false;
		return true;
	}

	void SetVisible(bool bNewVisible)
	{
		bIsVisible = bNewVisible;
	}
	void SetCastsShadow(bool bNewCastsShadow)
	{
		bCastsShadow = bNewCastsShadow;
	}
	void SetCollisionEnabled(bool bNewCollision)
	{
		bCollisionEnabled = bNewCollision;
	}

	void UpdatePositionBuffer(TArray<uint8>& InVertices, bool bUseMove)
	{
		PositionBuffer.SetData(InVertices, bUseMove);
		UpdateBoundingBox();
	}

	template<typename VertexType>
	void UpdatePositionBuffer(const TArray<VertexType>& InVertices, const FBox* BoundingBox = nullptr)
	{
		PositionBuffer.SetData(InVertices);

		if (BoundingBox)
		{
			LocalBoundingBox = *BoundingBox;
		}
		else
		{
			UpdateBoundingBox();
		}
	}

	void UpdateTangentsBuffer(TArray<uint8>& InVertices, bool bUseMove)
	{
		TangentsBuffer.SetData(InVertices, bUseMove);
	}

	template<typename VertexType>
	void UpdateTangentsBuffer(const TArray<VertexType>& InVertices)
	{
		TangentsBuffer.SetData(InVertices);
	}

	void UpdateUVsBuffer(TArray<uint8>& InVertices, bool bUseMove)
	{
		UVsBuffer.SetData(InVertices, bUseMove);
	}

	template<typename VertexType>
	void UpdateUVsBuffer(const TArray<VertexType>& InVertices)
	{
		UVsBuffer.SetData(InVertices);
	}

	void UpdateColorBuffer(TArray<uint8>& InVertices, bool bUseMove)
	{
		ColorBuffer.SetData(InVertices, bUseMove);
	}

	template<typename VertexType>
	void UpdateColorBuffer(const TArray<VertexType>& InVertices)
	{
		ColorBuffer.SetData(InVertices);
	}

	void UpdateIndexBuffer(TArray<uint8>& InIndices, bool bUseMove)
	{
		IndexBuffer.SetData(InIndices, bUseMove);
	}

	template<typename IndexType>
	void UpdateIndexBuffer(const TArray<IndexType>& InIndices)
	{
		IndexBuffer.SetData(InIndices);
	}

	template<typename IndexType>
	void UpdateAdjacencyIndexBuffer(const TArray<IndexType>& InIndices)
	{
		AdjacencyIndexBuffer.SetData(InIndices);
	}

	TSharedPtr<FRuntimeMeshAccessor> GetSectionMeshAccessor()
	{
		return MakeShared<FRuntimeMeshAccessor>(TangentsBuffer.IsUsingHighPrecision(), UVsBuffer.IsUsingHighPrecision(), UVsBuffer.NumUVs(), IndexBuffer.Is32BitIndices(),
			&PositionBuffer.GetData(), &TangentsBuffer.GetData(), &UVsBuffer.GetData(), &ColorBuffer.GetData(), &IndexBuffer.GetData());
	}

	TUniquePtr<FRuntimeMeshScopedUpdater> GetSectionMeshUpdater(const FRuntimeMeshDataPtr& ParentData, int32 SectionIndex, ESectionUpdateFlags UpdateFlags, FRuntimeMeshLockProvider* LockProvider, bool bIsReadonly)
	{
		return TUniquePtr<FRuntimeMeshScopedUpdater>(new FRuntimeMeshScopedUpdater(ParentData, SectionIndex, UpdateFlags, TangentsBuffer.IsUsingHighPrecision(), UVsBuffer.IsUsingHighPrecision(), UVsBuffer.NumUVs(), IndexBuffer.Is32BitIndices(),
			&PositionBuffer.GetData(), &TangentsBuffer.GetData(), &UVsBuffer.GetData(), &ColorBuffer.GetData(), &IndexBuffer.GetData(), LockProvider, bIsReadonly));
	}

	TSharedPtr<FRuntimeMeshIndicesAccessor> GetTessellationIndexAccessor()
	{
		return MakeShared<FRuntimeMeshIndicesAccessor>(AdjacencyIndexBuffer.Is32BitIndices(), &AdjacencyIndexBuffer.GetData());
	}








	bool CheckTangentBuffer(bool bInUseHighPrecision) const
	{
		return TangentsBuffer.IsUsingHighPrecision() == bInUseHighPrecision;
	}

	bool CheckUVBuffer(bool bInUseHighPrecision, int32 InNumUVs) const
	{
		return UVsBuffer.IsUsingHighPrecision() == bInUseHighPrecision && UVsBuffer.NumUVs() == InNumUVs;
	}

	bool CheckIndexBufferSize(bool b32BitIndices) const
	{
		return b32BitIndices == IndexBuffer.Is32BitIndices();
	}



	TSharedPtr<struct FRuntimeMeshSectionCreationParams, ESPMode::NotThreadSafe> GetSectionCreationParams();

	TSharedPtr<struct FRuntimeMeshSectionUpdateParams, ESPMode::NotThreadSafe> GetSectionUpdateData(ERuntimeMeshBuffersToUpdate BuffersToUpdate);

	TSharedPtr<struct FRuntimeMeshSectionPropertyUpdateParams, ESPMode::NotThreadSafe> GetSectionPropertyUpdateData();

	void UpdateBoundingBox();
	void SetBoundingBox(const FBox& InBoundingBox) { LocalBoundingBox = InBoundingBox; }

	int32 GetCollisionData(TArray<FVector>& OutPositions, TArray<FTriIndices>& OutIndices, TArray<FVector2D>& OutUVs);


	friend FArchive& operator <<(FArchive& Ar, FRuntimeMeshSection& MeshData)
	{
		Ar << const_cast<EUpdateFrequency&>(MeshData.UpdateFrequency);

		if (Ar.CustomVer(FRuntimeMeshVersion::GUID) >= FRuntimeMeshVersion::RuntimeMeshComponentUE4_19)
		{
			Ar << MeshData.PositionBuffer;
			Ar << MeshData.TangentsBuffer;
			Ar << MeshData.UVsBuffer;
			Ar << MeshData.ColorBuffer;
		}
		else
		{
			// This is a hack to read the old data and ignore it
			Ar << MeshData.PositionBuffer;
			Ar << MeshData.PositionBuffer;
			Ar << MeshData.PositionBuffer;
		}


		Ar << MeshData.IndexBuffer;
		Ar << MeshData.AdjacencyIndexBuffer;

		Ar << MeshData.LocalBoundingBox;

		Ar << MeshData.bCollisionEnabled;
		Ar << MeshData.bIsVisible;
		Ar << MeshData.bCastsShadow;

		// This is a hack to read the old data and ignore it
		if (Ar.CustomVer(FRuntimeMeshVersion::GUID) < FRuntimeMeshVersion::RuntimeMeshComponentUE4_19)
		{
			TArray<FVector> NullPositions;
			TArray<uint8> NullIndices;
			MeshData.PositionBuffer.SetData(NullPositions);
			MeshData.IndexBuffer.SetData(NullIndices, false);
			MeshData.AdjacencyIndexBuffer.SetData(NullIndices, false);
		}

		return Ar;
	}
};




/** Smart pointer to a Runtime Mesh Section */
using FRuntimeMeshSectionPtr = TSharedPtr<FRuntimeMeshSection, ESPMode::ThreadSafe>;




FORCEINLINE static FArchive& operator <<(FArchive& Ar, FRuntimeMeshSectionPtr& Section)
{
	if (Ar.IsSaving())
	{
		bool bHasSection = Section.IsValid();
		Ar << bHasSection;
		if (bHasSection)
		{
			Ar << *Section.Get();
		}
	}
	else if (Ar.IsLoading())
	{
		bool bHasSection;
		Ar << bHasSection;
		if (bHasSection)
		{
			Section = MakeShared<FRuntimeMeshSection, ESPMode::ThreadSafe>(Ar);
		}
	}
	return Ar;
}