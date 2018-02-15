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
    private int streamWidth;
    private int streamHeight;
    private int totalImageSize;
    private int particleSize;
    private int particleCount;
    private ParticleSystem.Particle[] particles;
    private Intrinsics depthIntrinsic;

    public float pSize = 0.01f;
    public float gradientNear = 0.0f;
    public float gradientFar = 3.0f;
    public int skipParticles = 2;
    public ParticleSystem pointCloudParticles;

    // Use this for initialization
    void Start()
    {
        Application.runInBackground = true;
        particleCount = 0;

        if (RealSenseDevice.Instance.ActiveProfile != null)
        {
            OnStartStreaming(RealSenseDevice.Instance.ActiveProfile);
        }
        else
        {
            RealSenseDevice.Instance.OnStart += OnStartStreaming;
        }
        particles = new ParticleSystem.Particle[particleSize];
    }

    private void OnStartStreaming(PipelineProfile activeProfile)
    {
        if(InitializeStream(activeProfile))
        {
            RealSenseDevice.Instance.onNewSample += OnFrame;
        }
    }

    private void OnFrame(Frame frame)
    {
        if (frame.Profile.Stream != Intel.RealSense.Stream.Depth)
            return;

        var depthFrame = frame as DepthFrame;
        if (depthFrame == null)
        {
            Debug.Log("Frame is not a depth frame");
            return;
        }

        UpdateParticleParams(depthFrame.Width, depthFrame.Height, depthFrame.Profile.Format);

        //TODO: Use PointCloud Processing Block instead
        int particleIndex = 0;
        particleCount = particleSize;
        for (int y = 0; y < streamHeight; y += skipParticles)
        {
            for (int x = 0; x < streamWidth; x += skipParticles)
            {
                float depthValue = depthFrame.GetDistance(x, y);
                //depthValue *= 0.001f;
                if (depthValue > 0)
                {
                    Vector3 point3D;
                    float xx = x;
                    float yy = y;
                    float px = (xx - depthIntrinsic.ppx) / depthIntrinsic.fx;
                    float py = (yy - depthIntrinsic.ppy) / depthIntrinsic.fy;

                    point3D.x = depthValue * px;
                    point3D.y = depthValue * -py;
                    point3D.z = depthValue;
                    particles[particleIndex].position = point3D;
                    particles[particleIndex].startSize = pSize;
                    //TODO: Take from texture map of pointcloud, or use the colorizer
                    float pColorVal = Remap((float)depthValue, gradientNear, gradientFar, 255.0f, 0.0f);
                    particles[particleIndex].startColor = new Color32((byte)pColorVal, (byte)pColorVal, (byte)pColorVal, 255);
                }
                else
                {
                    particles[particleIndex].position = new Vector3(0, 0, 0);
                    particles[particleIndex].startSize = (float)0.0;
                    particles[particleIndex].startColor = new Color32(0, 0, 0, 0);
                }
                particleIndex++;
            }
        }
    }

    private void UpdateParticleParams(int width, int height, Format format)
    {
        streamWidth = width;
        streamHeight = height;

        if (format != Format.Z16)
        {
            Debug.Log("Unsupported format");
            return;
        }

        const int bpp = 2;

        if (totalImageSize != streamWidth * streamHeight * bpp)
        {
            totalImageSize = streamWidth * streamHeight * bpp;
            particleSize = totalImageSize / skipParticles;
            particles = new ParticleSystem.Particle[particleSize];
        }

        if (particleSize != totalImageSize / skipParticles)
        {
            particleSize = totalImageSize / skipParticles;
            particles = new ParticleSystem.Particle[particleSize];
        }
    }

    void Update()
    {
        //TODO: Lock & copy particles?
        pointCloudParticles.SetParticles(particles, particleCount);
    }

    private bool InitializeStream(PipelineProfile activeProfile)
    {
        var depthStream = activeProfile.Streams.FirstOrDefault(s => s.Stream == Intel.RealSense.Stream.Depth);
        if (depthStream == null)
        {
            Debug.Log("No Depth stream available");
            return false;
        }
        var depthProfile = depthStream as VideoStreamProfile;
        depthIntrinsic = depthProfile.GetIntrinsics();
        streamWidth = depthProfile.Width;
        streamHeight = depthProfile.Height;
        return true;
    }

    private float Remap(float value, float from1, float to1, float from2, float to2)
    {
        return (value - from1) / (to1 - from1) * (to2 - from2) + from2;
    }
}
