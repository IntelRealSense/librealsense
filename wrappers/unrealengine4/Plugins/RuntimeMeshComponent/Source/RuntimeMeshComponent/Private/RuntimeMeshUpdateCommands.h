// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "Engine.h"
#include "Components/MeshComponent.h"



struct FRuntimeMeshSectionVertexBufferParams
{
	TArray<uint8> Data;
	int32 NumVertices;
};
struct FRuntimeMeshSectionTangentVertexBufferParams : public FRuntimeMeshSectionVertexBufferParams
{
	bool bUsingHighPrecision;
};
struct FRuntimeMeshSectionUVVertexBufferParams : public FRuntimeMeshSectionVertexBufferParams
{
	bool bUsingHighPrecision;
	int32 NumUVs;
};

struct FRuntimeMeshSectionIndexBufferParams
{
	bool b32BitIndices;
	TArray<uint8> Data;
	int32 NumIndices;
};

struct FRuntimeMeshSectionCreationParams
{
	EUpdateFrequency UpdateFrequency;

	FRuntimeMeshSectionVertexBufferParams PositionVertexBuffer;
	FRuntimeMeshSectionTangentVertexBufferParams TangentsVertexBuffer;
	FRuntimeMeshSectionUVVertexBufferParams UVsVertexBuffer;
	FRuntimeMeshSectionVertexBufferParams ColorVertexBuffer;

	FRuntimeMeshSectionIndexBufferParams IndexBuffer;
	FRuntimeMeshSectionIndexBufferParams AdjacencyIndexBuffer;

	bool bIsVisible;
	bool bCastsShadow;
};
using FRuntimeMeshSectionCreationParamsPtr = TSharedPtr<FRuntimeMeshSectionCreationParams, ESPMode::NotThreadSafe>;

struct FRuntimeMeshSectionUpdateParams
{
	ERuntimeMeshBuffersToUpdate BuffersToUpdate;

	FRuntimeMeshSectionVertexBufferParams PositionVertexBuffer;
	FRuntimeMeshSectionTangentVertexBufferParams TangentsVertexBuffer;
	FRuntimeMeshSectionUVVertexBufferParams UVsVertexBuffer;
	FRuntimeMeshSectionVertexBufferParams ColorVertexBuffer;

	FRuntimeMeshSectionIndexBufferParams IndexBuffer;
	FRuntimeMeshSectionIndexBufferParams AdjacencyIndexBuffer;
};
using FRuntimeMeshSectionUpdateParamsPtr = TSharedPtr<FRuntimeMeshSectionUpdateParams, ESPMode::NotThreadSafe>;

struct FRuntimeMeshSectionPropertyUpdateParams
{
	bool bIsVisible;
	bool bCastsShadow;
};
using FRuntimeMeshSectionPropertyUpdateParamsPtr = TSharedPtr<FRuntimeMeshSectionPropertyUpdateParams, ESPMode::NotThreadSafe>;

