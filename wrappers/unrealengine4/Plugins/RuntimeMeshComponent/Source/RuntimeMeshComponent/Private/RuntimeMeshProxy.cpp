// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshProxy.h"
#include "RuntimeMeshComponentPlugin.h"
#include "RuntimeMesh.h"

FRuntimeMeshProxy::FRuntimeMeshProxy(ERHIFeatureLevel::Type InFeatureLevel)
	: FeatureLevel(InFeatureLevel)
{
}

FRuntimeMeshProxy::~FRuntimeMeshProxy()
{
	// The mesh proxy can only be safely destroyed from the rendering thread.
	// This is so that all the resources can be safely freed correctly.
	check(IsInRenderingThread());
}

void FRuntimeMeshProxy::CreateSection_GameThread(int32 SectionId, const FRuntimeMeshSectionCreationParamsPtr& SectionData)
{
	// HORU: 4.22 rendering
	ENQUEUE_RENDER_COMMAND(FRuntimeMeshProxyCreateSection)(
		[this, SectionId, SectionData](FRHICommandListImmediate & RHICmdList)
		{
			CreateSection_RenderThread(SectionId, SectionData);
		}
	);
}

void FRuntimeMeshProxy::CreateSection_RenderThread(int32 SectionId, const FRuntimeMeshSectionCreationParamsPtr& SectionData)
{
	check(IsInRenderingThread());
	check(SectionData.IsValid());

	FRuntimeMeshSectionProxyPtr NewSection = MakeShareable(new FRuntimeMeshSectionProxy(FeatureLevel, SectionData),
		FRuntimeMeshRenderThreadDeleter<FRuntimeMeshSectionProxy>());

	// Add the section to the map, destroying any one that existed
	Sections.FindOrAdd(SectionId) = NewSection;

	// Update the cached values for rendering.
	UpdateCachedValues();
}

void FRuntimeMeshProxy::UpdateSection_GameThread(int32 SectionId, const FRuntimeMeshSectionUpdateParamsPtr& SectionData)
{
	// HORU: 4.22 rendering
	ENQUEUE_RENDER_COMMAND(FRuntimeMeshProxyUpdateSection)(
		[this, SectionId, SectionData](FRHICommandListImmediate & RHICmdList)
		{
			UpdateSection_RenderThread(SectionId, SectionData);
		}
	);
}

void FRuntimeMeshProxy::UpdateSection_RenderThread(int32 SectionId, const FRuntimeMeshSectionUpdateParamsPtr& SectionData)
{
	check(IsInRenderingThread());
	check(SectionData.IsValid());

	if (Sections.Contains(SectionId))
	{
		FRuntimeMeshSectionProxyPtr Section = Sections[SectionId];
		Section->FinishUpdate_RenderThread(SectionData);

		// Update the cached values for rendering.
		UpdateCachedValues();
	}
}

void FRuntimeMeshProxy::UpdateSectionProperties_GameThread(int32 SectionId, const FRuntimeMeshSectionPropertyUpdateParamsPtr& SectionData)
{
	// HORU: 4.22 rendering
	ENQUEUE_RENDER_COMMAND(FRuntimeMeshProxyUpdateSectionProperties)(
		[this, SectionId, SectionData](FRHICommandListImmediate & RHICmdList)
		{
			UpdateSectionProperties_RenderThread(SectionId, SectionData);
		}
	);
}

void FRuntimeMeshProxy::UpdateSectionProperties_RenderThread(int32 SectionId, const FRuntimeMeshSectionPropertyUpdateParamsPtr& SectionData)
{
	check(IsInRenderingThread());
	check(SectionData.IsValid());

	if (Sections.Contains(SectionId))
	{
		FRuntimeMeshSectionProxyPtr Section = Sections[SectionId];
		Section->FinishPropertyUpdate_RenderThread(SectionData);

		// Update the cached values for rendering.
		UpdateCachedValues();
	}
}

void FRuntimeMeshProxy::DeleteSection_GameThread(int32 SectionId)
{
	// HORU: 4.22 rendering
	ENQUEUE_RENDER_COMMAND(FRuntimeMeshProxyDeleteSection)(
		[this, SectionId](FRHICommandListImmediate & RHICmdList)
		{
			DeleteSection_RenderThread(SectionId);
		}
	);
}

void FRuntimeMeshProxy::DeleteSection_RenderThread(int32 SectionId)
{
	check(IsInRenderingThread());

	bool bChangedState = false;
	if (SectionId == INDEX_NONE)
	{
		Sections.Empty();
		bChangedState = true;
	}
	else
	{
		// Remove the section if it exists
		if (Sections.Remove(SectionId) > 0)
		{
			bChangedState = true;
		}
	}

	if (bChangedState)
	{
		// Update the cached values for rendering.
		UpdateCachedValues();
	}
}


void FRuntimeMeshProxy::UpdateCachedValues()
{
	check(IsInRenderingThread());
}
