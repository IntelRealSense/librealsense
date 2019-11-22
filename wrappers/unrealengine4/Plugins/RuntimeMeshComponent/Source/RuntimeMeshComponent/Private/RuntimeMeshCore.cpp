// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshCore.h"
#include "RuntimeMeshComponentPlugin.h"


//////////////////////////////////////////////////////////////////////////
//	FRuntimeMeshVertexStreamStructureElement

bool FRuntimeMeshVertexStreamStructureElement::operator==(const FRuntimeMeshVertexStreamStructureElement& Other) const
{
	return Offset == Other.Offset && Stride == Other.Stride && Type == Other.Type;
}

bool FRuntimeMeshVertexStreamStructureElement::operator!=(const FRuntimeMeshVertexStreamStructureElement& Other) const
{
	return !(*this == Other);
}


//////////////////////////////////////////////////////////////////////////
//	FRuntimeMeshVertexStreamStructure

bool FRuntimeMeshVertexStreamStructure::operator==(const FRuntimeMeshVertexStreamStructure& Other) const
{
	// First check the UVs
	if (UVs.Num() != Other.UVs.Num())
	{
		return false;
	}

	for (int32 Index = 0; Index < UVs.Num(); Index++)
	{
		if (UVs[Index] != Other.UVs[Index])
		{
			return false;
		}
	}

	// Then check the other components
	return Position == Other.Position &&
		Normal == Other.Normal &&
		Tangent == Other.Tangent &&
		Color == Other.Color;
}

bool FRuntimeMeshVertexStreamStructure::operator!=(const FRuntimeMeshVertexStreamStructure& Other) const
{
	return !(*this == Other);
}

bool FRuntimeMeshVertexStreamStructure::HasAnyElements() const
{
	static const auto IsElementValid = [](const FRuntimeMeshVertexStreamStructureElement& Element) -> bool
	{
		return Element.Type != EVertexElementType::VET_None;
	};

	for (const auto& Elem : UVs)
	{
		if (IsElementValid(Elem))
		{
			return true;
		}
	}

	return IsElementValid(Position) || IsElementValid(Normal) || IsElementValid(Tangent) || IsElementValid(Color);
}

bool FRuntimeMeshVertexStreamStructure::HasUVs() const
{
	return UVs.Num() > 0;
}

uint8 FRuntimeMeshVertexStreamStructure::CalculateStride() const
{
	uint8 MaxStride = 0;
	const auto AddElement = [&MaxStride](const FRuntimeMeshVertexStreamStructureElement& Element)
	{
		if (Element.Type != EVertexElementType::VET_None)
		{
			check(MaxStride == 0 || Element.Stride == MaxStride || Element.Stride == 0);
			MaxStride = FMath::Max(MaxStride, Element.Stride);
		}
	};

	AddElement(Position);
	AddElement(Normal);
	AddElement(Tangent);
	for (const auto& Elem : UVs)
	{
		AddElement(Elem);
	}
	AddElement(Color);


	return MaxStride;
}

bool FRuntimeMeshVertexStreamStructure::IsValid() const
{
	static const auto IsElementValid = [](const FRuntimeMeshVertexStreamStructureElement& Element) -> bool
	{
		return Element.Type == EVertexElementType::VET_None || Element.Stride > 0;
	};

	for (const auto& Elem : UVs)
	{
		if (!IsElementValid(Elem))
		{
			return false;
		}
	}

	return IsElementValid(Position) && IsElementValid(Normal) && IsElementValid(Tangent) && IsElementValid(Color);
}

bool FRuntimeMeshVertexStreamStructure::HasNoOverlap(const FRuntimeMeshVertexStreamStructure& Other) const
{
	static const auto HasNoOverlap = [](const FRuntimeMeshVertexStreamStructureElement& Left, const FRuntimeMeshVertexStreamStructureElement& Right) -> bool
	{
		return Left.Type == EVertexElementType::VET_None || Right.Type == EVertexElementType::VET_None;
	};

	// two streams can't have UVs
	if (UVs.Num() > 0 && Other.UVs.Num() > 0)
	{
		return false;
	}

	// Now check overlap of the other components
	return HasNoOverlap(Position, Other.Position) && HasNoOverlap(Normal, Other.Normal) && HasNoOverlap(Tangent, Other.Tangent) && HasNoOverlap(Color, Other.Color);
}

bool FRuntimeMeshVertexStreamStructure::ValidTripleStream(const FRuntimeMeshVertexStreamStructure& Stream1, const FRuntimeMeshVertexStreamStructure& Stream2, const FRuntimeMeshVertexStreamStructure& Stream3)
{
	return Stream1.IsValid() && Stream2.IsValid() && Stream3.IsValid() &&
		Stream1.HasNoOverlap(Stream2) && Stream1.HasNoOverlap(Stream3) && Stream2.HasNoOverlap(Stream3) &&
		Stream1.HasAnyElements() && Stream1.Position.IsValid() &&
		(!Stream3.HasAnyElements() || Stream2.HasAnyElements());
}
