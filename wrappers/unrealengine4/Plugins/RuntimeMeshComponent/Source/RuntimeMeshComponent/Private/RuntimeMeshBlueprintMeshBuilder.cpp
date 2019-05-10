// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshBlueprintMeshBuilder.h"





URuntimeBlueprintMeshBuilder* URuntimeMeshBuilderFunctions::MakeRuntimeMeshBuilder(bool bWantsHighPrecisionTangents /*= false*/, bool bWantsHighPrecisionUVs /*= false*/, int32 NumUVs /*= 1*/, bool bUse16BitIndices /*= false*/)
{
	URuntimeBlueprintMeshBuilder* Builder = NewObject<URuntimeBlueprintMeshBuilder>();
	Builder->MeshBuilder = ::MakeRuntimeMeshBuilder(bWantsHighPrecisionTangents, bWantsHighPrecisionUVs, NumUVs, !bUse16BitIndices);
	Builder->MeshAccessor = Builder->MeshBuilder;
	return Builder;
}
