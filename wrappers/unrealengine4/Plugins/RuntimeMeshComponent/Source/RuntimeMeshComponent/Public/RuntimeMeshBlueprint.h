// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshCore.h"
#include "RuntimeMeshBlueprint.generated.h"

/**
 * 
 */
USTRUCT(BlueprintType)
struct RUNTIMEMESHCOMPONENT_API FRuntimeMeshBlueprintVertexSimple
{
public:
	GENERATED_BODY()
	
	/** Vertex position */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Vertex)
	FVector Position;

	/** Vertex normal */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Vertex)
	FVector Normal;

	/** Vertex tangent */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Vertex)
	FRuntimeMeshTangent Tangent;

	/** Vertex color */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Vertex)
	FLinearColor Color;

	/** Vertex texture co-ordinate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Vertex)
	FVector2D UV0;


	FRuntimeMeshBlueprintVertexSimple()
		: Position(0.0f, 0.0f, 0.0f)
		, Normal(0.0f, 0.0f, 1.0f)
		, Tangent(FVector(1.0f, 0.0f, 0.0f), false)
		, Color(255, 255, 255)
		, UV0(0.0f, 0.0f)
	{
	}

	FRuntimeMeshBlueprintVertexSimple(const FVector& InPosition, const FVector& InNormal, const FRuntimeMeshTangent& InTangent, const FVector2D& InUV0, FLinearColor InColor = FLinearColor::White)
		: Position(InPosition)
		, Normal(InNormal)
		, Tangent(InTangent)
		, Color(InColor)
		, UV0(InUV0)
	{
	}
};
