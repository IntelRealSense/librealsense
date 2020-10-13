// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class URuntimeMeshComponent;

/**
 *
 */
class FRuntimeMeshComponentLegacySerialization
{
public:
	static bool Serialize(FArchive& Ar);

private:
	static void SerializeV1(FArchive& Ar);
	static void SerializeV2(FArchive& Ar);

	static void SerializeRMCSection(FArchive& Ar, int32 SectionIndex);

	static void SectionSerialize(FArchive& Ar, bool bNeedsPositionOnlyBuffer);
};
