using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Intel.RealSense;
using System.Linq;
using UnityEngine.Rendering;
using UnityEngine.Assertions;
using System.Runtime.InteropServices;
using System.Threading;

[RequireComponent(typeof(MeshFilter), typeof(MeshRenderer))]
public class PointCloudGenerator : MonoBehaviour
{
    public Stream stream = Stream.Color;
    Mesh mesh;
    Texture2D uvmap;

    readonly PointCloud pc = new PointCloud();

    int[] indices;
    Vector3[] vertices;
    private GCHandle handle;
    private IntPtr verticesPtr;
    int frameSize;
    private IntPtr frameData;

    readonly AutoResetEvent e = new AutoResetEvent(false);

    void Start()
    {
        RealSenseDevice.Instance.OnStart += OnStartStreaming;
        RealSenseDevice.Instance.OnStop += OnStopStreaming;
    }

    private void OnStartStreaming(PipelineProfile activeProfile)
    {
        using (var profile = activeProfile.Streams.FirstOrDefault(p => p.Stream == stream))
        {
            if (profile == null)
            {
                Debug.LogWarningFormat("Stream {0} not in active profile", stream);
            }
        }

        using (var profile = activeProfile.GetStream(Stream.Depth) as VideoStreamProfile)
        {
            Assert.IsTrue(SystemInfo.SupportsTextureFormat(TextureFormat.RGFloat));
            uvmap = new Texture2D(profile.Width, profile.Height, TextureFormat.RGFloat, false)
            {
                wrapMode = TextureWrapMode.Clamp,
                filterMode = FilterMode.Point,
            };
            GetComponent<MeshRenderer>().sharedMaterial.SetTexture("_UVMap", uvmap);

            mesh = new Mesh()
            {
                indexFormat = IndexFormat.UInt32,
            };

            vertices = new Vector3[profile.Width * profile.Height];
            handle = GCHandle.Alloc(vertices, GCHandleType.Pinned);
            verticesPtr = handle.AddrOfPinnedObject();

            indices = Enumerable.Range(0, profile.Width * profile.Height).ToArray();

            mesh.MarkDynamic();
            mesh.vertices = vertices;
            mesh.uv =
                Enumerable.Range(0, profile.Height).Select(y =>
                Enumerable.Range(0, profile.Width).Select(x =>
                    new Vector2((float)x / profile.Width, (float)y / profile.Height)
                )).SelectMany(v => v).ToArray();

            mesh.SetIndices(indices, MeshTopology.Points, 0, false);
            mesh.bounds = new Bounds(Vector3.zero, Vector3.one * 10f);

            GetComponent<MeshFilter>().sharedMesh = mesh;
        }

        RealSenseDevice.Instance.onNewSampleSet += OnFrames;
    }

    void OnDestroy()
    {
        e.WaitOne();
        
        if (pc != null)
            pc.Dispose();

        OnStopStreaming();
    }

    private void OnStopStreaming()
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

    private void OnFrames(FrameSet frames)
    {
        using (DepthFrame depthFrame = frames.DepthFrame)
        using (var points = pc.Calculate(depthFrame))
        {
            memcpy(verticesPtr, points.VertexData, points.Count * 3 * sizeof(float));

            using (var f = frames.FirstOrDefault<VideoFrame>(stream))
            {
                if (f != null)
                {
                    pc.MapTexture(f);

                    frameSize = depthFrame.Width * depthFrame.Height * 2 * sizeof(float);
                    if (frameData == IntPtr.Zero)
                        frameData = Marshal.AllocHGlobal(frameSize);
                    memcpy(frameData, points.TextureData, frameSize);
                }
            }

            e.Set();
        }
    }

    void Update()
    {
        if (e.WaitOne(0))
        {
            if (frameData != IntPtr.Zero && frameSize != 0)
            {
                uvmap.LoadRawTextureData(frameData, frameSize);
                uvmap.Apply();
            }

            mesh.vertices = vertices;
        }
    }

    [DllImport("msvcrt.dll", EntryPoint = "memcpy", CallingConvention = CallingConvention.Cdecl, SetLastError = false)]
    internal static extern IntPtr memcpy(IntPtr dest, IntPtr src, int count);
}
>>>>>>> Stashed changes
