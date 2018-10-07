// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshRendering.h"
#include "RuntimeMeshComponentPlugin.h"
#include "RuntimeMeshSectionProxy.h"


FRuntimeMeshVertexBuffer::FRuntimeMeshVertexBuffer(EUpdateFrequency InUpdateFrequency, int32 InVertexSize)
	: UsageFlags(InUpdateFrequency == EUpdateFrequency::Frequent? BUF_Dynamic : BUF_Static)
	, VertexSize(InVertexSize)
	, NumVertices(0)
	, ShaderResourceView(nullptr)
{
}

void FRuntimeMeshVertexBuffer::Reset(int32 InNumVertices)
{
	NumVertices = InNumVertices;
	ReleaseResource();
	InitResource();
}

void FRuntimeMeshVertexBuffer::InitRHI()
{
	if (VertexSize > 0 && NumVertices > 0)
	{
		// Create the vertex buffer
		FRHIResourceCreateInfo CreateInfo;
		VertexBufferRHI = RHICreateVertexBuffer(GetBufferSize(), UsageFlags | BUF_ShaderResource, CreateInfo);


#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 19
		if (RHISupportsManualVertexFetch(GMaxRHIShaderPlatform))
		{
			CreateSRV();
		}
#endif
	}
}

/* Set the size of the vertex buffer */
void FRuntimeMeshVertexBuffer::SetNum(int32 NewVertexCount)
{
	// Make sure we're not already the right size
	if (NewVertexCount != NumVertices)
	{
		NumVertices = NewVertexCount;

		// Rebuild resource
		ReleaseResource();
		InitResource();
	}
}

/* Set the data for the vertex buffer */
void FRuntimeMeshVertexBuffer::SetData(const TArray<uint8>& Data)
{
	check(Data.Num() == GetBufferSize());

	if (GetBufferSize() > 0)
	{
		check(VertexBufferRHI.IsValid());

		// Lock the vertex buffer
		void* Buffer = RHILockVertexBuffer(VertexBufferRHI, 0, Data.Num(), RLM_WriteOnly);

		// Write the vertices to the vertex buffer
		FMemory::Memcpy(Buffer, Data.GetData(), Data.Num());

		// Unlock the vertex buffer
		RHIUnlockVertexBuffer(VertexBufferRHI);
	}
}


FRuntimeMeshIndexBuffer::FRuntimeMeshIndexBuffer()
	: NumIndices(0), IndexSize(-1), UsageFlags(EBufferUsageFlags::BUF_None)
{
}

void FRuntimeMeshIndexBuffer::Reset(int32 InIndexSize, int32 InNumIndices, EUpdateFrequency InUpdateFrequency)
{
	IndexSize = InIndexSize;
	NumIndices = InNumIndices;
	UsageFlags = InUpdateFrequency == EUpdateFrequency::Frequent ? BUF_Dynamic : BUF_Static;
	ReleaseResource();
	InitResource();
}

void FRuntimeMeshIndexBuffer::InitRHI()
{
	if (IndexSize > 0 && NumIndices > 0)
	{
		// Create the index buffer
		FRHIResourceCreateInfo CreateInfo;
		IndexBufferRHI = RHICreateIndexBuffer(IndexSize, GetBufferSize(), BUF_Dynamic, CreateInfo);
	}
}

/* Set the size of the index buffer */
void FRuntimeMeshIndexBuffer::SetNum(int32 NewIndexCount)
{
	// Make sure we're not already the right size
	if (NewIndexCount != NumIndices)
	{
		NumIndices = NewIndexCount;

		// Rebuild resource
		ReleaseResource();
		InitResource();
	}
}

/* Set the data for the index buffer */
void FRuntimeMeshIndexBuffer::SetData(const TArray<uint8>& Data)
{
	check(Data.Num() == GetBufferSize());

	if (GetBufferSize() > 0)
	{
		check(IndexBufferRHI.IsValid());

		// Lock the index buffer
		void* Buffer = RHILockIndexBuffer(IndexBufferRHI, 0, Data.Num(), RLM_WriteOnly);

		// Write the indices to the vertex buffer	
		FMemory::Memcpy(Buffer, Data.GetData(), Data.Num());

		// Unlock the index buffer
		RHIUnlockIndexBuffer(IndexBufferRHI);
	}
}




FRuntimeMeshVertexFactory::FRuntimeMeshVertexFactory(ERHIFeatureLevel::Type InFeatureLevel, FRuntimeMeshSectionProxy* InSectionParent)
	: FLocalVertexFactory(
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 19
		InFeatureLevel, "FRuntimeMeshVertexFactory"
#endif
	)
	, SectionParent(InSectionParent)
{
}

/** Init function that can be called on any thread, and will do the right thing (enqueue command if called on main thread) */
void FRuntimeMeshVertexFactory::Init(FLocalVertexFactory::FDataType VertexStructure)
{
	if (IsInRenderingThread())
	{
		SetData(VertexStructure);
	}
	else
	{
		// Send the command to the render thread
		ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
			InitRuntimeMeshVertexFactory,
			FRuntimeMeshVertexFactory*, VertexFactory, this,
			FLocalVertexFactory::FDataType, VertexStructure, VertexStructure,
			{
				VertexFactory->Init(VertexStructure);
			});
	}
}

/* Gets the section visibility for static sections */


#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 19
uint64 FRuntimeMeshVertexFactory::GetStaticBatchElementVisibility(const class FSceneView& View, const struct FMeshBatch* Batch, const void* InViewCustomData) const
#else
uint64 FRuntimeMeshVertexFactory::GetStaticBatchElementVisibility(const class FSceneView& View, const struct FMeshBatch* Batch) const
#endif
{
	return SectionParent->ShouldRender();
}