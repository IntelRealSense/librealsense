using System;
using UnityEngine;
using Intel.RealSense;
using UnityEngine.Rendering;
using UnityEngine.Assertions;
using System.Runtime.InteropServices;
using System.Threading;
using System.Collections.Generic;

[RequireComponent(typeof(MeshFilter), typeof(MeshRenderer))]
public class RsPointCloudRenderer : MonoBehaviour
{
    private Mesh mesh;
    private Texture2D uvmap;

    private RsVideoStreamRequest _videoStreamFilter;
    private RsVideoStreamRequest _currVideoStreamFilter;

    private Vector3[] vertices;
    private GCHandle handle;
    private IntPtr verticesPtr;
    private int frameSize;
    private IntPtr frameData;

    private readonly AutoResetEvent e = new AutoResetEvent(false);

    void Start()
    {
        _videoStreamFilter = new RsVideoStreamRequest();
        _currVideoStreamFilter = _videoStreamFilter.Clone();
        RsDevice.Instance.OnNewSampleSet += OnFrames;
        RsDevice.Instance.OnStop += Dispose;
    }

    private void ResetMesh(RsVideoStreamRequest vsr)
    {
        int width = vsr.Width;
        int height = vsr.Height;
        if (width == 0 || height == 0)
            return;
        Dispose();
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
        _currVideoStreamFilter = vsr.Clone();
    }

    void OnDestroy()
    {
        Dispose();

        if(mesh != null)
            Destroy(null);
    }

    private void Dispose()
    {
        e.Reset();

        if (handle.IsAllocated)
            handle.Free();

        if (frameData != IntPtr.Zero)
        {
            Marshal.FreeHGlobal(frameData);
            frameData = IntPtr.Zero;
        }
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

    private void OnFrames(FrameSet frames)
    {
        using (var points = TryGetPoints(frames))
        {
            if (points == null)
                return;

            using (var depth = frames.DepthFrame)
            {
                if (_currVideoStreamFilter.Width != depth.Width || _currVideoStreamFilter.Height != depth.Height)
                {
                    _videoStreamFilter.Width = depth.Width;
                    _videoStreamFilter.Height = depth.Height;
                    return;
                }
            }

            memcpy(verticesPtr, points.VertexData, points.Count * 3 * sizeof(float));

            frameSize = points.Count * 2 * sizeof(float);
            if (frameData == IntPtr.Zero)
                frameData = Marshal.AllocHGlobal(frameSize);
            memcpy(frameData, points.TextureData, frameSize);

            e.Set();
        }
    }

    protected void Update()
    {
        if (!_videoStreamFilter.Equals(_currVideoStreamFilter))
            ResetMesh(_videoStreamFilter);

        if (e.WaitOne(0))
        {
            uvmap.LoadRawTextureData(frameData, frameSize);
            uvmap.Apply();

            mesh.vertices = vertices;
            mesh.UploadMeshData(false);
        }
    }
    
    [DllImport("msvcrt.dll", EntryPoint = "memcpy", CallingConvention = CallingConvention.Cdecl, SetLastError = false)]
    internal static extern IntPtr memcpy(IntPtr dest, IntPtr src, int count);
}
