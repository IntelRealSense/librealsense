// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "Engine/Engine.h"
#include "RuntimeMeshCore.h"
#include "RuntimeMeshGenericVertex.h"

class FRuntimeMeshSectionProxy;
using FRuntimeMeshSectionProxyPtr = TSharedPtr<FRuntimeMeshSectionProxy, ESPMode::NotThreadSafe>;
using FRuntimeMeshSectionProxyWeakPtr = TWeakPtr<FRuntimeMeshSectionProxy, ESPMode::NotThreadSafe>;


/** Single vertex buffer to hold one vertex stream within a section */
class FRuntimeMeshVertexBuffer : public FVertexBuffer
{
protected:
	/** The buffer configuration to use */
	const EBufferUsageFlags UsageFlags;

	/** Size of a single vertex */
	const int32 VertexSize;

	/** The number of vertices this buffer is currently allocated to hold */
	int32 NumVertices;

	/** Shader Resource View for this buffer */
	FShaderResourceViewRHIRef ShaderResourceView;

public:

	FRuntimeMeshVertexBuffer(EUpdateFrequency InUpdateFrequency, int32 InVertexSize);

	~FRuntimeMeshVertexBuffer() {}

	void Reset(int32 InNumVertices);

	virtual void InitRHI() override;

	/** Get the size of the vertex buffer */
	int32 Num() { return NumVertices; }

	/** Gets the full allocated size of the buffer (Equal to VertexSize * NumVertices) */
	int32 GetBufferSize() const { return NumVertices * VertexSize; }

	/* Set the size of the vertex buffer */
	void SetNum(int32 NewVertexCount);

	/* Set the data for the vertex buffer */
	void SetData(const TArray<uint8>& Data);

	virtual void Bind(FLocalVertexFactory::FDataType& DataType) = 0;

protected:
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 19
	virtual void CreateSRV() = 0;
#endif
};


class FRuntimeMeshPositionVertexBuffer : public FRuntimeMeshVertexBuffer
{
public:
	FRuntimeMeshPositionVertexBuffer(EUpdateFrequency InUpdateFrequency)
		: FRuntimeMeshVertexBuffer(InUpdateFrequency, sizeof(FVector))
	{

	}

	virtual void Bind(FLocalVertexFactory::FDataType& DataType) override 
	{
		DataType.PositionComponent = FVertexStreamComponent(this, 0, 12, VET_Float3);
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 19
		DataType.PositionComponentSRV = ShaderResourceView;
#endif
	}

protected:
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 19
	virtual void CreateSRV() override 
	{
		ShaderResourceView = RHICreateShaderResourceView(VertexBufferRHI, 4, PF_R32_FLOAT);
	}
#endif
};

class FRuntimeMeshTangentsVertexBuffer : public FRuntimeMeshVertexBuffer
{
	/** Whether this tangent buffer is using high precision tangents */
	bool bUseHighPrecision;

public:
	FRuntimeMeshTangentsVertexBuffer(EUpdateFrequency InUpdateFrequency, bool bInUseHighPrecision)
		: FRuntimeMeshVertexBuffer(InUpdateFrequency, (bInUseHighPrecision ? sizeof(FPackedRGBA16N) : sizeof(FPackedNormal)) * 2)
		, bUseHighPrecision(bInUseHighPrecision)
	{

	}

	virtual void Bind(FLocalVertexFactory::FDataType& DataType) override
	{
		uint32 TangentSizeInBytes = 0;
		uint32 TangentXOffset = 0;
		uint32 TangentZOffset = 0;
		EVertexElementType TangentElementType = VET_None;

		if (bUseHighPrecision)
		{
			TangentElementType = VET_UShort4N;
			TangentSizeInBytes = sizeof(FRuntimeMeshTangentsHighPrecision);
			TangentXOffset = offsetof(FRuntimeMeshTangentsHighPrecision, Tangent);
			TangentZOffset = offsetof(FRuntimeMeshTangentsHighPrecision, Normal);
		}
		else
		{
			TangentElementType = VET_PackedNormal;
			TangentSizeInBytes = sizeof(FRuntimeMeshTangents);
			TangentXOffset = offsetof(FRuntimeMeshTangents, Tangent);
			TangentZOffset = offsetof(FRuntimeMeshTangents, Normal);
		}

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 19
 		DataType.TangentBasisComponents[0] = FVertexStreamComponent(this, TangentXOffset, TangentSizeInBytes, TangentElementType, EVertexStreamUsage::ManualFetch);
 		DataType.TangentBasisComponents[1] = FVertexStreamComponent(this, TangentZOffset, TangentSizeInBytes, TangentElementType, EVertexStreamUsage::ManualFetch);
		DataType.TangentsSRV = ShaderResourceView;
#else
		DataType.TangentBasisComponents[0] = FVertexStreamComponent(this, TangentXOffset, TangentSizeInBytes, TangentElementType);
		DataType.TangentBasisComponents[1] = FVertexStreamComponent(this, TangentZOffset, TangentSizeInBytes, TangentElementType);
#endif
	}

protected:
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 20
	virtual void CreateSRV() override
	{
		ShaderResourceView = RHICreateShaderResourceView(VertexBufferRHI, bUseHighPrecision ? 8 : 4, bUseHighPrecision ? PF_R16G16B16A16_SNORM : PF_R8G8B8A8_SNORM);
	}
#elif ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 19
	virtual void CreateSRV() override
	{
		ShaderResourceView = RHICreateShaderResourceView(VertexBufferRHI, bUseHighPrecision? 8 : 4, bUseHighPrecision? PF_A16B16G16R16 : PF_R8G8B8A8);
	}
#endif
};

class FRuntimeMeshUVsVertexBuffer : public FRuntimeMeshVertexBuffer
{
	/** Whether this uv buffer is using high precision uvs */
	bool bUseHighPrecision;

	/** Num UV's in use */
	int32 NumUVs;

public:
	FRuntimeMeshUVsVertexBuffer(EUpdateFrequency InUpdateFrequency, bool bInUseHighPrecision, int32 InNumUVs)
		: FRuntimeMeshVertexBuffer(InUpdateFrequency, (bInUseHighPrecision ? sizeof(FVector2D) : sizeof(FVector2DHalf)) * InNumUVs)
		, bUseHighPrecision(bInUseHighPrecision), NumUVs(InNumUVs)
	{

	}

	virtual void Bind(FLocalVertexFactory::FDataType& DataType) override
	{ 
		DataType.TextureCoordinates.Empty();

#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 19
		DataType.NumTexCoords = NumUVs;
#endif


		EVertexElementType UVDoubleWideVertexElementType = VET_None;
		EVertexElementType UVVertexElementType = VET_None;
		uint32 UVSizeInBytes = 0;
		if (bUseHighPrecision)
		{
			UVSizeInBytes = sizeof(FVector2D);
			UVDoubleWideVertexElementType = VET_Float4;
			UVVertexElementType = VET_Float2;
		}
		else
		{
			UVSizeInBytes = sizeof(FVector2DHalf);
			UVDoubleWideVertexElementType = VET_Half4;
			UVVertexElementType = VET_Half2;
		}

		uint32 UVStride = UVSizeInBytes * NumUVs;

		int32 UVIndex;
		for (UVIndex = 0; UVIndex < NumUVs - 1; UVIndex += 2)
		{
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 19
			DataType.TextureCoordinates.Add(FVertexStreamComponent(this, UVSizeInBytes * UVIndex, UVStride, UVDoubleWideVertexElementType, EVertexStreamUsage::ManualFetch));
#else
			DataType.TextureCoordinates.Add(FVertexStreamComponent(this, UVSizeInBytes * UVIndex, UVStride, UVDoubleWideVertexElementType));
#endif
		}

		// possible last UV channel if we have an odd number
		if (UVIndex < NumUVs)
		{
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 19
			DataType.TextureCoordinates.Add(FVertexStreamComponent(this, UVSizeInBytes * UVIndex, UVStride, UVDoubleWideVertexElementType, EVertexStreamUsage::ManualFetch));
#else
			DataType.TextureCoordinates.Add(FVertexStreamComponent(this, UVSizeInBytes * UVIndex, UVStride, UVDoubleWideVertexElementType));
#endif
		}

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 19
		DataType.TextureCoordinatesSRV = ShaderResourceView;
#endif
	}

protected:
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 19
	virtual void CreateSRV() override
	{
		ShaderResourceView = RHICreateShaderResourceView(VertexBufferRHI, bUseHighPrecision ? 8 : 4, bUseHighPrecision ? PF_G32R32F : PF_G16R16F);
	}
#endif
};

class FRuntimeMeshColorVertexBuffer : public FRuntimeMeshVertexBuffer
{
public:
	FRuntimeMeshColorVertexBuffer(EUpdateFrequency InUpdateFrequency)
		: FRuntimeMeshVertexBuffer(InUpdateFrequency, sizeof(FColor))
	{

	}

	virtual void Bind(FLocalVertexFactory::FDataType& DataType) override
	{
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION >= 19
		DataType.ColorComponent = FVertexStreamComponent(this, 0, 4, EVertexElementType::VET_Color, EVertexStreamUsage::ManualFetch);
		DataType.ColorComponentsSRV = ShaderResourceView;
#else
		DataType.ColorComponent = FVertexStreamComponent(this, 0, 4, EVertexElementType::VET_Color);
#endif
	}

protected:
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 19
	virtual void CreateSRV() override
	{
		ShaderResourceView = RHICreateShaderResourceView(VertexBufferRHI, 4, PF_R8G8B8A8);
	}
#endif
};





/** Index Buffer */
class FRuntimeMeshIndexBuffer : public FIndexBuffer
{
private:
	/* The number of indices this buffer is currently allocated to hold */
	int32 NumIndices;

	/* The size of a single index*/
	int32 IndexSize;

	/* The buffer configuration to use */
	EBufferUsageFlags UsageFlags;

public:

	FRuntimeMeshIndexBuffer();

	~FRuntimeMeshIndexBuffer() {}

	void Reset(int32 InIndexSize, int32 InNumIndices, EUpdateFrequency InUpdateFrequency);

	virtual void InitRHI() override;

	/* Get the size of the index buffer */
	int32 Num() { return NumIndices; }

	/** Gets the full allocated size of the buffer (Equal to IndexSize * NumIndices) */
	int32 GetBufferSize() const { return NumIndices * IndexSize; }

	/* Set the size of the index buffer */
	void SetNum(int32 NewIndexCount);

	/* Set the data for the index buffer */
	void SetData(const TArray<uint8>& Data);
};

/** Vertex Factory */
class FRuntimeMeshVertexFactory : public FLocalVertexFactory
{
public:

	FRuntimeMeshVertexFactory(ERHIFeatureLevel::Type InFeatureLevel, FRuntimeMeshSectionProxy* InSectionParent);

	/** Init function that can be called on any thread, and will do the right thing (enqueue command if called on main thread) */
	void Init(FLocalVertexFactory::FDataType VertexStructure);

	/* Gets the section visibility for static sections */	
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 19
	virtual uint64 GetStaticBatchElementVisibility(const class FSceneView& View, const struct FMeshBatch* Batch, const void* InViewCustomData = nullptr) const override;
#else
	virtual uint64 GetStaticBatchElementVisibility(const class FSceneView& View, const struct FMeshBatch* Batch) const override;
#endif

private:
	/* Interface to the parent section for checking visibility.*/
	FRuntimeMeshSectionProxy * SectionParent;
};