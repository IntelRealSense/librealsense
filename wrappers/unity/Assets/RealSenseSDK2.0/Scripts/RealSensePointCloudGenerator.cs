using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Intel.RealSense;
using UnityEngine.Rendering;
using UnityEngine.Assertions;
using System.Runtime.InteropServices;
using System.Threading;

[RequireComponent(typeof(MeshFilter), typeof(MeshRenderer))]
public class RealSensePointCloudGenerator : MonoBehaviour
{
    public Stream stream = Stream.Color;
    Mesh mesh;
    Texture2D uvmap;

    PointCloud pc;

    Vector3[] vertices;
    private GCHandle handle;
    private IntPtr verticesPtr;
    int frameSize;
    private IntPtr frameData;

    Intrinsics intrinsics;
    readonly AutoResetEvent e = new AutoResetEvent(false);

    void Start()
    {
        RealSenseDevice.Instance.OnStart += OnStartStreaming;
        RealSenseDevice.Instance.OnStop += OnStopStreaming;
    }

    private void OnStartStreaming(PipelineProfile activeProfile)
    {
        pc = new PointCloud();

        using (var profile = activeProfile.GetStream(stream))
        {
            if (profile == null)
            {
                Debug.LogWarningFormat("Stream {0} not in active profile", stream);
            }
        }

        using (var profile = activeProfile.GetStream(Stream.Depth) as VideoStreamProfile)
        {
            intrinsics = profile.GetIntrinsics();

            Assert.IsTrue(SystemInfo.SupportsTextureFormat(TextureFormat.RGFloat));
            uvmap = new Texture2D(profile.Width, profile.Height, TextureFormat.RGFloat, false, true)
            {
                wrapMode = TextureWrapMode.Clamp,
                filterMode = FilterMode.Point,
            };
            GetComponent<MeshRenderer>().sharedMaterial.SetTexture("_UVMap", uvmap);

            if (mesh != null)
                Destroy(mesh);

            mesh = new Mesh()
            {
                indexFormat = IndexFormat.UInt32,
            };

            vertices = new Vector3[profile.Width * profile.Height];
            handle = GCHandle.Alloc(vertices, GCHandleType.Pinned);
            verticesPtr = handle.AddrOfPinnedObject();

            var indices = new int[vertices.Length];
            for (int i = 0; i < vertices.Length; i++)
                indices[i] = i;

            mesh.MarkDynamic();
            mesh.vertices = vertices;

            var uvs = new Vector2[vertices.Length];
            Array.Clear(uvs, 0, uvs.Length);
            var invSize = new Vector2(1f / profile.Width, 1f / profile.Height);
            for (int j = 0; j < profile.Height; j++)
            {
                for (int i = 0; i < profile.Width; i++)
                {
                    uvs[i + j * profile.Width].x = i * invSize.x;
                    uvs[i + j * profile.Width].y = j * invSize.y;
                }
            }

            mesh.uv = uvs;

            mesh.SetIndices(indices, MeshTopology.Points, 0, false);
            mesh.bounds = new Bounds(Vector3.zero, Vector3.one * 10f);

            GetComponent<MeshFilter>().sharedMesh = mesh;
        }

        RealSenseDevice.Instance.onNewSampleSet += OnFrames;
    }

    void OnDestroy()
    {
        OnStopStreaming();
    }


    private void OnStopStreaming()
    {
        // RealSenseDevice.Instance.onNewSampleSet -= OnFrames;

        e.Reset();

        if (handle.IsAllocated)
            handle.Free();

        if (frameData != IntPtr.Zero)
        {
            Marshal.FreeHGlobal(frameData);
            frameData = IntPtr.Zero;
        }

        if (pc != null)
        {
            pc.Dispose();
            pc = null;
        }
    }

    private void OnFrames(FrameSet frames)
    {
        using (var depthFrame = frames.DepthFrame)
        using (var points = pc.Calculate(depthFrame))
        using (var f = frames.FirstOrDefault<VideoFrame>(stream))
        {
            pc.MapTexture(f);

            memcpy(verticesPtr, points.VertexData, points.Count * 3 * sizeof(float));

            frameSize = depthFrame.Width * depthFrame.Height * 2 * sizeof(float);
            if (frameData == IntPtr.Zero)
                frameData = Marshal.AllocHGlobal(frameSize);
            memcpy(frameData, points.TextureData, frameSize);

            e.Set();
        }
    }

    void Update()
    {
        if (e.WaitOne(0))
        {
            uvmap.LoadRawTextureData(frameData, frameSize);
            uvmap.Apply();

            mesh.vertices = vertices;
            mesh.UploadMeshData(false);
        }
    }

    #region Debug Drawing
    public bool DebugDrawing = true;

    static Material lineMaterial;

    static void CreateLineMaterial()
    {
        if (!lineMaterial)
        {
            // Unity has a built-in shader that is useful for drawing
            // simple colored things.
            Shader shader = Shader.Find("Hidden/Internal-Colored");
            lineMaterial = new Material(shader);
            lineMaterial.hideFlags = HideFlags.HideAndDontSave;
            // Turn on alpha blending
            lineMaterial.SetInt("_SrcBlend", (int)UnityEngine.Rendering.BlendMode.SrcAlpha);
            lineMaterial.SetInt("_DstBlend", (int)UnityEngine.Rendering.BlendMode.OneMinusSrcAlpha);
            // Turn backface culling off
            lineMaterial.SetInt("_Cull", (int)UnityEngine.Rendering.CullMode.Off);
            // Turn off depth writes
            lineMaterial.SetInt("_ZWrite", 0);
        }
    }

    private void OnRenderObject()
    {
        if (!DebugDrawing)
            return;

        CreateLineMaterial();
        // Apply the line material
        lineMaterial.SetPass(0);

        GL.PushMatrix();
        GL.MultMatrix(transform.localToWorldMatrix);

        // Grid
        GL.PushMatrix();
        GL.MultMatrix(Matrix4x4.Translate(new Vector3(-5, -2, 0)));
        GL.Begin(GL.LINES);
        GL.Color(new Color(0, 0, 0, 0.15f));
        for (int i = 0; i <= 10; i++)
        {
            GL.Vertex3(i, 0, 0);
            GL.Vertex3(i, 0, 10);
        }
        for (int i = 0; i <= 10; i++)
        {
            GL.Vertex3(0, 0, i);
            GL.Vertex3(10, 0, i);
        }
        GL.End();
        GL.PopMatrix();

        // Frustum
        float a = Mathf.Atan2(intrinsics.fy, intrinsics.fx);
        float fx = intrinsics.fx / intrinsics.width;
        const float D = 10f;

        GL.Begin(GL.LINES);
        GL.Color(new Color(1f, 1f, 1f, 0.2f));
        GL.Vertex3(0, 0, 0);
        GL.Vertex3(D * fx, a * D * fx, D);
        GL.Vertex3(0, 0, 0);
        GL.Vertex3(-D * fx, a * D * fx, D);
        GL.Vertex3(0, 0, 0);
        GL.Vertex3(-D * fx, a * -D * fx, D);
        GL.Vertex3(0, 0, 0);
        GL.Vertex3(D * fx, a * -D * fx, D);
        GL.End();

        for (int i = 0; i < 3; i++)
        {
            float d = D * (i + 1) / 3;
            float dfx = d * fx;
            GL.Begin(GL.LINE_STRIP);
            GL.Color(new Color(1f, 1f, 1f, 0.2f));
            GL.Vertex3(-dfx, a * dfx, d);
            GL.Vertex3(dfx, a * dfx, d);
            GL.Vertex3(dfx, -a * dfx, d);
            GL.Vertex3(-dfx, -a * dfx, d);
            GL.Vertex3(-dfx, a * dfx, d);
            GL.End();
        }

        // Axis
        GL.Begin(GL.LINES);
        GL.Color(Color.red);
        GL.Vertex3(0, 0, 0);
        GL.Vertex3(0.1f, 0, 0);
        GL.Color(Color.green);
        GL.Vertex3(0, 0, 0);
        GL.Vertex3(0, 0.1f, 0);
        GL.Color(Color.blue);
        GL.Vertex3(0, 0, 0);
        GL.Vertex3(0, 0, 0.1f);
        GL.End();

        GL.PopMatrix();
    }
    #endregion


    [DllImport("msvcrt.dll", EntryPoint = "memcpy", CallingConvention = CallingConvention.Cdecl, SetLastError = false)]
    internal static extern IntPtr memcpy(IntPtr dest, IntPtr src, int count);
}
