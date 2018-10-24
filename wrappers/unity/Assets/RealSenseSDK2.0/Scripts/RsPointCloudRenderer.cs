using System;
using UnityEngine;
using Intel.RealSense;
using UnityEngine.Rendering;
using UnityEngine.Assertions;
using System.Runtime.InteropServices;
using System.Threading;
using System.Collections.Generic;
using System.Linq;

[RequireComponent(typeof(MeshFilter), typeof(MeshRenderer))]
public class RsPointCloudRenderer : MonoBehaviour
{
    public RsFrameProvider Source;
    private Mesh mesh;
    private Texture2D uvmap;

    [NonSerialized]
    private Vector3[] vertices;
    private GCHandle handle;
    private IntPtr verticesPtr;
    // private int frameSize;
    // private IntPtr frameData;

    FrameQueue q;

    void Start()
    {
        Source.OnStart += OnStartStreaming;
        Source.OnStop += Dispose;
    }

    private void OnStartStreaming(PipelineProfile obj)
    {
        q = new FrameQueue(1);

        using (var depth = obj.Streams.FirstOrDefault(s => s.Stream == Stream.Depth) as VideoStreamProfile)
            ResetMesh(depth.Width, depth.Height);

        Source.OnNewSample += OnNewSample;
    }

    private void ResetMesh(int width, int height)
    {
        Assert.IsTrue(SystemInfo.SupportsTextureFormat(TextureFormat.RGFloat));
        uvmap = new Texture2D(width, height, TextureFormat.RGFloat, false, true)
        {
            wrapMode = TextureWrapMode.Clamp,
            filterMode = FilterMode.Point,
        };
        GetComponent<MeshRenderer>().sharedMaterial.SetTexture("_UVMap", uvmap);

        if (mesh != null)
            mesh.Clear();
        else
            mesh = new Mesh()
            {
                indexFormat = IndexFormat.UInt32,
            };

        vertices = new Vector3[width * height];
        if (handle.IsAllocated)
            handle.Free();
        handle = GCHandle.Alloc(vertices, GCHandleType.Pinned);
        verticesPtr = handle.AddrOfPinnedObject();

        var indices = new int[vertices.Length];
        for (int i = 0; i < vertices.Length; i++)
            indices[i] = i;

        mesh.MarkDynamic();
        mesh.vertices = vertices;

        var uvs = new Vector2[width * height];
        Array.Clear(uvs, 0, uvs.Length);
        for (int j = 0; j < height; j++)
        {
            for (int i = 0; i < width; i++)
            {
                uvs[i + j * width].x = i / (float)width;
                uvs[i + j * width].y = j / (float)height;
            }
        }

        mesh.uv = uvs;

        mesh.SetIndices(indices, MeshTopology.Points, 0, false);
        mesh.bounds = new Bounds(Vector3.zero, Vector3.one * 10f);

        GetComponent<MeshFilter>().sharedMesh = mesh;
    }

    void OnDestroy()
    {
        if (q != null)
        {
            q.Dispose();
            q = null;
        }

        if (handle.IsAllocated)
            handle.Free();

        if (mesh != null)
            Destroy(null);
    }

    private void Dispose()
    {
        Source.OnNewSample -= OnNewSample;

        if (q != null)
        {
            q.Dispose();
            q = null;
        }

        if (handle.IsAllocated)
            handle.Free();
    }

    private Points TryGetPoints(FrameSet frameset)
    {
        foreach (var f in frameset)
        {
            if (f is Points)
                return f as Points;
            f.Dispose();
        }
        return null;
    }

    private void OnNewSample(Frame frame)
    {
        try
        {
            if (frame.IsComposite)
            {
                using (var fs = FrameSet.FromFrame(frame))
                using (var points = TryGetPoints(fs))
                {
                    if (points != null)
                    {
                        q.Enqueue(points);
                    }
                }
                return;
            }

            if (frame is Points)
            {
                q.Enqueue(frame);
            }
        }
        catch (Exception e)
        {
            Debug.LogException(e);
        }
    }


    protected void Update()
    {
        if (q != null)
        {
            Frame f;
            if (!q.PollForFrame(out f))
                return;

            using (var points = f as Points)
            {
                if (points.Count != mesh.vertexCount)
                {
                    using (var p = f.Profile as VideoStreamProfile)
                        ResetMesh(p.Width, p.Height);
                }

                int s = points.Count * sizeof(float);

                if (points.TextureData != IntPtr.Zero)
                {
                    uvmap.LoadRawTextureData(points.TextureData, s * 2);
                    uvmap.Apply();
                }

                if (points.VertexData != IntPtr.Zero)
                {
                    memcpy(verticesPtr, points.VertexData, s * 3);

                    mesh.vertices = vertices;
                    mesh.UploadMeshData(false);
                }
            }
        }
    }

    [DllImport("msvcrt.dll", EntryPoint = "memcpy", CallingConvention = CallingConvention.Cdecl, SetLastError = false)]
    internal static extern IntPtr memcpy(IntPtr dest, IntPtr src, int count);
}
