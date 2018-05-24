using System;
using UnityEngine;
using Intel.RealSense;

public class PointCloudGenerator : MonoBehaviour
{
    public bool mirrored;
    public float pointsSize = 1;
    public int skipParticles = 2;
    public ParticleSystem pointCloudParticles;

    private ParticleSystem.Particle[] particles = new ParticleSystem.Particle[0];
    private PointCloud pc = new PointCloud();
    private Points.Vertex[] vertices;
    private byte[] lastColorImage;
    private Align aligner;

    // Use this for initialization
    void Start()
    {
        aligner = new Align(Intel.RealSense.Stream.Color);
        RealSenseDevice.Instance.onNewSampleSet += OnFrames;

    }
    //object l = new object();
    private void OnFrames(FrameSet frames)
    {
        using (var aligned = aligner.Process(frames))
        {
            using (var colorFrame = aligned.ColorFrame)
            using (var depthFrame = aligned.DepthFrame)
            {
                if (depthFrame == null)
                {
                    Debug.Log("No depth frame in frameset, can't create point cloud");
                    return;
                }

                if (!UpdateParticleParams(depthFrame.Width, depthFrame.Height))
                {
                    Debug.Log("Unable to craete point cloud");
                    return;
                }

                using (var points = pc.Calculate(depthFrame))
                {
                    setParticals(points, colorFrame);
                }
            }
        }
    }

    private void setParticals(Points points, VideoFrame colorFrame)
    {
        if (points == null)
            throw new Exception("Frame in queue is not a points frame");

        if (lastColorImage == null)
        {
            int colorFrameSize = colorFrame.Height * colorFrame.Stride;
            lastColorImage = new byte[colorFrameSize];
        }
        colorFrame.CopyTo(lastColorImage);

        vertices = vertices ?? new Points.Vertex[points.Count];
        points.CopyTo(vertices);

        Debug.Assert(vertices.Length == particles.Length);
        int mirror = mirrored ? -1 : 1;
        for (int index = 0; index < vertices.Length; index += skipParticles)
        {
            var v = vertices[index];
            if (v.z > 0)
            {
                particles[index].position = new Vector3(v.x * mirror, v.y, v.z);
                particles[index].startSize = v.z * pointsSize * 0.02f;
                particles[index].startColor = new Color32(lastColorImage[index * 3], lastColorImage[index * 3 + 1], lastColorImage[index * 3 + 2], 255);
            }
            else //Required since we reuse the array
            {
                particles[index].position = Vector3.zero;
                particles[index].startSize = 0;
                particles[index].startColor = Color.black;
            }
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
        //Either way, update particles
        pointCloudParticles.SetParticles(particles, particles.Length);
    }
}