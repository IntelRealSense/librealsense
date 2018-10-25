// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshActor.h"
#include "RuntimeMeshComponent.h"
#include "RuntimeMeshComponentPlugin.h"


ARuntimeMeshActor::ARuntimeMeshActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bRunGenerateMeshesOnConstruction(true)
	, bRunGenerateMeshesOnBeginPlay(false)
{
	bCanBeDamaged = false;

	RuntimeMeshComponent = CreateDefaultSubobject<URuntimeMeshComponent>(TEXT("RuntimeMeshComponent0"));
	RuntimeMeshComponent->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	RuntimeMeshComponent->Mobility = EComponentMobility::Static;
	RuntimeMeshComponent->bGenerateOverlapEvents = false;

	RootComponent = RuntimeMeshComponent;
}

ERuntimeMeshMobility ARuntimeMeshActor::GetRuntimeMeshMobility()
{
	if (RuntimeMeshComponent)
	{
		return RuntimeMeshComponent->GetRuntimeMeshMobility();
	}
	return ERuntimeMeshMobility::Static;
}

void ARuntimeMeshActor::SetRuntimeMeshMobility(ERuntimeMeshMobility NewMobility)
{
	if (RuntimeMeshComponent)
	{
		RuntimeMeshComponent->SetRuntimeMeshMobility(NewMobility);
	}
}

void ARuntimeMeshActor::SetMobility(EComponentMobility::Type InMobility)
{
	if (RuntimeMeshComponent)
	{
		RuntimeMeshComponent->SetMobility(InMobility);
	}
}

EComponentMobility::Type ARuntimeMeshActor::GetMobility()
{
	if (RuntimeMeshComponent)
	{
		return RuntimeMeshComponent->Mobility;
	}
	return EComponentMobility::Static;
}

void ARuntimeMeshActor::OnConstruction(const FTransform& Transform)
{
	if (bRunGenerateMeshesOnConstruction)
	{
		GenerateMeshes();
	}
}

void ARuntimeMeshActor::BeginPlay()
{
	bool bIsGameWorld = GetWorld() && GetWorld()->IsGameWorld() && !GetWorld()->IsPreviewWorld() && !GetWorld()->IsEditorWorld();

	bool bHadSerializedMeshData = false;
	if (RuntimeMeshComponent)
	{
		URuntimeMesh* Mesh = RuntimeMeshComponent->GetRuntimeMesh();
		if (Mesh)
		{
			bHadSerializedMeshData = Mesh->HasSerializedMeshData();
		}
	}

	if ((bIsGameWorld && !bHadSerializedMeshData) || bRunGenerateMeshesOnBeginPlay)
	{
		GenerateMeshes();
	}
}

void ARuntimeMeshActor::GenerateMeshes_Implementation()
{

}

