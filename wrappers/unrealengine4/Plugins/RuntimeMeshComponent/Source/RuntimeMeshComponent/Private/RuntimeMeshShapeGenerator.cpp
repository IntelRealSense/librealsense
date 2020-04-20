// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

#include "RuntimeMeshShapeGenerator.h"
#include "RuntimeMeshComponentPlugin.h"



void URuntimeMeshShapeGenerator::ConvertQuadToTriangles(TArray<int32>& Triangles, int32 Vert0, int32 Vert1, int32 Vert2, int32 Vert3)
{
	Triangles.Add(Vert0);
	Triangles.Add(Vert1);
	Triangles.Add(Vert3);

	Triangles.Add(Vert1);
	Triangles.Add(Vert2);
	Triangles.Add(Vert3);
}


#define CREATEBOX_NUMVERTS 24
#define CREATEBOX_NUMTRIS 36
#define CREATEGRIDMESHTRIANGLES_NUMVERTS(NumX, NumY) (((NumX) + 1) * ((NumY) + 1))
#define CREATEGRIDMESHTRIANGLES_NUMTRIS(NumX, NumY) ((NumX) * (NumY) * 2 * 3)



void URuntimeMeshShapeGenerator::CreateBoxMesh(FVector BoxRadius, TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FRuntimeMeshTangent>& Tangents)
{
	Vertices.Empty(CREATEBOX_NUMVERTS);
	Triangles.Empty(CREATEBOX_NUMTRIS);
	Normals.Empty(CREATEBOX_NUMVERTS);
	UVs.Empty(CREATEBOX_NUMVERTS);
	Tangents.Empty(CREATEBOX_NUMVERTS);

	FVerticesBuilderFunction VerticesBuilder = [&](const FVector& Position, const FVector& Normal, const FRuntimeMeshTangent& Tangent, const FVector2D& UV0)
	{
		Vertices.Add(Position);
		Normals.Add(Normal);
		Tangents.Add(Tangent);
		UVs.Add(UV0);
	};

	FTrianglesBuilderFunction TrianglesBuilder = [&](int32 Index)
	{
		Triangles.Add(Index);
	};

	CreateBoxMesh(BoxRadius, VerticesBuilder, TrianglesBuilder);
}

void URuntimeMeshShapeGenerator::CreateBoxMeshPacked(FVector BoxRadius, TArray<FRuntimeMeshBlueprintVertexSimple>& Vertices, TArray<int32>& Triangles)
{
	Vertices.Empty(CREATEBOX_NUMVERTS);
	Triangles.Empty(CREATEBOX_NUMTRIS);

	FVerticesBuilderFunction VerticesBuilder = [&](const FVector& Position, const FVector& Normal, const FRuntimeMeshTangent& Tangent, const FVector2D& UV0)
	{
		Vertices.Add(FRuntimeMeshBlueprintVertexSimple(Position, Normal, Tangent, UV0));
	};

	FTrianglesBuilderFunction TrianglesBuilder = [&](int32 Index)
	{
		Triangles.Add(Index);
	};

	CreateBoxMesh(BoxRadius, VerticesBuilder, TrianglesBuilder);
}

void URuntimeMeshShapeGenerator::CreateBoxMesh(FVector BoxRadius, TArray<FRuntimeMeshVertexSimple>& Vertices, TArray<int32>& Triangles)
{
	Vertices.Empty(CREATEBOX_NUMVERTS);
	Triangles.Empty(CREATEBOX_NUMTRIS);

	FVerticesBuilderFunction VerticesBuilder = [&](const FVector& Position, const FVector& Normal, const FRuntimeMeshTangent& Tangent, const FVector2D& UV0)
	{
		Vertices.Add(FRuntimeMeshVertexSimple(Position, Normal, Tangent, UV0));
	};

	FTrianglesBuilderFunction TrianglesBuilder = [&](int32 Index)
	{
		Triangles.Add(Index);
	};

	CreateBoxMesh(BoxRadius, VerticesBuilder, TrianglesBuilder);
}

void URuntimeMeshShapeGenerator::CreateBoxMesh(FVector BoxRadius, const TSharedPtr<FRuntimeMeshAccessor>& MeshBuilder)
{
	MeshBuilder->EmptyVertices(CREATEBOX_NUMVERTS);
	MeshBuilder->EmptyIndices(CREATEBOX_NUMTRIS);
	FVerticesBuilderFunction VerticesBuilder = [&](const FVector& Position, const FVector& Normal, const FRuntimeMeshTangent& Tangent, const FVector2D& UV0)
	{
		int32 NewVertex = MeshBuilder->AddVertex(Position);		
		MeshBuilder->SetNormalTangent(NewVertex, Normal, Tangent);
		MeshBuilder->SetUV(NewVertex, UV0);
	};

	FTrianglesBuilderFunction TrianglesBuilder = [&](int32 Index)
	{
		MeshBuilder->AddIndex(Index);
	};

	CreateBoxMesh(BoxRadius, VerticesBuilder, TrianglesBuilder);
}

void URuntimeMeshShapeGenerator::CreateBoxMesh(FVector BoxRadius, FRuntimeMeshAccessor& MeshBuilder)
{
	MeshBuilder.EmptyVertices(CREATEBOX_NUMVERTS);
	MeshBuilder.EmptyIndices(CREATEBOX_NUMTRIS);
	FVerticesBuilderFunction VerticesBuilder = [&](const FVector& Position, const FVector& Normal, const FRuntimeMeshTangent& Tangent, const FVector2D& UV0)
	{
		int32 NewVertex = MeshBuilder.AddVertex(Position);
		MeshBuilder.SetNormalTangent(NewVertex, Normal, Tangent);
		MeshBuilder.SetUV(NewVertex, UV0);
	};

	FTrianglesBuilderFunction TrianglesBuilder = [&](int32 Index)
	{
		MeshBuilder.AddIndex(Index);
	};

	CreateBoxMesh(BoxRadius, VerticesBuilder, TrianglesBuilder);
}


void URuntimeMeshShapeGenerator::CreateGridMeshTriangles(int32 NumX, int32 NumY, bool bWinding, TArray<int32>& Triangles)
{
	Triangles.Reset(CREATEGRIDMESHTRIANGLES_NUMTRIS(NumX - 1, NumY - 1));

	FTrianglesBuilderFunction TrianglesBuilder = [&](int32 Index)
	{
		Triangles.Add(Index);
	};

	CreateGridMeshTriangles(NumX, NumY, bWinding, TrianglesBuilder);
}



void URuntimeMeshShapeGenerator::CreateGridMesh(float Width, float Height, int32 NumSubdivisionsX, int32 NumSubdivisionsY, TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FRuntimeMeshTangent>& Tangents)
{
	Vertices.Empty(CREATEGRIDMESHTRIANGLES_NUMVERTS(NumSubdivisionsX, NumSubdivisionsY));
	Triangles.Empty(CREATEGRIDMESHTRIANGLES_NUMTRIS(NumSubdivisionsX, NumSubdivisionsY));
	Normals.Empty(CREATEGRIDMESHTRIANGLES_NUMVERTS(NumSubdivisionsX, NumSubdivisionsY));
	UVs.Empty(CREATEGRIDMESHTRIANGLES_NUMVERTS(NumSubdivisionsX, NumSubdivisionsY));
	Tangents.Empty(CREATEGRIDMESHTRIANGLES_NUMVERTS(NumSubdivisionsX, NumSubdivisionsY));

	FVerticesBuilderFunction VerticesBuilder = [&](const FVector& Position, const FVector& Normal, const FRuntimeMeshTangent& Tangent, const FVector2D& UV0)
	{
		Vertices.Add(Position);
		Normals.Add(Normal);
		Tangents.Add(Tangent);
		UVs.Add(UV0);
	};

	FTrianglesBuilderFunction TrianglesBuilder = [&](int32 Index)
	{
		Triangles.Add(Index);
	};

	CreateGridMesh(Width, Height, NumSubdivisionsX, NumSubdivisionsY, VerticesBuilder, TrianglesBuilder);
}

void URuntimeMeshShapeGenerator::CreateGridMeshPacked(float Width, float Height, int32 NumSubdivisionsX, int32 NumSubdivisionsY, TArray<FRuntimeMeshBlueprintVertexSimple>& Vertices, TArray<int32>& Triangles)
{
	Vertices.Empty(CREATEGRIDMESHTRIANGLES_NUMVERTS(NumSubdivisionsX, NumSubdivisionsY));
	Triangles.Empty(CREATEGRIDMESHTRIANGLES_NUMTRIS(NumSubdivisionsX, NumSubdivisionsY));

	FVerticesBuilderFunction VerticesBuilder = [&](const FVector& Position, const FVector& Normal, const FRuntimeMeshTangent& Tangent, const FVector2D& UV0)
	{
		Vertices.Add(FRuntimeMeshBlueprintVertexSimple(Position, Normal, Tangent, UV0));
	};

	FTrianglesBuilderFunction TrianglesBuilder = [&](int32 Index)
	{
		Triangles.Add(Index);
	};

	CreateGridMesh(Width, Height, NumSubdivisionsX, NumSubdivisionsY, VerticesBuilder, TrianglesBuilder);
}

void URuntimeMeshShapeGenerator::CreateGridMesh(float Width, float Height, int32 NumSubdivisionsX, int32 NumSubdivisionsY, TArray<FRuntimeMeshVertexSimple>& Vertices, TArray<int32>& Triangles)
{
	Vertices.Empty(CREATEGRIDMESHTRIANGLES_NUMVERTS(NumSubdivisionsX, NumSubdivisionsY));
	Triangles.Empty(CREATEGRIDMESHTRIANGLES_NUMTRIS(NumSubdivisionsX, NumSubdivisionsY));

	FVerticesBuilderFunction VerticesBuilder = [&](const FVector& Position, const FVector& Normal, const FRuntimeMeshTangent& Tangent, const FVector2D& UV0)
	{
		Vertices.Add(FRuntimeMeshVertexSimple(Position, Normal, Tangent, UV0));
	};

	FTrianglesBuilderFunction TrianglesBuilder = [&](int32 Index)
	{
		Triangles.Add(Index);
	};

	CreateGridMesh(Width, Height, NumSubdivisionsX, NumSubdivisionsY, VerticesBuilder, TrianglesBuilder);
}

void URuntimeMeshShapeGenerator::CreateGridMesh(float Width, float Height, int32 NumSubdivisionsX, int32 NumSubdivisionsY, const TSharedPtr<FRuntimeMeshAccessor>& MeshBuilder)
{
	MeshBuilder->EmptyVertices(CREATEGRIDMESHTRIANGLES_NUMVERTS(NumSubdivisionsX, NumSubdivisionsY));
	MeshBuilder->EmptyIndices(CREATEGRIDMESHTRIANGLES_NUMTRIS(NumSubdivisionsX, NumSubdivisionsY));
	FVerticesBuilderFunction VerticesBuilder = [&](const FVector& Position, const FVector& Normal, const FRuntimeMeshTangent& Tangent, const FVector2D& UV0)
	{
		int32 NewVertex = MeshBuilder->AddVertex(Position);
		MeshBuilder->SetNormalTangent(NewVertex, Normal, Tangent);
		MeshBuilder->SetUV(NewVertex, UV0);
	};

	FTrianglesBuilderFunction TrianglesBuilder = [&](int32 Index)
	{
		MeshBuilder->AddIndex(Index);
	};

	CreateGridMesh(Width, Height, NumSubdivisionsX, NumSubdivisionsY, VerticesBuilder, TrianglesBuilder);
}

void URuntimeMeshShapeGenerator::ConvertQuadToTriangles(TFunction<void(int32 Index)> TrianglesBuilder, int32 Vert0, int32 Vert1, int32 Vert2, int32 Vert3)
{
	TrianglesBuilder(Vert0);
	TrianglesBuilder(Vert1);
	TrianglesBuilder(Vert3);

	TrianglesBuilder(Vert1);
	TrianglesBuilder(Vert2);
	TrianglesBuilder(Vert3);
}

void URuntimeMeshShapeGenerator::CreateBoxMesh(FVector BoxRadius, FVerticesBuilderFunction VerticesBuilder, FTrianglesBuilderFunction TrianglesBuilder)
{
	// Generate verts
	FVector BoxVerts[8];
	BoxVerts[0] = FVector(-BoxRadius.X, BoxRadius.Y, BoxRadius.Z);
	BoxVerts[1] = FVector(BoxRadius.X, BoxRadius.Y, BoxRadius.Z);
	BoxVerts[2] = FVector(BoxRadius.X, -BoxRadius.Y, BoxRadius.Z);
	BoxVerts[3] = FVector(-BoxRadius.X, -BoxRadius.Y, BoxRadius.Z);

	BoxVerts[4] = FVector(-BoxRadius.X, BoxRadius.Y, -BoxRadius.Z);
	BoxVerts[5] = FVector(BoxRadius.X, BoxRadius.Y, -BoxRadius.Z);
	BoxVerts[6] = FVector(BoxRadius.X, -BoxRadius.Y, -BoxRadius.Z);
	BoxVerts[7] = FVector(-BoxRadius.X, -BoxRadius.Y, -BoxRadius.Z);

	FVector Normal;
	FRuntimeMeshTangent Tangent;

	// Pos Z
	Normal = FVector(0.0f, 0.0f, 1.0f);
	Tangent.TangentX = FVector(0.0f, -1.0f, 0.0f);
	VerticesBuilder(BoxVerts[0], Normal, Tangent, FVector2D(0.0f, 0.0f));
	VerticesBuilder(BoxVerts[1], Normal, Tangent, FVector2D(0.0f, 1.0f));
	VerticesBuilder(BoxVerts[2], Normal, Tangent, FVector2D(1.0f, 1.0f));
	VerticesBuilder(BoxVerts[3], Normal, Tangent, FVector2D(1.0f, 0.0f));
	ConvertQuadToTriangles(TrianglesBuilder, 0, 1, 2, 3);

	// Neg X
	Normal = FVector(-1.0f, 0.0f, 0.0f);
	Tangent.TangentX = FVector(0.0f, -1.0f, 0.0f);
	VerticesBuilder(BoxVerts[4], Normal, Tangent, FVector2D(0.0f, 0.0f));
	VerticesBuilder(BoxVerts[0], Normal, Tangent, FVector2D(0.0f, 1.0f));
	VerticesBuilder(BoxVerts[3], Normal, Tangent, FVector2D(1.0f, 1.0f));
	VerticesBuilder(BoxVerts[7], Normal, Tangent, FVector2D(1.0f, 0.0f));
	ConvertQuadToTriangles(TrianglesBuilder, 4, 5, 6, 7);

	// Pos Y
	Normal = FVector(0.0f, 1.0f, 0.0f);
	Tangent.TangentX = FVector(-1.0f, 0.0f, 0.0f);
	VerticesBuilder(BoxVerts[5], Normal, Tangent, FVector2D(0.0f, 0.0f));
	VerticesBuilder(BoxVerts[1], Normal, Tangent, FVector2D(0.0f, 1.0f));
	VerticesBuilder(BoxVerts[0], Normal, Tangent, FVector2D(1.0f, 1.0f));
	VerticesBuilder(BoxVerts[4], Normal, Tangent, FVector2D(1.0f, 0.0f));
	ConvertQuadToTriangles(TrianglesBuilder, 8, 9, 10, 11);

	// Pos X
	Normal = FVector(1.0f, 0.0f, 0.0f);
	Tangent.TangentX = FVector(0.0f, 1.0f, 0.0f);
	VerticesBuilder(BoxVerts[6], Normal, Tangent, FVector2D(0.0f, 0.0f));
	VerticesBuilder(BoxVerts[2], Normal, Tangent, FVector2D(0.0f, 1.0f));
	VerticesBuilder(BoxVerts[1], Normal, Tangent, FVector2D(1.0f, 1.0f));
	VerticesBuilder(BoxVerts[5], Normal, Tangent, FVector2D(1.0f, 0.0f));
	ConvertQuadToTriangles(TrianglesBuilder, 12, 13, 14, 15);

	// Neg Y
	Normal = FVector(0.0f, -1.0f, 0.0f);
	Tangent.TangentX = FVector(1.0f, 0.0f, 0.0f);
	VerticesBuilder(BoxVerts[7], Normal, Tangent, FVector2D(0.0f, 0.0f));
	VerticesBuilder(BoxVerts[3], Normal, Tangent, FVector2D(0.0f, 1.0f));
	VerticesBuilder(BoxVerts[2], Normal, Tangent, FVector2D(1.0f, 1.0f));
	VerticesBuilder(BoxVerts[6], Normal, Tangent, FVector2D(1.0f, 0.0f));
	ConvertQuadToTriangles(TrianglesBuilder, 16, 17, 18, 19);

	// Neg Z
	Normal = FVector(0.0f, 0.0f, -1.0f);
	Tangent.TangentX = FVector(0.0f, 1.0f, 0.0f);
	VerticesBuilder(BoxVerts[7], Normal, Tangent, FVector2D(0.0f, 0.0f));
	VerticesBuilder(BoxVerts[6], Normal, Tangent, FVector2D(0.0f, 1.0f));
	VerticesBuilder(BoxVerts[5], Normal, Tangent, FVector2D(1.0f, 1.0f));
	VerticesBuilder(BoxVerts[4], Normal, Tangent, FVector2D(1.0f, 0.0f));
	ConvertQuadToTriangles(TrianglesBuilder, 20, 21, 22, 23);
}

void URuntimeMeshShapeGenerator::CreateGridMeshTriangles(int32 NumX, int32 NumY, bool bWinding, FTrianglesBuilderFunction TrianglesBuilder)
{
	if (NumX >= 2 && NumY >= 2)
	{
		// Build Quads
		for (int XIdx = 0; XIdx < NumX - 1; XIdx++)
		{
			for (int YIdx = 0; YIdx < NumY - 1; YIdx++)
			{
				const int32 I0 = (XIdx + 0)*NumY + (YIdx + 0);
				const int32 I1 = (XIdx + 1)*NumY + (YIdx + 0);
				const int32 I2 = (XIdx + 1)*NumY + (YIdx + 1);
				const int32 I3 = (XIdx + 0)*NumY + (YIdx + 1);

				if (bWinding)
				{
					ConvertQuadToTriangles(TrianglesBuilder, I0, I1, I2, I3);
				}
				else
				{
					ConvertQuadToTriangles(TrianglesBuilder, I0, I3, I2, I1);
				}
			}
		}
	}
}

void URuntimeMeshShapeGenerator::CreateGridMesh(float Width, float Height, int32 NumSubdivisionsX, int32 NumSubdivisionsY, FVerticesBuilderFunction VerticesBuilder, FTrianglesBuilderFunction TrianglesBuilder)
{
	static const FVector Normal(0.0f, 0.0f, 1.0f);
	static const FVector Tangent(0.0f, -1.0f, 0.0f);


	const float HalfX = Width * 0.5f;
	const float HalfY = Height * 0.5f;

	int32 NumVerticesX = NumSubdivisionsX + 1;
	int32 NumVerticesY = NumSubdivisionsY + 1;

	for (int32 X = 0; X < NumVerticesX; X++)
	{
		for (int32 Y = 0; Y < NumVerticesY; Y++)
		{
			FVector2D Position;
			Position.X = -HalfX + ((X / (float)NumSubdivisionsX) * Width);
			Position.Y = -HalfY + ((Y / (float)NumSubdivisionsY) * Height);

			FVector2D UV;
			UV.X = X / (float)NumSubdivisionsX;
			UV.Y = Y / (float)NumSubdivisionsY;

			VerticesBuilder(FVector(Position, 0.0f), Normal, Tangent, UV);
		}
	}

	CreateGridMeshTriangles(NumVerticesX, NumVerticesY, false, TrianglesBuilder);
}


