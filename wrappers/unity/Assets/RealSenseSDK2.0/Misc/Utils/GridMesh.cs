using System.Collections;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;

[RequireComponent(typeof(MeshFilter))]
public class GridMesh : MonoBehaviour
{
    public int Divisions = 10;
    public float Size = 4f;
    private Mesh mesh;

    // Use this for initialization
    void Start()
    {
        mesh = new Mesh();
        GetComponent<MeshFilter>().sharedMesh = mesh;

        float sz = 0.5f * Size;
        var verts = new List<Vector3>(Divisions);
        for (int i = 0; i <= Divisions; i++)
        {
            float t = Size * i / Divisions - sz;
            verts.Add(new Vector3(t, 0, -sz));
            verts.Add(new Vector3(t, 0, sz));
            verts.Add(new Vector3(-sz, 0, t));
            verts.Add(new Vector3(sz, 0, t));
        }
        mesh.SetVertices(verts);
        mesh.SetIndices(Enumerable.Range(0, verts.Count).ToArray(), MeshTopology.Lines, 0);
    }

    void OnDestroy()
    {
        if (mesh != null)
            Destroy(mesh);
    }
}
