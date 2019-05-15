// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshCore.h"
#include "RuntimeMeshBuilder.h"
#include "RuntimeMeshBlueprintMeshBuilder.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Abstract)
class RUNTIMEMESHCOMPONENT_API URuntimeBlueprintMeshAccessor : public UObject
{
	GENERATED_BODY()

protected:

	TSharedPtr<FRuntimeMeshAccessor> MeshAccessor;

public:

	
	
	friend class URuntimeMeshBuilderFunctions;
};

UCLASS(BlueprintType)
class RUNTIMEMESHCOMPONENT_API URuntimeBlueprintMeshBuilder : public URuntimeBlueprintMeshAccessor
{
	GENERATED_BODY()

	TSharedPtr<FRuntimeMeshBuilder> MeshBuilder;
	friend class URuntimeMeshBuilderFunctions;
	
public:
	TSharedPtr<FRuntimeMeshBuilder> GetMeshBuilder() { return MeshBuilder; }


	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMeshBuilder")
	bool IsUsingHighPrecisionTangents(URuntimeBlueprintMeshBuilder*& OutMeshBuilder)
	{
		OutMeshBuilder = this;
		return MeshAccessor->IsUsingHighPrecisionTangents();
	}
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMeshBuilder")
	bool IsUsingHighPrecisionUVs(URuntimeBlueprintMeshBuilder*& OutMeshBuilder)
	{
		OutMeshBuilder = this;
		return MeshAccessor->IsUsingHighPrecisionUVs();
	}
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMeshBuilder")
	bool IsUsing32BitIndices(URuntimeBlueprintMeshBuilder*& OutMeshBuilder)
	{
		OutMeshBuilder = this;
		return MeshAccessor->IsUsing32BitIndices();
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMeshBuilder")
	bool IsReadonly(URuntimeBlueprintMeshBuilder*& OutMeshBuilder)
	{
		OutMeshBuilder = this;
		return MeshAccessor->IsReadonly();
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMeshBuilder")
	int32 NumVertices(URuntimeBlueprintMeshBuilder*& OutMeshBuilder)
	{
		OutMeshBuilder = this;
		return MeshAccessor->NumVertices();
	}
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMeshBuilder")
	int32 NumUVChannels(URuntimeBlueprintMeshBuilder*& OutMeshBuilder)
	{
		OutMeshBuilder = this;
		return MeshAccessor->NumUVChannels();
	}
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMeshBuilder")
	int32 NumIndices(URuntimeBlueprintMeshBuilder*& OutMeshBuilder)
	{
		OutMeshBuilder = this;
		return MeshAccessor->NumIndices();
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMeshBuilder")
	void EmptyVertices(URuntimeBlueprintMeshBuilder*& OutMeshBuilder, int32 Slack = 0)
	{
		OutMeshBuilder = this;
		MeshAccessor->EmptyVertices(Slack);
	}
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMeshBuilder")
	void SetNumVertices(URuntimeBlueprintMeshBuilder*& OutMeshBuilder, int32 NewNum)
	{
		OutMeshBuilder = this;
		MeshAccessor->SetNumVertices(NewNum);
	}
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMeshBuilder")
	void EmptyIndices(URuntimeBlueprintMeshBuilder*& OutMeshBuilder, int32 Slack = 0)
	{
		OutMeshBuilder = this;
		MeshAccessor->EmptyIndices(Slack);
	}
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMeshBuilder")
	void SetNumIndices(URuntimeBlueprintMeshBuilder*& OutMeshBuilder, int32 NewNum)
	{
		OutMeshBuilder = this;
		MeshAccessor->SetNumIndices(NewNum);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMeshBuilder")
	int32 AddVertex(URuntimeBlueprintMeshBuilder*& OutMeshBuilder, FVector InPosition, FVector Normal, FRuntimeMeshTangent Tangent, FVector2D UV0, FLinearColor Color)
	{
		OutMeshBuilder = this;
		int32 Index = MeshAccessor->AddVertex(InPosition);
		MeshAccessor->SetNormalTangent(Index, Normal, Tangent);
		MeshAccessor->SetUV(Index, UV0);
		MeshAccessor->SetColor(Index, Color.ToFColor(false));
		return Index;
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMeshBuilder")
	FVector GetPosition(URuntimeBlueprintMeshBuilder*& OutMeshBuilder, int32 Index)
	{
		OutMeshBuilder = this;
		return MeshAccessor->GetPosition(Index);
	}
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMeshBuilder")
	FVector4 GetNormal(URuntimeBlueprintMeshBuilder*& OutMeshBuilder, int32 Index)
	{
		OutMeshBuilder = this;
		return MeshAccessor->GetNormal(Index);
	}
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMeshBuilder")
	FVector GetTangent(URuntimeBlueprintMeshBuilder*& OutMeshBuilder, int32 Index)
	{
		OutMeshBuilder = this;
		return MeshAccessor->GetTangent(Index);
	}
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMeshBuilder")
	FLinearColor GetColor(URuntimeBlueprintMeshBuilder*& OutMeshBuilder, int32 Index)
	{
		OutMeshBuilder = this;
		return FLinearColor(MeshAccessor->GetColor(Index));
	}
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMeshBuilder")
	FVector2D GetUV(URuntimeBlueprintMeshBuilder*& OutMeshBuilder, int32 Index, int32 Channel = 0)
	{
		OutMeshBuilder = this;
		return MeshAccessor->GetUV(Index, Channel);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMeshBuilder")
	int32 SetVertex(URuntimeBlueprintMeshBuilder*& OutMeshBuilder, int32 Index, FVector InPosition, FVector Normal, FRuntimeMeshTangent Tangent, FVector2D UV0, FLinearColor Color)
	{
		OutMeshBuilder = this;
		MeshAccessor->SetPosition(Index, InPosition);
		MeshAccessor->SetNormalTangent(Index, Normal, Tangent);
		MeshAccessor->SetUV(Index, UV0);
		MeshAccessor->SetColor(Index, Color.ToFColor(false));
		return Index;
	}


	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMeshBuilder")
	int32 SetPosition(URuntimeBlueprintMeshBuilder*& OutMeshBuilder, int32 Index, FVector Value)
	{
		OutMeshBuilder = this;
		MeshAccessor->SetPosition(Index, Value);
		return Index;
	}
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMeshBuilder")
	int32 SetNormal(URuntimeBlueprintMeshBuilder*& OutMeshBuilder, int32 Index, const FVector4& Value)
	{
		OutMeshBuilder = this;
		MeshAccessor->SetNormal(Index, Value);
		return Index;
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMeshBuilder")
	int32 SetTangent(URuntimeBlueprintMeshBuilder*& OutMeshBuilder, int32 Index, FRuntimeMeshTangent Value)
	{
		OutMeshBuilder = this;
		MeshAccessor->SetTangent(Index, Value);
		return Index;
	}
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMeshBuilder")
	int32 SetColor(URuntimeBlueprintMeshBuilder*& OutMeshBuilder, int32 Index, FLinearColor Value)
	{
		OutMeshBuilder = this;
		MeshAccessor->SetColor(Index, Value.ToFColor(false));
		return Index;
	}
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMeshBuilder")
	int32 SetUV(URuntimeBlueprintMeshBuilder*& OutMeshBuilder, int32 Index, FVector2D Value, int32 Channel = 0)
	{
		OutMeshBuilder = this;
		MeshAccessor->SetUV(Index, Channel, Value);
		return Index;
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMeshBuilder")
	int32 SetNormalTangent(URuntimeBlueprintMeshBuilder*& OutMeshBuilder, int32 Index, FVector Normal, FRuntimeMeshTangent Tangent)
	{
		OutMeshBuilder = this;
		MeshAccessor->SetNormalTangent(Index, Normal, Tangent);
		return Index;
	}
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMeshBuilder")
	int32 SetTangents(URuntimeBlueprintMeshBuilder*& OutMeshBuilder, int32 Index, FVector TangentX, FVector TangentY, FVector TangentZ)
	{
		OutMeshBuilder = this;
		MeshAccessor->SetTangents(Index, TangentX, TangentY, TangentZ);
		return Index;
	}

	//FRuntimeMeshAccessorVertex GetVertex(int32 Index) const;
	//void SetVertex(int32 Index, const FRuntimeMeshAccessorVertex& Vertex);
	//int32 AddVertex(const FRuntimeMeshAccessorVertex& Vertex);


	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMeshBuilder")
	int32 AddIndex(URuntimeBlueprintMeshBuilder*& OutMeshBuilder, int32 NewIndex)
	{
		OutMeshBuilder = this;
		return MeshAccessor->AddIndex(NewIndex);
	}
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMeshBuilder")
	int32 AddTriangle(URuntimeBlueprintMeshBuilder*& OutMeshBuilder, int32 Index0, int32 Index1, int32 Index2)
	{
		OutMeshBuilder = this;
		return MeshAccessor->AddTriangle(Index0, Index1, Index2);
	}

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMeshBuilder")
	int32 GetIndex(URuntimeBlueprintMeshBuilder*& OutMeshBuilder, int32 Index)
	{
		OutMeshBuilder = this;
		return MeshAccessor->GetIndex(Index);
	}
	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMeshBuilder")
	void SetIndex(URuntimeBlueprintMeshBuilder*& OutMeshBuilder, int32 Index, int32 Value)
	{
		OutMeshBuilder = this;
		MeshAccessor->SetIndex(Index, Value);
	}
};



UCLASS()
class RUNTIMEMESHCOMPONENT_API URuntimeMeshBuilderFunctions : public UBlueprintFunctionLibrary
{

	GENERATED_BODY()

public:

	UFUNCTION(BlueprintCallable, Category = "Components|RuntimeMeshBuilder")
	static URuntimeBlueprintMeshBuilder* MakeRuntimeMeshBuilder(bool bWantsHighPrecisionTangents = false, bool bWantsHighPrecisionUVs = false, int32 NumUVs = 1, bool bUse16BitIndices = false);
};