// Copyright 2018-2019 Gabriel Zerbib (Moddingear). All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Using a single struct makes everything nicer
 * If you already have something similar in your project, you can use an operator to convert it in place when you use the mesh manager :
 *	operator FRuntimeMeshDataStruct<FVertexNoxel, int32>() {
 *		return FRuntimeMeshDataStruct<FVertexNoxel, int32>(Vertices, Triangles);
 *	}
 * (put this in your struct)
 * or you could even directly inherit from this one
 */
template<typename VertexType, typename TriangleType>
struct /*RUNTIMEMESHCOMPONENT_API*/ FRuntimeMeshDataStruct //Do not uncomment the API, otherwise you'll get unresolved externals
{
	TArray<VertexType> Vertices;
	TArray<TriangleType> Triangles;

	FRuntimeMeshDataStruct() {}

	FRuntimeMeshDataStruct(TArray<VertexType> InVertices, TArray<TriangleType> InTriangles) {
		Vertices = InVertices;
		Triangles = InTriangles;
	}

	FRuntimeMeshDataStruct<VertexType, TriangleType> FlipNormals() {
		FRuntimeMeshDataStruct<VertexType, TriangleType> newmesh = FRuntimeMeshDataStruct(Vertices, Triangles);
		for (int32 i = 0; i < Triangles.Num(); i += 3) {
			newmesh.Triangles[i] = Triangles[i + 1];
			newmesh.Triangles[i + 1] = Triangles[i];
		}
		return newmesh;
	}
};

template<typename Datastruct, typename VertexType, typename TriangleType>
class /*RUNTIMEMESHCOMPONENT_API*/ FRuntimeMeshManager
{
private:

	typedef FRuntimeMeshDataStruct<VertexType, TriangleType> MeshType;

	FRuntimeMeshDataStruct<VertexType, TriangleType> Mesh;

	TArray<Datastruct> DataMap;

	TArray<int32> VertLengths;

	TArray<int32> TrisLengths;

public:

	/* Used to know if a mesh is already present in the mesh manager */
	FORCEINLINE bool HasMesh(Datastruct id) 
	{
		return DataMap.contains(id);
	}

	/* Returns the full mesh that combines all provided meshes */
	FORCEINLINE MeshType GetMesh()
	{
		return Mesh;
	}

	/* Returns the internal array of the saved meshes' data, in correct order 
	Only useful if you want to iterate on the meshes */
	FORCEINLINE TArray<Datastruct> GetMeshData()
	{
		return DataMap;
	}

	/* Returns the mesh that was saved at the ID */
	FORCEINLINE MeshType GetMesh(Datastruct id)
	{
		int32 index = DataMap.find(id);
		return GetMeshAtIndex(index);
	}
	
	/* Returns the mesh that was saved at the index
	Only useful when iterating */
	FORCEINLINE MeshType GetMeshAtIndex(int32 index)
	{
		MeshType submesh;
		submesh.Vertices.SetNum(VertLengths[index]);
		submesh.Triangles.SetNum(TrisLengths[index]);
		int32 vertstart, trisstart;
		getStart(index, vertstart, trisstart);
		for (int32 i = 0; i < VertLengths[index]; i++)
		{
			submesh.Vertices[i] = Mesh.Vertices[i + vertstart];
		}
		for (int32 i = 0; i < TrisLengths[index]; i++)
		{
			submesh.Triangles[i] = Mesh.Triangles[i + trisstart];
		}
	}

	/* Adds or updates the mesh for the specified ID
	Giving a mesh with an ID that is already registered will cause it to update 
	Use the same vert and tris length to avoid recreating the array */
	FORCEINLINE void AddMesh(Datastruct id, MeshType mesh)
	{
		if (DataMap.Contains(id)) {
			/*int32 index = DataMap.Find(id);
			if (mesh.Vertices.Num() == VertLengths[index] && mesh.Triangles.Num() == TrisLengths[index]) {
				//If the lengths match, update the arrays only
				int32 VertStart, TrisStart;
				getStart(index, VertStart, TrisStart);
				for (int i = 0; i < mesh.Vertices.Num(); i++)
				{
					Mesh.Vertices[i + VertStart] = mesh.Vertices[i];
				}
				for (int i = 0; i < mesh.Triangles.Num(); i++)
				{
					Mesh.Triangles[i + TrisStart] = mesh.Triangles[i] + VertStart;
				}
				return; //stop the execution
			}
			else {*/
				RemoveMesh(id);
			//}
		}

		
		int32 vertstart = Mesh.Vertices.Num(), vertlen = mesh.Vertices.Num(), trisstart = Mesh.Triangles.Num(), trislen = mesh.Triangles.Num();

		DataMap.Add(id); //Add panel and mesh properties to the map
		VertLengths.Add(vertlen);
		TrisLengths.Add(trislen);

		Mesh.Vertices.Append(mesh.Vertices); //Modify the mesh
		for (int i = 0; i < trislen; i++)
		{
			Mesh.Triangles.Add(vertstart + mesh.Triangles[i]);
		}
	};

	/* Removes the submesh that was linked to the supplied id */
	FORCEINLINE void RemoveMesh(Datastruct id) 
	{
		if (!DataMap.Contains(id)) {
			return;
		}
		int index = DataMap.Find(id);
		int32 vertstart = 0, vertlen = VertLengths[index], trisstart = 0, trislen = TrisLengths[index];

		getStart(index, vertstart, trisstart); // Find the start of the saved data

		//Remove tris first to avoid updating the indices that will be removed
		removeTris(trisstart, trislen); //Delete tris
		removeVerts(vertstart, vertlen, trisstart); //Delete vertices

		//Data
		DataMap.RemoveAt(index);
		VertLengths.RemoveAt(index);
		TrisLengths.RemoveAt(index);
	};

private:

	FORCEINLINE void getStart(int32 index, int32& VertStart, int32& TrisStart) 
	{
		VertStart = 0; TrisStart = 0;

		for (int i = 0; i < index; i++)
		{
			VertStart += VertLengths[i];
			TrisStart += TrisLengths[i];
		}
	}

	/* Removes the verts and shifts the triangles references */
	FORCEINLINE void removeVerts(int32 start, int32 len, int32 trisStart) 
	{
		// Since meshes don't refer to other meshes, we update only the last since we are sure that only the last have indices bigger than those removed
		for (int i = trisStart; i < Mesh.Triangles.Num(); i++)
		{
			Mesh.Triangles[i] -= len;
		}
		Mesh.Vertices.RemoveAt(start, len, true);
	}

	FORCEINLINE void removeTris(int32 start, int32 len) 
	{
		Mesh.Triangles.RemoveAt(start, len, true);
	}

public:
	/* Retrieves the ID of the submesh that was hit 
	Takes the tris number, not the index of the tris (it doesn't need to be multiplied by 3, just feed the collision data)*/
	FORCEINLINE Datastruct GetMeshHit(TriangleType tris) 
	{
		int32 trisnum = 0;
		for (int i = 0; i < DataMap.Num(); i++)
		{
			trisnum += TrisLengths[i];
			if (tris * 3 < trisnum) {
				return DataMap[i];
			}
		}
		return Datastruct();
	};

	/* Returns the internal mesh index of the mesh that was hit */
	FORCEINLINE int32 GetMeshIndexHit(TriangleType tris)
	{
		int32 trisnum = 0;
		for (int i = 0; i < DataMap.Num(); i++)
		{
			trisnum += TrisLengths[i];
			if (tris * 3 < trisnum) {
				return i;
			}
		}
		return INDEX_NONE;
	};
};
