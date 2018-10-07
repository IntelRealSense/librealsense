// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#pragma once

#include "RuntimeMeshCore.h"
#include "Containers/StaticArray.h"

//////////////////////////////////////////////////////////////////////////
//	
//	This file contains a generic vertex structure capable of efficiently representing a vertex
//	with any combination of position, normal, tangent, color, and 0-8 UV channels.
//
//	Example use: (This defines a vertex with all components and 1 UV with default precision for normal/tangent and UV)
//
//	DECLARE_RUNTIME_MESH_VERTEX(MyVertex, true, true, true, true, 1, ERuntimeMeshVertexTangentBasisType::Default, ERuntimeMeshVertexUVType::Default)
//	
//	MyVertex Vertex;
//	Vertex.Position = FVector(0,0,0);
//	Vertex.Normal = FVector(0,0,0);
//	Vertex.UV0 = FVector2D(0,0);
//
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// Texture Component Type Selector
//////////////////////////////////////////////////////////////////////////

enum class ERuntimeMeshVertexUVType
{
	Default = 1,
	HighPrecision = 2,
};

template<ERuntimeMeshVertexUVType UVType>
struct FRuntimeMeshVertexUVsTypeSelector;

template<>
struct FRuntimeMeshVertexUVsTypeSelector<ERuntimeMeshVertexUVType::Default>
{
	typedef FVector2DHalf UVsType;
	static const EVertexElementType VertexElementType1Channel = VET_Half2;
	static const EVertexElementType VertexElementType2Channel = VET_Half4;
};

template<>
struct FRuntimeMeshVertexUVsTypeSelector<ERuntimeMeshVertexUVType::HighPrecision>
{
	typedef FVector2D UVsType;
	static const EVertexElementType VertexElementType1Channel = VET_Float2;
	static const EVertexElementType VertexElementType2Channel = VET_Float4;
};


//////////////////////////////////////////////////////////////////////////
// Tangent Basis Component Type Selector
//////////////////////////////////////////////////////////////////////////

enum class ERuntimeMeshVertexTangentBasisType
{
	Default = 1,
	HighPrecision = 2,
};

template<ERuntimeMeshVertexTangentBasisType TangentBasisType>
struct FRuntimeMeshVertexTangentTypeSelector;

template<>
struct FRuntimeMeshVertexTangentTypeSelector<ERuntimeMeshVertexTangentBasisType::Default>
{
	typedef FPackedNormal TangentsType;
	static const EVertexElementType VertexElementType = VET_PackedNormal;
};

template<>
struct FRuntimeMeshVertexTangentTypeSelector<ERuntimeMeshVertexTangentBasisType::HighPrecision>
{
	typedef FPackedRGBA16N TangentsType;
	static const EVertexElementType VertexElementType = VET_UShort4N;
};

struct RuntimeMeshNormalUtil
{
	static void SetNormalW(FPackedNormal& Target, float Determinant)
	{
		Target.Vector.W = Determinant < 0.0f ? 0 : -1; // -1 == 0xFF
	}

	static void SetNormalW(FPackedRGBA16N& Target, float Determinant)
	{
		Target.W = Determinant < 0.0f ? 0 : -1; // -1 == 0xFFFF
	}
};

//////////////////////////////////////////////////////////////////////////
// Vertex Structure Generator
//////////////////////////////////////////////////////////////////////////

struct FRuntimeMeshVertexUtilities
{
	//////////////////////////////////////////////////////////////////////////
	// Position Component
	//////////////////////////////////////////////////////////////////////////
	template<typename RuntimeVertexType, bool WantsPosition>
	struct PositionComponent
	{
		static void AddComponent(FRuntimeMeshVertexStreamStructure& VertexStructure)
		{
			VertexStructure.Position = RUNTIMEMESH_VERTEXSTREAMCOMPONENT(RuntimeVertexType, Position, VET_Float3);
		}

	};

	template<typename RuntimeVertexType>
	struct PositionComponent<RuntimeVertexType, false>
	{
		static void AddComponent(FRuntimeMeshVertexStreamStructure& VertexStructure)
		{
		}
	};

	//////////////////////////////////////////////////////////////////////////
	// Normal/Tangent Components
	//////////////////////////////////////////////////////////////////////////
	template<typename RuntimeVertexType, bool WantsNormal, bool WantsTangent, ERuntimeMeshVertexTangentBasisType NormalTangentType>
	struct NormalTangentComponent
	{
		static void AddComponent(FRuntimeMeshVertexStreamStructure& VertexStructure)
		{
			VertexStructure.Normal = RUNTIMEMESH_VERTEXSTREAMCOMPONENT(RuntimeVertexType, Normal,
				FRuntimeMeshVertexTangentTypeSelector<NormalTangentType>::VertexElementType);
			VertexStructure.Tangent = RUNTIMEMESH_VERTEXSTREAMCOMPONENT(RuntimeVertexType, Tangent,
				FRuntimeMeshVertexTangentTypeSelector<NormalTangentType>::VertexElementType);
		}
	};

	template<typename RuntimeVertexType, ERuntimeMeshVertexTangentBasisType NormalTangentType>
	struct NormalTangentComponent<RuntimeVertexType, true, false, NormalTangentType>
	{
		static void AddComponent(FRuntimeMeshVertexStreamStructure& VertexStructure)
		{
			VertexStructure.Normal = RUNTIMEMESH_VERTEXSTREAMCOMPONENT(RuntimeVertexType, Normal,
				FRuntimeMeshVertexTangentTypeSelector<NormalTangentType>::VertexElementType);
		}
	};

	template<typename RuntimeVertexType, ERuntimeMeshVertexTangentBasisType NormalTangentType>
	struct NormalTangentComponent<RuntimeVertexType, false, true, NormalTangentType>
	{
		static void AddComponent(FRuntimeMeshVertexStreamStructure& VertexStructure)
		{
			VertexStructure.Tangent = RUNTIMEMESH_VERTEXSTREAMCOMPONENT(RuntimeVertexType, Tangent,
				FRuntimeMeshVertexTangentTypeSelector<NormalTangentType>::VertexElementType);
		}
	};

	template<typename RuntimeVertexType, ERuntimeMeshVertexTangentBasisType NormalTangentType>
	struct NormalTangentComponent<RuntimeVertexType, false, false, NormalTangentType>
	{
		static void AddComponent(FRuntimeMeshVertexStreamStructure& VertexStructure)
		{
		}
	};

	//////////////////////////////////////////////////////////////////////////
	// Color Component
	//////////////////////////////////////////////////////////////////////////
	template<typename RuntimeVertexType, bool WantsColor>
	struct ColorComponent
	{
		static void AddComponent(FRuntimeMeshVertexStreamStructure& VertexStructure)
		{
			VertexStructure.Color = RUNTIMEMESH_VERTEXSTREAMCOMPONENT(RuntimeVertexType, Color, VET_Color);
		}
	};

	template<typename RuntimeVertexType>
	struct ColorComponent<RuntimeVertexType, false>
	{
		static void AddComponent(FRuntimeMeshVertexStreamStructure& VertexStructure)
		{
		}
	};


	//////////////////////////////////////////////////////////////////////////
	// UV Components
	//////////////////////////////////////////////////////////////////////////
	template<typename RuntimeVertexType, int32 NumWantedUVChannels, ERuntimeMeshVertexUVType UVType>
	struct TextureChannels
	{
		static void AddChannels(FRuntimeMeshVertexStreamStructure& VertexStructure)
		{
		}
	};

	template<typename RuntimeVertexType, ERuntimeMeshVertexUVType UVType>
	struct TextureChannels<RuntimeVertexType, 1, UVType>
	{
		static void AddChannels(FRuntimeMeshVertexStreamStructure& VertexStructure)
		{
			VertexStructure.UVs.Add(RUNTIMEMESH_VERTEXSTREAMCOMPONENT(RuntimeVertexType, UV0, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType1Channel));
		}
	};

	template<typename RuntimeVertexType, ERuntimeMeshVertexUVType UVType>
	struct TextureChannels<RuntimeVertexType, 2, UVType>
	{
		static void AddChannels(FRuntimeMeshVertexStreamStructure& VertexStructure)
		{
			VertexStructure.UVs.Add(RUNTIMEMESH_VERTEXSTREAMCOMPONENT(RuntimeVertexType, UV0, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
		}
	};

	template<typename RuntimeVertexType, ERuntimeMeshVertexUVType UVType>
	struct TextureChannels<RuntimeVertexType, 3, UVType>
	{
		static void AddChannels(FRuntimeMeshVertexStreamStructure& VertexStructure)
		{

			VertexStructure.UVs.Add(RUNTIMEMESH_VERTEXSTREAMCOMPONENT(RuntimeVertexType, UV0, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
			VertexStructure.UVs.Add(RUNTIMEMESH_VERTEXSTREAMCOMPONENT(RuntimeVertexType, UV2, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType1Channel));
		}
	};

	template<typename RuntimeVertexType, ERuntimeMeshVertexUVType UVType>
	struct TextureChannels<RuntimeVertexType, 4, UVType>
	{
		static void AddChannels(FRuntimeMeshVertexStreamStructure& VertexStructure)
		{
			VertexStructure.UVs.Add(RUNTIMEMESH_VERTEXSTREAMCOMPONENT(RuntimeVertexType, UV0, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
			VertexStructure.UVs.Add(RUNTIMEMESH_VERTEXSTREAMCOMPONENT(RuntimeVertexType, UV2, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
		}
	};

	template<typename RuntimeVertexType, ERuntimeMeshVertexUVType UVType>
	struct TextureChannels<RuntimeVertexType, 5, UVType>
	{
		static void AddChannels(FRuntimeMeshVertexStreamStructure& VertexStructure)
		{
			VertexStructure.UVs.Add(RUNTIMEMESH_VERTEXSTREAMCOMPONENT(RuntimeVertexType, UV0, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
			VertexStructure.UVs.Add(RUNTIMEMESH_VERTEXSTREAMCOMPONENT(RuntimeVertexType, UV2, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
			VertexStructure.UVs.Add(RUNTIMEMESH_VERTEXSTREAMCOMPONENT(RuntimeVertexType, UV4, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType1Channel));
		}
	};

	template<typename RuntimeVertexType, ERuntimeMeshVertexUVType UVType>
	struct TextureChannels<RuntimeVertexType, 6, UVType>
	{
		static void AddChannels(FRuntimeMeshVertexStreamStructure& VertexStructure)
		{
			VertexStructure.UVs.Add(RUNTIMEMESH_VERTEXSTREAMCOMPONENT(RuntimeVertexType, UV0, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
			VertexStructure.UVs.Add(RUNTIMEMESH_VERTEXSTREAMCOMPONENT(RuntimeVertexType, UV2, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
			VertexStructure.UVs.Add(RUNTIMEMESH_VERTEXSTREAMCOMPONENT(RuntimeVertexType, UV4, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
		}
	};

	template<typename RuntimeVertexType, ERuntimeMeshVertexUVType UVType>
	struct TextureChannels<RuntimeVertexType, 7, UVType>
	{
		static void AddChannels(FRuntimeMeshVertexStreamStructure& VertexStructure)
		{
			VertexStructure.UVs.Add(RUNTIMEMESH_VERTEXSTREAMCOMPONENT(RuntimeVertexType, UV0, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
			VertexStructure.UVs.Add(RUNTIMEMESH_VERTEXSTREAMCOMPONENT(RuntimeVertexType, UV2, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
			VertexStructure.UVs.Add(RUNTIMEMESH_VERTEXSTREAMCOMPONENT(RuntimeVertexType, UV4, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
			VertexStructure.UVs.Add(RUNTIMEMESH_VERTEXSTREAMCOMPONENT(RuntimeVertexType, UV6, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType1Channel));
		}
	};

	template<typename RuntimeVertexType, ERuntimeMeshVertexUVType UVType>
	struct TextureChannels<RuntimeVertexType, 8, UVType>
	{
		static void AddChannels(FRuntimeMeshVertexStreamStructure& VertexStructure)
		{
			VertexStructure.UVs.Add(RUNTIMEMESH_VERTEXSTREAMCOMPONENT(RuntimeVertexType, UV0, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
			VertexStructure.UVs.Add(RUNTIMEMESH_VERTEXSTREAMCOMPONENT(RuntimeVertexType, UV2, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
			VertexStructure.UVs.Add(RUNTIMEMESH_VERTEXSTREAMCOMPONENT(RuntimeVertexType, UV4, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
			VertexStructure.UVs.Add(RUNTIMEMESH_VERTEXSTREAMCOMPONENT(RuntimeVertexType, UV6, FRuntimeMeshVertexUVsTypeSelector<UVType>::VertexElementType2Channel));
		}
	};
};





//////////////////////////////////////////////////////////////////////////
// Struct Member Builders
//////////////////////////////////////////////////////////////////////////

// Position declaration
#define RUNTIMEMESH_VERTEX_DECLARE_POSITION_true FVector Position;
#define RUNTIMEMESH_VERTEX_DECLARE_POSITION_false 
#define RUNTIMEMESH_VERTEX_DECLARE_POSITION(HasPosition) RUNTIMEMESH_VERTEX_DECLARE_POSITION_##HasPosition

// Normal declaration
#define RUNTIMEMESH_VERTEX_DECLARE_NORMAL_true(TangentType) FRuntimeMeshVertexTangentTypeSelector<TangentType>::TangentsType Normal;
#define RUNTIMEMESH_VERTEX_DECLARE_NORMAL_false(TangentType)
#define RUNTIMEMESH_VERTEX_DECLARE_NORMAL(HasNormal, TangentType) RUNTIMEMESH_VERTEX_DECLARE_NORMAL_##HasNormal(TangentType)

// Tangent declaration
#define RUNTIMEMESH_VERTEX_DECLARE_TANGENT_true(TangentType) FRuntimeMeshVertexTangentTypeSelector<TangentType>::TangentsType Tangent;
#define RUNTIMEMESH_VERTEX_DECLARE_TANGENT_false(TangentType)
#define RUNTIMEMESH_VERTEX_DECLARE_TANGENT(HasTangent, TangentType) RUNTIMEMESH_VERTEX_DECLARE_TANGENT_##HasTangent(TangentType)

// Color declaration
#define RUNTIMEMESH_VERTEX_DECLARE_COLOR_true FColor Color;
#define RUNTIMEMESH_VERTEX_DECLARE_COLOR_false
#define RUNTIMEMESH_VERTEX_DECLARE_COLOR(HasColor) RUNTIMEMESH_VERTEX_DECLARE_COLOR_##HasColor

// UV Channels
#define RUNTIMEMESH_VERTEX_DECLARE_UVCHANNEL_0(UVType)

#define RUNTIMEMESH_VERTEX_DECLARE_UVCHANNEL_1(UVType) \
	FRuntimeMeshVertexUVsTypeSelector<UVType>::UVsType UV0;

#define RUNTIMEMESH_VERTEX_DECLARE_UVCHANNEL_2(UVType) \
	RUNTIMEMESH_VERTEX_DECLARE_UVCHANNEL_1(UVType) \
	FRuntimeMeshVertexUVsTypeSelector<UVType>::UVsType UV1;

#define RUNTIMEMESH_VERTEX_DECLARE_UVCHANNEL_3(UVType) \
	RUNTIMEMESH_VERTEX_DECLARE_UVCHANNEL_2(UVType) \
	FRuntimeMeshVertexUVsTypeSelector<UVType>::UVsType UV2;

#define RUNTIMEMESH_VERTEX_DECLARE_UVCHANNEL_4(UVType) \
	RUNTIMEMESH_VERTEX_DECLARE_UVCHANNEL_3(UVType) \
	FRuntimeMeshVertexUVsTypeSelector<UVType>::UVsType UV3;

#define RUNTIMEMESH_VERTEX_DECLARE_UVCHANNEL_5(UVType) \
	RUNTIMEMESH_VERTEX_DECLARE_UVCHANNEL_4(UVType) \
	FRuntimeMeshVertexUVsTypeSelector<UVType>::UVsType UV4;

#define RUNTIMEMESH_VERTEX_DECLARE_UVCHANNEL_6(UVType) \
	RUNTIMEMESH_VERTEX_DECLARE_UVCHANNEL_5(UVType) \
	FRuntimeMeshVertexUVsTypeSelector<UVType>::UVsType UV5;

#define RUNTIMEMESH_VERTEX_DECLARE_UVCHANNEL_7(UVType) \
	RUNTIMEMESH_VERTEX_DECLARE_UVCHANNEL_6(UVType) \
	FRuntimeMeshVertexUVsTypeSelector<UVType>::UVsType UV6;

#define RUNTIMEMESH_VERTEX_DECLARE_UVCHANNEL_8(UVType) \
	RUNTIMEMESH_VERTEX_DECLARE_UVCHANNEL_7(UVType) \
	FRuntimeMeshVertexUVsTypeSelector<UVType>::UVsType UV7;

#define RUNTIMEMESH_VERTEX_DECLARE_UVCHANNELS(NumChannels, UVType) RUNTIMEMESH_VERTEX_DECLARE_UVCHANNEL_##NumChannels(UVType)




//////////////////////////////////////////////////////////////////////////
//	Member default inits
//////////////////////////////////////////////////////////////////////////

#define RUNTIMEMESH_VERTEX_DEFAULTINIT_POSITION_true Position = FVector(0.0f, 0.0f, 0.0f);
#define RUNTIMEMESH_VERTEX_DEFAULTINIT_POSITION_false 
#define RUNTIMEMESH_VERTEX_DEFAULTINIT_POSITION(HasPosition) RUNTIMEMESH_VERTEX_DEFAULTINIT_POSITION_##HasPosition

#define RUNTIMEMESH_VERTEX_DEFAULTINIT_NORMAL_true Normal = FVector4(0.0f, 0.0f, 1.0f, 1.0f);
#define RUNTIMEMESH_VERTEX_DEFAULTINIT_NORMAL_false 
#define RUNTIMEMESH_VERTEX_DEFAULTINIT_NORMAL(HasNormal) RUNTIMEMESH_VERTEX_DEFAULTINIT_NORMAL_##HasNormal

#define RUNTIMEMESH_VERTEX_DEFAULTINIT_TANGENT_true Tangent = FVector(1.0f, 0.0f, 0.0f);
#define RUNTIMEMESH_VERTEX_DEFAULTINIT_TANGENT_false 
#define RUNTIMEMESH_VERTEX_DEFAULTINIT_TANGENT(HasTangent) RUNTIMEMESH_VERTEX_DEFAULTINIT_TANGENT_##HasTangent

#define RUNTIMEMESH_VERTEX_DEFAULTINIT_COLOR_true Color = FColor::White;
#define RUNTIMEMESH_VERTEX_DEFAULTINIT_COLOR_false 
#define RUNTIMEMESH_VERTEX_DEFAULTINIT_COLOR(HasColor) RUNTIMEMESH_VERTEX_DEFAULTINIT_COLOR_##HasColor

#define RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_0

#define RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_1 \
	UV0 = FVector2D(0.0f, 0.0f);

#define RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_2 \
	RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_1 \
	UV1 = FVector2D(0.0f, 0.0f);

#define RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_3 \
	RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_2 \
	UV2 = FVector2D(0.0f, 0.0f);

#define RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_4 \
	RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_3 \
	UV3 = FVector2D(0.0f, 0.0f);

#define RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_5 \
	RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_4 \
	UV4 = FVector2D(0.0f, 0.0f);

#define RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_6 \
	RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_5 \
	UV5 = FVector2D(0.0f, 0.0f);

#define RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_7 \
	RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_6 \
	UV6 = FVector2D(0.0f, 0.0f);

#define RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_8 \
	RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_7 \
	UV7 = FVector2D(0.0f, 0.0f);

#define RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNELS(NumChannels) RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNEL_##NumChannels



//////////////////////////////////////////////////////////////////////////
//	Constructor Parameter Builders
//////////////////////////////////////////////////////////////////////////

#define RUNTIMEMESH_VERTEX_PARAMETER_POSITION_true const FVector& InPosition, 
#define RUNTIMEMESH_VERTEX_PARAMETER_POSITION_false
#define RUNTIMEMESH_VERTEX_PARAMETER_POSITION(NeedsPosition) RUNTIMEMESH_VERTEX_PARAMETER_POSITION_##NeedsPosition

#define RUNTIMEMESH_VERTEX_INIT_POSITION_true Position = InPosition;
#define RUNTIMEMESH_VERTEX_INIT_POSITION_false
#define RUNTIMEMESH_VERTEX_INIT_POSITION(NeedsPosition) RUNTIMEMESH_VERTEX_INIT_POSITION_##NeedsPosition



#define RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_0
#define RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_1 , const FVector2D& InUV0 = FVector2D::ZeroVector
#define RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_2 RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_1 , const FVector2D& InUV1 = FVector2D::ZeroVector
#define RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_3 RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_2 , const FVector2D& InUV2 = FVector2D::ZeroVector
#define RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_4 RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_3 , const FVector2D& InUV3 = FVector2D::ZeroVector
#define RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_5 RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_4 , const FVector2D& InUV4 = FVector2D::ZeroVector
#define RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_6 RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_5 , const FVector2D& InUV5 = FVector2D::ZeroVector
#define RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_7 RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_6 , const FVector2D& InUV6 = FVector2D::ZeroVector
#define RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_8 RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_7 , const FVector2D& InUV7 = FVector2D::ZeroVector
#define RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNELS(NumChannels) RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNEL_##NumChannels


#define RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_0

#define RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_1 \
	UV0 = InUV0;

#define RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_2 \
	RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_1 \
	UV1 = InUV1;

#define RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_3 \
	RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_2 \
	UV2 = InUV2;

#define RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_4 \
	RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_3 \
	UV3 = InUV3;

#define RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_5 \
	RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_4 \
	UV4 = InUV4;

#define RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_6 \
	RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_5 \
	UV5 = InUV5;

#define RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_7 \
	RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_6 \
	UV6 = InUV6;

#define RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_8 \
	RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_7 \
	UV7 = InUV7;

#define RUNTIMEMESH_VERTEX_INIT_UVCHANNELS(NumChannels) RUNTIMEMESH_VERTEX_INIT_UVCHANNEL_##NumChannels



//////////////////////////////////////////////////////////////////////////
//	Constructor Definitions
//////////////////////////////////////////////////////////////////////////


// PreProcessor IF with pass through for all the constructor arguments
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, Condition, IfTrue) \
	RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF_##Condition(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, IfTrue)
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF_false(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, IfTrue)
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF_true(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, IfTrue) IfTrue


// Implementation of Position only Constructor
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_IMPLEMENTATION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType) \
	VertexName(const FVector& InPosition)							\
	{																\
		RUNTIMEMESH_VERTEX_INIT_POSITION(NeedsPosition)				\
		RUNTIMEMESH_VERTEX_DEFAULTINIT_NORMAL(NeedsNormal)			\
		RUNTIMEMESH_VERTEX_DEFAULTINIT_TANGENT(NeedsTangent)		\
		RUNTIMEMESH_VERTEX_DEFAULTINIT_COLOR(NeedsColor)			\
		RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNELS(UVChannelCount)	\
	}

// Defines the Position Constructor if it's wanted
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)								\
	RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, NeedsPosition,				\
		RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_IMPLEMENTATION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)				\
	)

// Implementation of Position/Normal Constructor
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_NORMAL_IMPLEMENTATION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType) \
	VertexName(RUNTIMEMESH_VERTEX_PARAMETER_POSITION(NeedsPosition) const FVector& InNormal)	\
	{																\
		RUNTIMEMESH_VERTEX_INIT_POSITION(NeedsPosition)				\
		Normal = InNormal;											\
		RUNTIMEMESH_VERTEX_DEFAULTINIT_TANGENT(NeedsTangent)		\
		RUNTIMEMESH_VERTEX_DEFAULTINIT_COLOR(NeedsColor)			\
		RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNELS(UVChannelCount)	\
	}

// Defines the Position/Normal Constructor if it's wanted
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_NORMAL(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)						\
	RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, NeedsNormal,				\
		RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_NORMAL_IMPLEMENTATION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)		\
	)

// Implementation of Position/Color Constructor
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_COLOR_IMPLEMENTATION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType) \
	VertexName(RUNTIMEMESH_VERTEX_PARAMETER_POSITION(NeedsPosition) const FColor& InColor RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNELS(UVChannelCount))	\
	{																\
		RUNTIMEMESH_VERTEX_INIT_POSITION(NeedsPosition)				\
		RUNTIMEMESH_VERTEX_DEFAULTINIT_NORMAL(NeedsNormal)			\
		RUNTIMEMESH_VERTEX_DEFAULTINIT_TANGENT(NeedsTangent)		\
		Color = InColor;											\
		RUNTIMEMESH_VERTEX_INIT_UVCHANNELS(UVChannelCount)			\
	}

// Defines the Position/Color Constructor if it's wanted
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_COLOR(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)					\
	RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, NeedsColor,				\
		RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_COLOR_IMPLEMENTATION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)		\
	)


// Implementation of Position/Normal/Tangent Constructor
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_NORMAL_TANGENT_IMPLEMENTATION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)	\
	VertexName(RUNTIMEMESH_VERTEX_PARAMETER_POSITION(NeedsPosition) const FVector& InNormal, const FRuntimeMeshTangent& InTangent RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNELS(UVChannelCount))		\
	{																	\
		RUNTIMEMESH_VERTEX_INIT_POSITION(NeedsPosition)					\
		Normal = FVector4(InNormal, InTangent.bFlipTangentY? -1 : 1);	\
		Tangent = InTangent.TangentX;									\
		RUNTIMEMESH_VERTEX_DEFAULTINIT_COLOR(NeedsColor)				\
		RUNTIMEMESH_VERTEX_INIT_UVCHANNELS(UVChannelCount)				\
	}

// Defines the Position/Normal/Tangent Constructor if it's wanted
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_NORMAL_TANGENT(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)						\
	RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, NeedsNormal,						\
		RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, NeedsTangent,					\
			RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_NORMAL_TANGENT_IMPLEMENTATION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)	\
		)																																															\
	)


// Implementation of Position/TangentX/TangentY/TangentZ Constructor
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_TANGENTX_TANGENTY_TANGENTZ_IMPLEMENTATION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)		\
	VertexName(RUNTIMEMESH_VERTEX_PARAMETER_POSITION(NeedsPosition) const FVector& InTangentX, const FVector& InTangentY, const FVector& InTangentZ RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNELS(UVChannelCount))	\
	{																											\
		RUNTIMEMESH_VERTEX_INIT_POSITION(NeedsPosition)															\
		Normal = FVector4(InTangentZ, GetBasisDeterminantSign(InTangentX, InTangentY, InTangentZ));				\
		Tangent = InTangentX;																					\
		RUNTIMEMESH_VERTEX_DEFAULTINIT_COLOR(NeedsColor)														\
		RUNTIMEMESH_VERTEX_INIT_UVCHANNELS(UVChannelCount)														\
	}

// Defines the Position/TangentX/TangentY/TangentZ Constructor if it's wanted
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_TANGENTX_TANGENTY_TANGENTZ(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)						\
	RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, NeedsNormal,									\
		RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, NeedsTangent,								\
			RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_TANGENTX_TANGENTY_TANGENTZ_IMPLEMENTATION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)	\
		)																																																		\
	)


// Implementation of Position/Normal/Tangent Constructor
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_NORMAL_TANGENT_COLOR_IMPLEMENTATION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)					\
	VertexName(RUNTIMEMESH_VERTEX_PARAMETER_POSITION(NeedsPosition) const FVector& InNormal, const FRuntimeMeshTangent& InTangent, const FColor& InColor RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNELS(UVChannelCount))	\
	{																	\
		RUNTIMEMESH_VERTEX_INIT_POSITION(NeedsPosition)					\
		Normal = FVector4(InNormal, InTangent.bFlipTangentY? -1 : 1);	\
		Tangent = InTangent.TangentX;									\
		Color = InColor;												\
		RUNTIMEMESH_VERTEX_INIT_UVCHANNELS(UVChannelCount)				\
	}

// Defines the Position/Normal/Tangent Constructor if it's wanted
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_NORMAL_TANGENT_COLOR(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)								\
	RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, NeedsNormal,									\
		RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, NeedsTangent,								\
			RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, NeedsColor,								\
				RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_NORMAL_TANGENT_COLOR_IMPLEMENTATION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)		\
			)																																																	\
		)																																																		\
	)


// Implementation of Position/TangentX/TangentY/TangentZ Constructor
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_TANGENTX_TANGENTY_TANGENTZ_COLOR_IMPLEMENTATION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)							\
	VertexName(RUNTIMEMESH_VERTEX_PARAMETER_POSITION(NeedsPosition) const FVector& InTangentX, const FVector& InTangentY, const FVector& InTangentZ, const FColor& InColor RUNTIMEMESH_VERTEX_PARAMETER_UVCHANNELS(UVChannelCount))		\
	{																											\
		RUNTIMEMESH_VERTEX_INIT_POSITION(NeedsPosition)															\
		Normal = FVector4(InTangentZ, GetBasisDeterminantSign(InTangentX, InTangentY, InTangentZ));				\
		Tangent = InTangentX;																					\
		Color = InColor;																						\
		RUNTIMEMESH_VERTEX_INIT_UVCHANNELS(UVChannelCount)														\
	}

// Defines the Position/TangentX/TangentY/TangentZ Constructor if it's wanted
#define RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_TANGENTX_TANGENTY_TANGENTZ_COLOR(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)							\
	RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, NeedsNormal,											\
		RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, NeedsTangent,										\
			RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, NeedsColor,										\
				RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_TANGENTX_TANGENTY_TANGENTZ_COLOR_IMPLEMENTATION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)	\
			)																																																			\
		)																																																				\
	)


// Implementation of Position only Constructor
#define RUNTIMEMESH_VERTEX_NORMALTANGENT_SET_MPLEMENTATION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)	\
	void SetTangents(const FVector& TangentX, const FVector& TangentY, const FVector& TangentZ)																				\
	{																																										\
		Normal = FVector4(TangentZ, GetBasisDeterminantSign(TangentX, TangentY, TangentZ));																					\
		Tangent = TangentX;																																					\
	}																																										\
																																											\
	void SetTangent(const FRuntimeMeshTangent& InTangent)																													\
	{																																										\
		InTangent.ModifyNormal(Normal);																																		\
		Tangent = InTangent.TangentX;																																		\
	}

// Defines the Position Constructor if it's wanted
#define RUNTIMEMESH_VERTEX_NORMALTANGENT_SET(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)								\
	RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, NeedsNormal,			\
		RUNTIMEMESH_VERTEX_CONSTRUCTOR_DEFINITION_IF(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, NeedsTangent,		\
			RUNTIMEMESH_VERTEX_NORMALTANGENT_SET_MPLEMENTATION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)			\
		)																																												\
	)





#define __DECLARE_RUNTIME_MESH_VERTEXINTERNAL(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, APIQUALIFIER)							\
	struct APIQUALIFIER VertexName																																									\
	{																																																\
		RUNTIMEMESH_VERTEX_DECLARE_POSITION(NeedsPosition)																																			\
		RUNTIMEMESH_VERTEX_DECLARE_NORMAL(NeedsNormal, TangentsType)																																\
		RUNTIMEMESH_VERTEX_DECLARE_TANGENT(NeedsTangent, TangentsType)																																\
		RUNTIMEMESH_VERTEX_DECLARE_UVCHANNELS(UVChannelCount, UVChannelType)																														\
		RUNTIMEMESH_VERTEX_DECLARE_COLOR(NeedsColor)																																				\
 																																																	\
 		VertexName() { }																																											\
 																																																	\
 		VertexName(EForceInit)																																										\
 		{																																															\
 			RUNTIMEMESH_VERTEX_DEFAULTINIT_POSITION(NeedsPosition)																																	\
 			RUNTIMEMESH_VERTEX_DEFAULTINIT_NORMAL(NeedsNormal)																																		\
 			RUNTIMEMESH_VERTEX_DEFAULTINIT_TANGENT(NeedsTangent)																																	\
 			RUNTIMEMESH_VERTEX_DEFAULTINIT_UVCHANNELS(UVChannelCount)																																\
 			RUNTIMEMESH_VERTEX_DEFAULTINIT_COLOR(NeedsColor)																																		\
 		}																																															\
 																																																	\
 		RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)										\
 																																																	\
 		RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_NORMAL(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)								\
 																																																	\
 		RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_COLOR(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)								\
 																																																	\
 		RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_NORMAL_TANGENT(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)						\
 																																																	\
 		RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_TANGENTX_TANGENTY_TANGENTZ(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)			\
 																																																	\
 		RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_NORMAL_TANGENT_COLOR(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)					\
 																																																	\
 		RUNTIMEMESH_VERTEX_CONSTRUCTOR_POSITION_TANGENTX_TANGENTY_TANGENTZ_COLOR(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)		\
																																																	\
		RUNTIMEMESH_VERTEX_NORMALTANGENT_SET(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)											\
																																																	\
		static FRuntimeMeshVertexStreamStructure GetVertexStructure()																																\
		{																																															\
			FRuntimeMeshVertexStreamStructure VertexStructure;																																		\
			/* Add Position component if necessary */																																				\
			FRuntimeMeshVertexUtilities::PositionComponent<VertexName, NeedsPosition>::AddComponent(VertexStructure);																				\
			/* Add normal and tangent components if necessary */																																	\
			FRuntimeMeshVertexUtilities::NormalTangentComponent<VertexName, NeedsNormal, NeedsTangent, TangentsType>::AddComponent(VertexStructure);												\
			/* Add color component if necessary */																																					\
			FRuntimeMeshVertexUtilities::ColorComponent<VertexName, NeedsColor>::AddComponent(VertexStructure);																						\
			/* Add all texture channels */																																							\
			FRuntimeMeshVertexUtilities::TextureChannels<VertexName, UVChannelCount, UVChannelType>::AddChannels(VertexStructure);																	\
			return VertexStructure;																																									\
		}																																															\
	};		


#define DECLARE_RUNTIME_MESH_VERTEX(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType)		\
	__DECLARE_RUNTIME_MESH_VERTEXINTERNAL(VertexName, NeedsPosition, NeedsNormal, NeedsTangent, NeedsColor, UVChannelCount, TangentsType, UVChannelType, /**/)















//////////////////////////////////////////////////////////////////////////
// Name Vertex Configurations
//////////////////////////////////////////////////////////////////////////

/** Simple vertex with 1 UV channel */
__DECLARE_RUNTIME_MESH_VERTEXINTERNAL(FRuntimeMeshVertexSimple, true, true, true, true, 1, ERuntimeMeshVertexTangentBasisType::Default, ERuntimeMeshVertexUVType::HighPrecision, RUNTIMEMESHCOMPONENT_API)

/** Simple vertex with 2 UV channels */
__DECLARE_RUNTIME_MESH_VERTEXINTERNAL(FRuntimeMeshVertexDualUV, true, true, true, true, 2, ERuntimeMeshVertexTangentBasisType::Default, ERuntimeMeshVertexUVType::HighPrecision, RUNTIMEMESHCOMPONENT_API)

/** Simple vertex with 1 UV channel and NO position component (Meant to be used with separate position buffer) */
__DECLARE_RUNTIME_MESH_VERTEXINTERNAL(FRuntimeMeshVertexNoPosition, false, true, true, true, 1, ERuntimeMeshVertexTangentBasisType::Default, ERuntimeMeshVertexUVType::HighPrecision, RUNTIMEMESHCOMPONENT_API)

/** Simple vertex with 2 UV channels and NO position component (Meant to be used with separate position buffer) */
__DECLARE_RUNTIME_MESH_VERTEXINTERNAL(FRuntimeMeshVertexNoPositionDualUV, false, true, true, true, 2, ERuntimeMeshVertexTangentBasisType::Default, ERuntimeMeshVertexUVType::HighPrecision, RUNTIMEMESHCOMPONENT_API)

/** Simple vertex with 1 UV channel */
__DECLARE_RUNTIME_MESH_VERTEXINTERNAL(FRuntimeMeshVertexHiPrecisionNormals, true, true, true, true, 1, ERuntimeMeshVertexTangentBasisType::HighPrecision, ERuntimeMeshVertexUVType::HighPrecision, RUNTIMEMESHCOMPONENT_API)

/** Simple vertex with 2 UV channels */
__DECLARE_RUNTIME_MESH_VERTEXINTERNAL(FRuntimeMeshVertexDualUVHiPrecisionNormals, true, true, true, true, 2, ERuntimeMeshVertexTangentBasisType::HighPrecision, ERuntimeMeshVertexUVType::HighPrecision, RUNTIMEMESHCOMPONENT_API)

/** Simple vertex with 1 UV channel and NO position component (Meant to be used with separate position buffer) */
__DECLARE_RUNTIME_MESH_VERTEXINTERNAL(FRuntimeMeshVertexNoPositionHiPrecisionNormals, false, true, true, true, 1, ERuntimeMeshVertexTangentBasisType::HighPrecision, ERuntimeMeshVertexUVType::HighPrecision, RUNTIMEMESHCOMPONENT_API)

/** Simple vertex with 2 UV channels and NO position component (Meant to be used with separate position buffer) */
__DECLARE_RUNTIME_MESH_VERTEXINTERNAL(FRuntimeMeshVertexNoPositionDualUVHiPrecisionNormals, false, true, true, true, 2, ERuntimeMeshVertexTangentBasisType::HighPrecision, ERuntimeMeshVertexUVType::HighPrecision, RUNTIMEMESHCOMPONENT_API)





struct FRuntimeMeshTangents
{
	FPackedNormal Tangent;
	FPackedNormal Normal;
};

struct FRuntimeMeshTangentsHighPrecision
{
	FPackedRGBA16N Tangent;
	FPackedRGBA16N Normal;
};

struct FRuntimeMeshDualUV
{
	FVector2DHalf UV0;
	FVector2DHalf UV1;

	FVector2DHalf& operator[] (int32 Index) 
	{
		return Index > 0 ? UV1 : UV0;
	}
};
struct FRuntimeMeshDualUVHighPrecision
{
	FVector2D UV0;
	FVector2D UV1;

	FVector2D& operator[] (int32 Index)
	{
		return Index > 0 ? UV1 : UV0;
	}
};


template<typename VertexType>
inline bool GetTangentIsHighPrecision()
{
	static_assert(sizeof(VertexType) == -1, "Invalid Tangent type.");
}

template<>
inline bool GetTangentIsHighPrecision<FRuntimeMeshTangents>()
{
	return false;
}

template<>
inline bool GetTangentIsHighPrecision<FRuntimeMeshTangentsHighPrecision>()
{
	return true;
}

template<typename VertexType>
inline void GetUVVertexProperties(bool& bIsUsingHighPrecision, int32& NumUVs)
{
	static_assert(sizeof(VertexType) == -1, "Invalid UV type.");
}

template<>
inline void GetUVVertexProperties<FVector2DHalf>(bool& bIsUsingHighPrecision, int32& NumUVs)
{
	bIsUsingHighPrecision = false;
	NumUVs = 1;
}

template<>
inline void GetUVVertexProperties<FVector2D>(bool& bIsUsingHighPrecision, int32& NumUVs)
{
	bIsUsingHighPrecision = true;
	NumUVs = 1;
}

template<>
inline void GetUVVertexProperties<FRuntimeMeshDualUV>(bool& bIsUsingHighPrecision, int32& NumUVs)
{
	bIsUsingHighPrecision = false;
	NumUVs = 2;
}

template<>
inline void GetUVVertexProperties<FRuntimeMeshDualUVHighPrecision>(bool& bIsUsingHighPrecision, int32& NumUVs)
{
	bIsUsingHighPrecision = true;
	NumUVs = 2;
}



