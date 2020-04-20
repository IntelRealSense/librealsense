// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshComponent.h"
#include "RuntimeMeshComponentPlugin.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "IPhysXCookingModule.h"
#include "RuntimeMeshCore.h"
#include "RuntimeMeshGenericVertex.h"
#include "RuntimeMeshUpdateCommands.h"
#include "RuntimeMeshSection.h"
#include "RuntimeMeshSectionProxy.h"
#include "RuntimeMesh.h"
#include "RuntimeMeshComponentProxy.h"
#include "RuntimeMeshLegacySerialization.h"
#include "NavigationSystem.h"



DECLARE_CYCLE_STAT(TEXT("RMC - New Collision Data Recieved"), STAT_RuntimeMeshComponent_NewCollisionMeshReceived, STATGROUP_RuntimeMesh);





URuntimeMeshComponent::URuntimeMeshComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 21
	, BodySetup(nullptr)
#endif

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
 	if (UNavigationSystemV1::ShouldUpdateNavOctreeOnComponentChange() && IsRegistered())
 	{
 		UWorld* MyWorld = GetWorld();
 
 		if (MyWorld != nullptr && FNavigationSystem::GetCurrent(MyWorld) != nullptr &&
 			(FNavigationSystem::GetCurrent(MyWorld)->ShouldAllowClientSideNavigation() || !MyWorld->IsNetMode(ENetMode::NM_Client)))
 		{
			UNavigationSystemV1::UpdateComponentInNavOctree(*this);
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
#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 21
	return BodySetup;
#else
	if (GetRuntimeMesh())
	{
		return GetRuntimeMesh()->BodySetup;
	}
	
	return nullptr;
#endif
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

UMaterialInterface* URuntimeMeshComponent::GetOverrideMaterial(int32 ElementIndex) const
{
	return Super::GetMaterial(ElementIndex);
}

int32 URuntimeMeshComponent::GetSectionIdFromCollisionFaceIndex(int32 FaceIndex) const
{
	int32 SectionIndex = 0;

	if (URuntimeMesh* Mesh = GetRuntimeMesh())
	{
		SectionIndex = Mesh->GetSectionIdFromCollisionFaceIndex(FaceIndex);
	}

	return SectionIndex;
}

void URuntimeMeshComponent::GetSectionIdAndFaceIdFromCollisionFaceIndex(int32 FaceIndex, int32 & SectionIndex, int32 & SectionFaceIndex) const
{
	if (URuntimeMesh* Mesh = GetRuntimeMesh())
	{
		Mesh->GetSectionIdAndFaceIdFromCollisionFaceIndex(FaceIndex, SectionIndex, SectionFaceIndex);
	}
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


#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 22

bool URuntimeMeshComponent::GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData)
{
	URuntimeMesh* RuntimeMesh = GetRuntimeMesh();
	if (RuntimeMesh)
	{
		return RuntimeMesh->GetPhysicsTriMeshData(CollisionData, InUseAllTriData);
	}

	return false;
}

bool URuntimeMeshComponent::ContainsPhysicsTriMeshData(bool InUseAllTriData) const
{
	URuntimeMesh* RuntimeMesh = GetRuntimeMesh();
	if (RuntimeMesh)
	{
		return RuntimeMesh->ContainsPhysicsTriMeshData(InUseAllTriData);
	}
	return false;
}

#endif

#if ENGINE_MAJOR_VERSION == 4 && ENGINE_MINOR_VERSION < 21

UBodySetup* URuntimeMeshComponent::CreateNewBodySetup()
{
	UBodySetup* NewBodySetup = NewObject<UBodySetup>(this, NAME_None, (IsTemplate() ? RF_Public : RF_NoFlags));
	NewBodySetup->BodySetupGuid = FGuid::NewGuid();

	return NewBodySetup;
}

void URuntimeMeshComponent::FinishPhysicsAsyncCook(UBodySetup* FinishedBodySetup)
{
	//SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_AsyncCollisionFinish);
	check(IsInGameThread());

	int32 FoundIdx;
	if (AsyncBodySetupQueue.Find(FinishedBodySetup, FoundIdx))
	{
		// The new body was found in the array meaning it's newer so use it
		BodySetup = FinishedBodySetup;

		// Shift down all remaining body setups, removing any old setups
		for (int32 Index = FoundIdx + 1; Index < AsyncBodySetupQueue.Num(); Index++)
		{
			AsyncBodySetupQueue[Index - (FoundIdx + 1)] = AsyncBodySetupQueue[Index];
			AsyncBodySetupQueue[Index] = nullptr;
		}
		AsyncBodySetupQueue.SetNum(AsyncBodySetupQueue.Num() - (FoundIdx + 1));
		
		NewCollisionMeshReceived();
	}
}

void URuntimeMeshComponent::UpdateCollision(bool bForceCookNow)
{
	//SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_CollisionUpdate);
	check(IsInGameThread());
	// HORU: workaround for a nullpointer
	if (!GetRuntimeMesh())
		return;
	//check(GetRuntimeMesh());

	UWorld* World = GetWorld();
	const bool bShouldCookAsync = !bForceCookNow && World && World->IsGameWorld() && GetRuntimeMesh()->bUseAsyncCooking;

	if (bShouldCookAsync)
	{
		UBodySetup* NewBodySetup = CreateNewBodySetup();
		AsyncBodySetupQueue.Add(NewBodySetup);

		GetRuntimeMesh()->SetBasicBodySetupParameters(NewBodySetup);
		GetRuntimeMesh()->CopyCollisionElementsToBodySetup(NewBodySetup);

		NewBodySetup->CreatePhysicsMeshesAsync(
			FOnAsyncPhysicsCookFinished::CreateUObject(this, &URuntimeMeshComponent::FinishPhysicsAsyncCook, NewBodySetup));
	}
	else
	{
		AsyncBodySetupQueue.Empty();
		UBodySetup* NewBodySetup = CreateNewBodySetup();

		// Change body setup guid 
		NewBodySetup->BodySetupGuid = FGuid::NewGuid();

		GetRuntimeMesh()->SetBasicBodySetupParameters(NewBodySetup);
		GetRuntimeMesh()->CopyCollisionElementsToBodySetup(NewBodySetup);

		// Update meshes
		NewBodySetup->bHasCookedCollisionData = true;
		NewBodySetup->InvalidatePhysicsData();
		NewBodySetup->CreatePhysicsMeshes();

		BodySetup = NewBodySetup;
		NewCollisionMeshReceived();
	}
}

#endif

bool URuntimeMeshComponent::IsAsyncCollisionCookingPending() const
{
	return AsyncBodySetupQueue.Num() != 0;
}
