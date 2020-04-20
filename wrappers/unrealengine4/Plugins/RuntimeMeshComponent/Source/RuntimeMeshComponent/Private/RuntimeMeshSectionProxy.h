// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "Engine/Engine.h"
#include "Components/MeshComponent.h"
#include "RuntimeMeshRendering.h"
#include "RuntimeMeshUpdateCommands.h"


struct FRuntimeMeshSectionNullBufferElement
{
	FPackedNormal Normal;
	FPackedNormal Tangent;
	FColor Color;
	FVector2DHalf UV0;

	FRuntimeMeshSectionNullBufferElement()
		: Normal(FVector(0.0f, 0.0f, 1.0f))
		, Tangent(FVector(1.0f, 0.0f, 0.0f))
		, Color(FColor::Transparent)
		, UV0(FVector2D::ZeroVector)
	{ }
};


class FRuntimeMeshSectionProxy : public TSharedFromThis<FRuntimeMeshSectionProxy, ESPMode::NotThreadSafe>
{
	/** Update frequency of this section */
	EUpdateFrequency UpdateFrequency;

	/** Vertex factory for this section */
	FRuntimeMeshVertexFactory VertexFactory;

	/** Vertex buffer containing the positions for this section */
	FRuntimeMeshPositionVertexBuffer PositionBuffer;

	/** Vertex buffer containing the tangents for this section */
	FRuntimeMeshTangentsVertexBuffer TangentsBuffer;

	/** Vertex buffer containing the UVs for this section */
	FRuntimeMeshUVsVertexBuffer UVsBuffer;

	/** Vertex buffer containing the colors for this section */
	FRuntimeMeshColorVertexBuffer ColorBuffer;

	/** Index buffer for this section */
	FRuntimeMeshIndexBuffer IndexBuffer;

	/** Index buffer for this section */
	FRuntimeMeshIndexBuffer AdjacencyIndexBuffer;

	/** Whether this section is currently visible */
	bool bIsVisible;

	/** Should this section cast a shadow */
	bool bCastsShadow;

public:
	FRuntimeMeshSectionProxy(ERHIFeatureLevel::Type InFeatureLevel, FRuntimeMeshSectionCreationParamsPtr CreationData);

	~FRuntimeMeshSectionProxy();

	bool ShouldRender();
	bool CanRender();

	bool WantsToRenderInStaticPath() const;

	bool CastsShadow() const;

	FRuntimeMeshVertexFactory* GetVertexFactory() { return &VertexFactory; }

	void CreateMeshBatch(FMeshBatch& MeshBatch, bool bWantsAdjacencyInfo);

	void BuildVertexDataType(FLocalVertexFactory::FDataType& DataType);

	void FinishUpdate_RenderThread(FRuntimeMeshSectionUpdateParamsPtr UpdateData);

	void FinishPropertyUpdate_RenderThread(FRuntimeMeshSectionPropertyUpdateParamsPtr UpdateData);
};


