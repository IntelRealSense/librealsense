// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RuntimeMeshActor.generated.h"

UCLASS(HideCategories = (Input), ShowCategories = ("Input|MouseInput", "Input|TouchInput"), ComponentWrapperClass, Meta = (ChildCanTick))
class RUNTIMEMESHCOMPONENT_API ARuntimeMeshActor : public AActor
{
	GENERATED_UCLASS_BODY()

private:
	UPROPERTY(Category = "RuntimeMeshActor", VisibleAnywhere, BlueprintReadOnly, Meta = (ExposeFunctionCategories = "Mesh,Rendering,Physics,Components|RuntimeMesh", AllowPrivateAccess = "true"))
	class URuntimeMeshComponent* RuntimeMeshComponent;

public:
	UPROPERTY(Category = "RuntimeMeshActor", EditAnywhere, Meta = (AllowPrivateAccess = "true"))
	bool bRunGenerateMeshesOnConstruction;

	UPROPERTY(Category = "RuntimeMeshActor", EditAnywhere, Meta = (AllowPrivateAccess = "true"))
	bool bRunGenerateMeshesOnBeginPlay;

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshActor", Meta = (AllowPrivateAccess = "true", DisplayName = "Get Mobility"))
	ERuntimeMeshMobility GetRuntimeMeshMobility();

	UFUNCTION(BlueprintCallable, Category = "RuntimeMeshActor", Meta = (AllowPrivateAccess = "true", DisplayName = "Set Mobility"))
	void SetRuntimeMeshMobility(ERuntimeMeshMobility NewMobility);

public:

	/** Function to change mobility type */
	void SetMobility(EComponentMobility::Type InMobility);

	EComponentMobility::Type GetMobility();

	/** Returns RuntimeMeshComponent subobject **/
	class URuntimeMeshComponent* GetRuntimeMeshComponent() const { return RuntimeMeshComponent; }


	virtual void OnConstruction(const FTransform& Transform) override;

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RuntimeMeshActor")
	void GenerateMeshes();

};
