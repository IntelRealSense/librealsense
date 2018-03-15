using System;
using System.IO;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using System.Runtime.InteropServices;
using System.Threading;
using Intel.RealSense;
using System.Linq;

public class PointCloudGenerator : MonoBehaviour
{
    private ParticleSystem.Particle[] particles = new ParticleSystem.Particle[0];
    private PointCloud pc = new PointCloud();
    FrameQueue pointsQueue = new FrameQueue(1);
    Points.Vertex[] vertices;
    Points.TextureCoordinate[] textureCoordinate;
    private byte[] lastColorImage;
    Texture2D colorTexture;
    private int colorFrameWidth;
    private int colorFrameHeight;

    public bool mirrored;
    public float pointsSize = 0.01f;
    public int skipParticles = 2;
    public ParticleSystem pointCloudParticles;

    // Use this for initialization
    void Start()
    {
        //RealSenseDevice.Instance.onNewSample += OnFrame;
        RealSenseDevice.Instance.onNewSampleSet += OnFrames;

    }
    object l = new object();
    private void OnFrames(FrameSet frames)
    {
        if(frames.DepthFrame == null)
        {
            Debug.Log("No depth frame in frameset, can't create point cloud");
            return;
        }

        if (!UpdateParticleParams(frames.DepthFrame.Width, frames.DepthFrame.Height))
        {
            Debug.Log("Unable to craete point cloud");
            return;
        }

        using (var points = pc.Calculate(frames.DepthFrame))
        {
            if (frames.ColorFrame != null)
            {
                if (frames.ColorFrame.BitsPerPixel == 24)
                {
                    pc.MapTexture(frames.ColorFrame);
                    colorFrameWidth = frames.ColorFrame.Width;
                    colorFrameHeight = frames.ColorFrame.Height;
                    var newSize = frames.ColorFrame.Stride * colorFrameHeight;
                    lock (l)
                    {
                        if (lastColorImage == null || lastColorImage.Length != newSize)
                            lastColorImage = new byte[newSize];

                        frames.ColorFrame.CopyTo(lastColorImage);
                    }
                }
            }
            pointsQueue.Enqueue(points);
        }
    }

    private bool UpdateParticleParams(int width, int height)
    {
        var numParticles = (width * height);
        if (particles.Length != numParticles)
        {
            particles = new ParticleSystem.Particle[numParticles];
        }

        return true;
    }

    void Update()
    {
        Frame frame;
        if (pointsQueue.PollForFrame(out frame))
        {
            using (Points points = frame as Points)
            {
                if (points == null)
                    throw new Exception("Frame in queue is not a points frame");

                vertices = vertices ?? new Points.Vertex[points.Count];
                points.CopyTo(vertices);

                lock (l)
                {
                    if (textureCoordinate == null || textureCoordinate.Length != points.Count)
                        textureCoordinate = new Points.TextureCoordinate[points.Count];

                    points.CopyTo(textureCoordinate);

                    if (lastColorImage != null)
                    {
                        if (colorTexture == null || colorTexture.width != colorFrameWidth || colorTexture.height != colorFrameHeight)
                        {
                            colorTexture = new Texture2D(colorFrameWidth, colorFrameHeight, TextureFormat.RGB24, false, true)
                            {
                                wrapMode = TextureWrapMode.Clamp,
                                filterMode = FilterMode.Point
                            };
                        }

                        colorTexture.LoadRawTextureData(lastColorImage);
                        colorTexture.Apply();
                    }
                }
                Debug.Assert(vertices.Length == particles.Length);
                int mirror = mirrored ? -1 : 1;
                for (int index = 0; index < vertices.Length; index += skipParticles)
                {
                    var v = vertices[index];
                    if (v.z > 0)
                    {
                        particles[index].position = new Vector3(v.x * mirror, v.y, v.z);
                        particles[index].startSize = pointsSize;
                        particles[index].startColor = colorTexture.GetPixelBilinear(textureCoordinate[index].u, textureCoordinate[index].v);
                    }
                    else //Required since we reuse the array
                    {
                        particles[index].position = Vector3.zero;
                        particles[index].startSize = 0;
                        particles[index].startColor = Color.black;
                    }
                }
            }
        }
        //Either way, update particles
        pointCloudParticles.SetParticles(particles, particles.Length);
    }
}
