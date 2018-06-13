using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Runtime.InteropServices;
using System.Threading;
using Intel.RealSense;
using System.Linq;
using UnityEngine.Rendering;
using UnityEngine.Assertions;

[RequireComponent(typeof(MeshFilter), typeof(MeshRenderer))]
public class PointCloudGenerator : MonoBehaviour
{
    public Stream stream = Stream.Color;
    Mesh mesh;
    Texture2D uvmap;

    PointCloud pc;

    int[] indices;
    Vector3[] vertices;
    private GCHandle handle;

    int frameSize;
    private IntPtr frameData;

    readonly AutoResetEvent e = new AutoResetEvent(false);

    void Start()
    {
        pc = new PointCloud();

        RealSenseDevice.Instance.onNewSampleSet += OnFrames;
        var streams = RealSenseDevice.Instance.ActiveProfile.Streams;

        using (var profile = streams.FirstOrDefault(s => s.Stream == stream) as VideoStreamProfile)
        {
            if (profile != null)
            {
                Assert.IsTrue(SystemInfo.SupportsTextureFormat(TextureFormat.RGFloat));
                uvmap = new Texture2D(profile.Width, profile.Height, TextureFormat.RGFloat, false)
                {
                    wrapMode = TextureWrapMode.Clamp,
                    filterMode = FilterMode.Point,
                };
                GetComponent<MeshRenderer>().sharedMaterial.SetTexture("_UVMap", uvmap);
            }
            else
            {
                Debug.LogWarningFormat("Stream {0} not found", stream);
            }
        }

        using (var profile = streams.First(s => s.Stream == Stream.Depth) as VideoStreamProfile)
        {

            mesh = new Mesh()
            {
                indexFormat = IndexFormat.UInt32,
            };

            vertices = new Vector3[profile.Width * profile.Height];
            handle = GCHandle.Alloc(vertices, GCHandleType.Pinned);

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
    }

    void OnDestroy()
    {
        if (pc != null)
            pc.Dispose();

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
        using (var points = pc.Calculate(frames.DepthFrame))
        {
            memcpy(handle.AddrOfPinnedObject(), points.VertexData, points.Count * 3 * sizeof(float));

            // var f = frames[stream] as VideoFrame;
            var f = frames.FirstOrDefault<VideoFrame>(stream);
            if (f != null)
            {
                pc.MapTexture(f);

                frameSize = f.Width * f.Height * 2 * sizeof(float);
                if (frameData == IntPtr.Zero)
                    frameData = Marshal.AllocHGlobal(frameSize);
                memcpy(frameData, points.TextureData, frameSize);
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
