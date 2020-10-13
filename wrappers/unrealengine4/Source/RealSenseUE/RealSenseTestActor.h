#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RealSenseTestActor.generated.h"

UCLASS(ClassGroup="RealSense", BlueprintType)
class REALSENSEUE_API ARealSenseTestActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ARealSenseTestActor();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;
};
