// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshSectionProxy.h"
#include "RuntimeMeshComponentPlugin.h"




FRuntimeMeshSectionProxy::FRuntimeMeshSectionProxy(ERHIFeatureLevel::Type InFeatureLevel, FRuntimeMeshSectionCreationParamsPtr CreationData)
	: UpdateFrequency(CreationData->UpdateFrequency)
	, VertexFactory(InFeatureLevel, this)
	, PositionBuffer(CreationData->UpdateFrequency)
	, TangentsBuffer(CreationData->UpdateFrequency, CreationData->TangentsVertexBuffer.bUsingHighPrecision)
	, UVsBuffer(CreationData->UpdateFrequency, CreationData->UVsVertexBuffer.bUsingHighPrecision, CreationData->UVsVertexBuffer.NumUVs)
	, ColorBuffer(CreationData->UpdateFrequency)
	, bIsVisible(CreationData->bIsVisible)
	, bCastsShadow(CreationData->bCastsShadow)
{
	check(IsInRenderingThread());

	PositionBuffer.Reset(CreationData->PositionVertexBuffer.NumVertices);
	PositionBuffer.SetData(CreationData->PositionVertexBuffer.Data);

	TangentsBuffer.Reset(CreationData->TangentsVertexBuffer.NumVertices);
	TangentsBuffer.SetData(CreationData->TangentsVertexBuffer.Data);

	UVsBuffer.Reset(CreationData->UVsVertexBuffer.NumVertices);
	UVsBuffer.SetData(CreationData->UVsVertexBuffer.Data);

	ColorBuffer.Reset(CreationData->ColorVertexBuffer.NumVertices);
	ColorBuffer.SetData(CreationData->ColorVertexBuffer.Data);


	IndexBuffer.Reset(CreationData->IndexBuffer.b32BitIndices ? 4 : 2, CreationData->IndexBuffer.NumIndices, UpdateFrequency);
	IndexBuffer.SetData(CreationData->IndexBuffer.Data);

	AdjacencyIndexBuffer.Reset(CreationData->IndexBuffer.b32BitIndices ? 4 : 2, CreationData->AdjacencyIndexBuffer.NumIndices, UpdateFrequency);
	AdjacencyIndexBuffer.SetData(CreationData->AdjacencyIndexBuffer.Data);


#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 19
	if (CanRender())
	{
#endif
		FLocalVertexFactory::FDataType DataType;
		BuildVertexDataType(DataType);

		VertexFactory.Init(DataType);
		VertexFactory.InitResource();
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 19
	}
#endif
}

FRuntimeMeshSectionProxy::~FRuntimeMeshSectionProxy()
{
	check(IsInRenderingThread());

	PositionBuffer.ReleaseResource();
	TangentsBuffer.ReleaseResource();
	UVsBuffer.ReleaseResource();
	ColorBuffer.ReleaseResource();
	IndexBuffer.ReleaseResource();
	AdjacencyIndexBuffer.ReleaseResource();
	VertexFactory.ReleaseResource();
}

bool FRuntimeMeshSectionProxy::ShouldRender()
{
	return bIsVisible && CanRender();
}

bool FRuntimeMeshSectionProxy::CanRender()
{
	if (PositionBuffer.Num() <= 0)
	{
		return false;
	}

	if (PositionBuffer.Num() != TangentsBuffer.Num() || PositionBuffer.Num() != UVsBuffer.Num())
	{
		return false;
	}

	return true;
}

bool FRuntimeMeshSectionProxy::WantsToRenderInStaticPath() const
{
	return UpdateFrequency == EUpdateFrequency::Infrequent;
}

bool FRuntimeMeshSectionProxy::CastsShadow() const
{
	return bCastsShadow;
}


void FRuntimeMeshSectionProxy::CreateMeshBatch(FMeshBatch& MeshBatch, bool bWantsAdjacencyInfo)
{
	MeshBatch.VertexFactory = &VertexFactory;

	MeshBatch.Type = bWantsAdjacencyInfo ? PT_12_ControlPointPatchList : PT_TriangleList;

	MeshBatch.DepthPriorityGroup = SDPG_World;
	MeshBatch.CastShadow = bCastsShadow;

	// Make sure that if the material wants adjacency information, that you supply it
	check(!bWantsAdjacencyInfo || AdjacencyIndexBuffer.Num() > 0);

	FRuntimeMeshIndexBuffer* CurrentIndexBuffer = bWantsAdjacencyInfo ? &AdjacencyIndexBuffer : &IndexBuffer;

	int32 NumIndicesPerTriangle = bWantsAdjacencyInfo ? 12 : 3;
	int32 NumPrimitives = CurrentIndexBuffer->Num() / NumIndicesPerTriangle;

	FMeshBatchElement& BatchElement = MeshBatch.Elements[0];
	BatchElement.IndexBuffer = CurrentIndexBuffer;
	BatchElement.FirstIndex = 0;
	BatchElement.NumPrimitives = NumPrimitives;
	BatchElement.MinVertexIndex = 0;
	BatchElement.MaxVertexIndex = PositionBuffer.Num() - 1;
}

void FRuntimeMeshSectionProxy::BuildVertexDataType(FLocalVertexFactory::FDataType& DataType)
{
	PositionBuffer.Bind(DataType);
	TangentsBuffer.Bind(DataType);
	UVsBuffer.Bind(DataType);
	ColorBuffer.Bind(DataType);
}

void FRuntimeMeshSectionProxy::FinishUpdate_RenderThread(FRuntimeMeshSectionUpdateParamsPtr UpdateData)
{
	check(IsInRenderingThread());

	// Update position buffer
	if (!!(UpdateData->BuffersToUpdate & ERuntimeMeshBuffersToUpdate::PositionBuffer))
	{
		PositionBuffer.Reset(UpdateData->PositionVertexBuffer.NumVertices);
		PositionBuffer.SetData(UpdateData->PositionVertexBuffer.Data);
	}

	// Update tangent buffer
	if (!!(UpdateData->BuffersToUpdate & ERuntimeMeshBuffersToUpdate::TangentBuffer))
	{
		TangentsBuffer.SetNum(UpdateData->TangentsVertexBuffer.NumVertices);
		TangentsBuffer.SetData(UpdateData->TangentsVertexBuffer.Data);
	}

	// Update uv buffer
	if (!!(UpdateData->BuffersToUpdate & ERuntimeMeshBuffersToUpdate::UVBuffer))
	{
		UVsBuffer.SetNum(UpdateData->UVsVertexBuffer.NumVertices);
		UVsBuffer.SetData(UpdateData->UVsVertexBuffer.Data);
	}

	// Update color buffer
	if (!!(UpdateData->BuffersToUpdate & ERuntimeMeshBuffersToUpdate::ColorBuffer))
	{
		ColorBuffer.SetNum(UpdateData->ColorVertexBuffer.NumVertices);
		ColorBuffer.SetData(UpdateData->ColorVertexBuffer.Data);
	}

	// Update index buffer
	if (!!(UpdateData->BuffersToUpdate & ERuntimeMeshBuffersToUpdate::IndexBuffer))
	{
		IndexBuffer.SetNum(UpdateData->IndexBuffer.NumIndices);
		IndexBuffer.SetData(UpdateData->IndexBuffer.Data);
	}

	// Update index buffer
	if (!!(UpdateData->BuffersToUpdate & ERuntimeMeshBuffersToUpdate::AdjacencyIndexBuffer))
	{
		AdjacencyIndexBuffer.SetNum(UpdateData->AdjacencyIndexBuffer.NumIndices);
		AdjacencyIndexBuffer.SetData(UpdateData->AdjacencyIndexBuffer.Data);
	}

#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 19
	// If this platform uses manual vertex fetch, we need to update the SRVs
	if (RHISupportsManualVertexFetch(GMaxRHIShaderPlatform))
	{
		FLocalVertexFactory::FDataType DataType;
		BuildVertexDataType(DataType);

		VertexFactory.ReleaseResource();
		VertexFactory.Init(DataType);
		VertexFactory.InitResource();
	}
#endif
}

void FRuntimeMeshSectionProxy::FinishPropertyUpdate_RenderThread(FRuntimeMeshSectionPropertyUpdateParamsPtr UpdateData)
{
	// Copy visibility/shadow
	bIsVisible = UpdateData->bIsVisible;
	bCastsShadow = UpdateData->bCastsShadow;
}
