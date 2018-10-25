// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshComponent.h"
#include "RuntimeMeshComponentPlugin.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "Physics/IPhysXCookingModule.h"
#include "RuntimeMeshCore.h"
#include "RuntimeMeshGenericVertex.h"
#include "RuntimeMeshUpdateCommands.h"
#include "RuntimeMeshSection.h"
#include "RuntimeMeshSectionProxy.h"
#include "RuntimeMesh.h"
#include "RuntimeMeshComponentProxy.h"
#include "RuntimeMeshLegacySerialization.h"




DECLARE_CYCLE_STAT(TEXT("RMC - New Collision Data Recieved"), STAT_RuntimeMeshComponent_NewCollisionMeshReceived, STATGROUP_RuntimeMesh);





URuntimeMeshComponent::URuntimeMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetNetAddressable();
}

void URuntimeMeshComponent::EnsureHasRuntimeMesh()
{
	if (RuntimeMeshReference == nullptr)
	{
		SetRuntimeMesh(NewObject<URuntimeMesh>(this));
	}
}

void URuntimeMeshComponent::SetRuntimeMesh(URuntimeMesh* NewMesh)
{
	// Unlink from any existing runtime mesh
	if (RuntimeMeshReference)
	{
		RuntimeMeshReference->UnRegisterLinkedComponent(this);
		RuntimeMeshReference = nullptr;
	}

	if (NewMesh)
	{
		RuntimeMeshReference = NewMesh;
		RuntimeMeshReference->RegisterLinkedComponent(this);
	}

	MarkRenderStateDirty();
}


void URuntimeMeshComponent::NewCollisionMeshReceived()
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMeshComponent_NewCollisionMeshReceived);

	// First recreate the physics state
	RecreatePhysicsState();
	
 	// Now update the navigation.
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 20
	FNavigationSystem::UpdateComponentData(*this);
#else
 	if (UNavigationSystem::ShouldUpdateNavOctreeOnComponentChange() && IsRegistered())
 	{
 		UWorld* MyWorld = GetWorld();
 
 		if (MyWorld != nullptr && MyWorld->GetNavigationSystem() != nullptr &&
 			(MyWorld->GetNavigationSystem()->ShouldAllowClientSideNavigation() || !MyWorld->IsNetMode(ENetMode::NM_Client)))
 		{
 			UNavigationSystem::UpdateComponentInNavOctree(*this);
 		}
 	}
#endif
}

void URuntimeMeshComponent::NewBoundsReceived()
{
	UpdateBounds();
	MarkRenderTransformDirty();
}

void URuntimeMeshComponent::ForceProxyRecreate()
{
	MarkRenderStateDirty();
}





void URuntimeMeshComponent::SendSectionCreation(int32 SectionIndex)
{
	MarkRenderStateDirty();
}

void URuntimeMeshComponent::SendSectionPropertiesUpdate(int32 SectionIndex)
{
	MarkRenderStateDirty();
}

FBoxSphereBounds URuntimeMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	if (GetRuntimeMesh())
	{
		return GetRuntimeMesh()->GetLocalBounds().TransformBy(LocalToWorld);
	}

	return FBoxSphereBounds(FSphere(FVector::ZeroVector, 1));
}


FPrimitiveSceneProxy* URuntimeMeshComponent::CreateSceneProxy()
{
	return RuntimeMeshReference != nullptr ? new FRuntimeMeshComponentSceneProxy(this) : nullptr;
}

UBodySetup* URuntimeMeshComponent::GetBodySetup()
{
	if (GetRuntimeMesh())
	{
		return GetRuntimeMesh()->BodySetup;
	}
	
	return nullptr;
}


int32 URuntimeMeshComponent::GetNumMaterials() const
{
	int32 RuntimeMeshSections = GetRuntimeMesh() != nullptr ? GetRuntimeMesh()->GetNumSections() : 0;

	return FMath::Max(Super::GetNumMaterials(), RuntimeMeshSections);
}

void URuntimeMeshComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const
{
	if (URuntimeMesh* Mesh = GetRuntimeMesh())
	{
		Mesh->GetUsedMaterials(OutMaterials);
	}

	Super::GetUsedMaterials(OutMaterials, bGetDebugMaterials);
}

UMaterialInterface* URuntimeMeshComponent::GetMaterial(int32 ElementIndex) const
{
	UMaterialInterface* Mat = Super::GetMaterial(ElementIndex);

	// Use default override material system
	if (Mat != nullptr)
		return Mat;

	// fallback to RM sections material
	if (URuntimeMesh* Mesh = GetRuntimeMesh())
	{
		return Mesh->GetSectionMaterial(ElementIndex);
	}

	// Had no RM/Section return null
	return nullptr;
}

UMaterialInterface* URuntimeMeshComponent::GetMaterialFromCollisionFaceIndex(int32 FaceIndex, int32& SectionIndex) const
{
	UMaterialInterface* Result = nullptr;
	SectionIndex = 0;

	if (URuntimeMesh* Mesh = GetRuntimeMesh())
	{
		Result = Mesh->GetMaterialFromCollisionFaceIndex(FaceIndex, SectionIndex);
	}

	return Result;
}



void URuntimeMeshComponent::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	// This is a no-op serialization handler that can read all the old data
	// but does nothing with it
	if (FRuntimeMeshComponentLegacySerialization::Serialize(Ar))
	{
		//// If this was an old file we attempt to recover by just rerunning the construction script of the owning actor.
		//if (AActor* Actor = GetOwner())
		//{
		//	if (!Actor->HasAnyFlags(RF_ClassDefaultObject))
		//	{
		//		Actor->RerunConstructionScripts();
		//	}
		//}
	}
}



void URuntimeMeshComponent::PostLoad()
{
	Super::PostLoad();

	if (RuntimeMeshReference)
	{
		RuntimeMeshReference->RegisterLinkedComponent(this);
	}
}