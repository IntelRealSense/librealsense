// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMesh.h"
#include "RuntimeMeshComponentPlugin.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "Physics/IPhysXCookingModule.h"
#include "RuntimeMeshComponent.h"
#include "RuntimeMeshProxy.h"
#include "RuntimeMeshBuilder.h"
#include "RuntimeMeshLibrary.h"

DECLARE_CYCLE_STAT(TEXT("RM - Collision Update"), STAT_RuntimeMesh_CollisionUpdate, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Async Collision Cook Finish"), STAT_RuntimeMesh_AsyncCollisionFinish, STATGROUP_RuntimeMesh);
DECLARE_CYCLE_STAT(TEXT("RM - Collision Finalize"), STAT_RuntimeMesh_CollisionFinalize, STATGROUP_RuntimeMesh);




//////////////////////////////////////////////////////////////////////////
//	FRuntimeMeshCollisionCookTickObject
void FRuntimeMeshCollisionCookTickObject::Tick(float DeltaTime)
{
	URuntimeMesh* Mesh = Owner.Get();
	if (Mesh && Mesh->bCollisionIsDirty)
	{
		Mesh->UpdateCollision();
		Mesh->bCollisionIsDirty = false;
	}
}

bool FRuntimeMeshCollisionCookTickObject::IsTickable() const
{
	URuntimeMesh* Mesh = Owner.Get();
	if (Mesh)
	{
		return Mesh->bCollisionIsDirty;
	}
	return false;
}

TStatId FRuntimeMeshCollisionCookTickObject::GetStatId() const
{
	return TStatId();
}


UWorld* FRuntimeMeshCollisionCookTickObject::GetTickableGameObjectWorld() const
{
	URuntimeMesh* Mesh = Owner.Get();
	if (Mesh)
	{
		return Mesh->GetWorld();
	}
	return nullptr;
}

//////////////////////////////////////////////////////////////////////////
//	URuntimeMesh


URuntimeMesh::URuntimeMesh(const FObjectInitializer& ObjectInitializer)
	: UObject(ObjectInitializer)
	, Data(new FRuntimeMeshData())
	, bCollisionIsDirty(false)
	, bUseComplexAsSimpleCollision(true)
	, bUseAsyncCooking(false)
	, bShouldSerializeMeshData(true)
	, CollisionMode(ERuntimeMeshCollisionCookingMode::CookingPerformance)
	, BodySetup(nullptr)
{
	Data->Setup(TWeakObjectPtr<URuntimeMesh>(this));
}

void URuntimeMesh::CookCollisionNow()
{
	check(IsInGameThread());

	if (bCollisionIsDirty)
	{
		UpdateCollision(true);
		bCollisionIsDirty = false;
	}
}
void URuntimeMesh::RegisterLinkedComponent(URuntimeMeshComponent* NewComponent)
{
	LinkedComponents.AddUnique(NewComponent);
}

void URuntimeMesh::UnRegisterLinkedComponent(URuntimeMeshComponent* ComponentToRemove)
{
	check(LinkedComponents.Contains(ComponentToRemove));

	LinkedComponents.RemoveSingleSwap(ComponentToRemove, true);
}



bool URuntimeMesh::GetPhysicsTriMeshData(struct FTriMeshCollisionData* CollisionData, bool InUseAllTriData)
{
	CollisionData->bFastCook = CollisionMode == ERuntimeMeshCollisionCookingMode::CookingPerformance;

	return GetRuntimeMeshData()->GetPhysicsTriMeshData(CollisionData, InUseAllTriData);
}

bool URuntimeMesh::ContainsPhysicsTriMeshData(bool InUseAllTriData) const
{
	return GetRuntimeMeshData()->ContainsPhysicsTriMeshData(InUseAllTriData);
}



void URuntimeMesh::Serialize(FArchive& Ar)
{
	Super::Serialize(Ar);

	// Serialize the entire data object.
	Ar.UsingCustomVersion(FRuntimeMeshVersion::GUID);

	if (Ar.CustomVer(FRuntimeMeshVersion::GUID) < FRuntimeMeshVersion::SerializationUpgradeToConfigurable || bShouldSerializeMeshData)
	{
		Ar << Data.Get();
	}
}

void URuntimeMesh::PostLoad()
{
	Super::PostLoad();

	UpdateLocalBounds();
	MarkCollisionDirty();
}

UMaterialInterface* URuntimeMesh::GetMaterialFromCollisionFaceIndex(int32 FaceIndex, int32& SectionIndex) const
{
	SectionIndex = GetRuntimeMeshData()->GetSectionFromCollisionFaceIndex(FaceIndex);

	if (Materials.IsValidIndex(SectionIndex))
	{
		return Materials[SectionIndex];
	}
	return nullptr;
}

int32 URuntimeMesh::GetSectionIdFromCollisionFaceIndex(int32 FaceIndex) const
{
	return GetRuntimeMeshData()->GetSectionFromCollisionFaceIndex(FaceIndex);
}

void URuntimeMesh::MarkCollisionDirty()
{
	// Flag the collision as dirty
	bCollisionIsDirty = true;
	
	if (!CookTickObject.IsValid())
	{
		CookTickObject = MakeUnique<FRuntimeMeshCollisionCookTickObject>(TWeakObjectPtr<URuntimeMesh>(this));
	}
}

#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 21
UBodySetup* URuntimeMesh::CreateNewBodySetup()
{
	UBodySetup* NewBodySetup = NewObject<UBodySetup>(this, NAME_None, (IsTemplate() ? RF_Public : RF_NoFlags));
	NewBodySetup->BodySetupGuid = FGuid::NewGuid();

	return NewBodySetup;
}
#endif

void URuntimeMesh::CopyCollisionElementsToBodySetup(UBodySetup* Setup)
{
	GetRuntimeMeshData()->CopyCollisionElementsToBodySetup(Setup);
}

void URuntimeMesh::SetBasicBodySetupParameters(UBodySetup* Setup)
{
	Setup->BodySetupGuid = FGuid::NewGuid();

	Setup->bGenerateMirroredCollision = false;
	Setup->bDoubleSidedGeometry = true;
	Setup->CollisionTraceFlag = bUseComplexAsSimpleCollision ? CTF_UseComplexAsSimple : CTF_UseDefault;
}

void URuntimeMesh::UpdateCollision(bool bForceCookNow)
{
#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION < 21
	DoForAllLinkedComponents([bForceCookNow](URuntimeMeshComponent* Mesh)
	{
		Mesh->UpdateCollision(bForceCookNow);
	});

#else
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_CollisionUpdate);
	check(IsInGameThread());
	
	UWorld* World = GetWorld();
	const bool bShouldCookAsync = !bForceCookNow && World && World->IsGameWorld() && bUseAsyncCooking;

	if (bShouldCookAsync)
	{
		// Abort all previous ones still standing
		for (UBodySetup* OldBody : AsyncBodySetupQueue)
		{
			OldBody->AbortPhysicsMeshAsyncCreation();
		}

		UBodySetup* NewBodySetup = CreateNewBodySetup();
		AsyncBodySetupQueue.Add(NewBodySetup);

		SetBasicBodySetupParameters(NewBodySetup);
		CopyCollisionElementsToBodySetup(NewBodySetup);

		NewBodySetup->CreatePhysicsMeshesAsync(
			FOnAsyncPhysicsCookFinished::CreateUObject(this, &URuntimeMesh::FinishPhysicsAsyncCook, NewBodySetup));
	}
	else
	{
		AsyncBodySetupQueue.Empty();
		UBodySetup* NewBodySetup = CreateNewBodySetup();

		// Change body setup guid 
		NewBodySetup->BodySetupGuid = FGuid::NewGuid();

		SetBasicBodySetupParameters(NewBodySetup);
		CopyCollisionElementsToBodySetup(NewBodySetup);

		// Update meshes
		NewBodySetup->bHasCookedCollisionData = true;
		NewBodySetup->InvalidatePhysicsData();
		NewBodySetup->CreatePhysicsMeshes();

		BodySetup = NewBodySetup;
		FinalizeNewCookedData();
	}
#endif
}

#if ENGINE_MAJOR_VERSION >= 4 && ENGINE_MINOR_VERSION >= 21
void URuntimeMesh::FinishPhysicsAsyncCook(bool bSuccess, UBodySetup* FinishedBodySetup)
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_AsyncCollisionFinish);
	check(IsInGameThread());

	int32 FoundIdx;
	if (AsyncBodySetupQueue.Find(FinishedBodySetup, FoundIdx))
	{
		if (bSuccess)
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

			FinalizeNewCookedData();

		}
		else
		{
			AsyncBodySetupQueue.RemoveAt(FoundIdx);
		}
	}
}

void URuntimeMesh::FinalizeNewCookedData()
{
	SCOPE_CYCLE_COUNTER(STAT_RuntimeMesh_CollisionFinalize);
	check(IsInGameThread());
	
	// Alert all linked components so they can update their physics state.
	DoForAllLinkedComponents([](URuntimeMeshComponent* Mesh)
	{
		Mesh->NewCollisionMeshReceived();
	});

	// Call user event to notify of collision updated.
	if (CollisionUpdated.IsBound())
	{
		CollisionUpdated.Broadcast();
	}
}
#endif

void URuntimeMesh::UpdateLocalBounds()
{
	DoForAllLinkedComponents([](URuntimeMeshComponent* Mesh)
	{
		Mesh->NewBoundsReceived();
	});
}

void URuntimeMesh::ForceProxyRecreate()
{
	DoForAllLinkedComponents([](URuntimeMeshComponent* Mesh)
	{
		Mesh->ForceProxyRecreate();
	});
}

void URuntimeMesh::SendSectionCreation(int32 SectionIndex)
{
	DoForAllLinkedComponents([SectionIndex](URuntimeMeshComponent* Mesh)
	{
		Mesh->SendSectionCreation(SectionIndex);
	});
}

void URuntimeMesh::SendSectionPropertiesUpdate(int32 SectionIndex)
{
	DoForAllLinkedComponents([SectionIndex](URuntimeMeshComponent* Mesh)
	{
		Mesh->SendSectionPropertiesUpdate(SectionIndex);
	});
}


