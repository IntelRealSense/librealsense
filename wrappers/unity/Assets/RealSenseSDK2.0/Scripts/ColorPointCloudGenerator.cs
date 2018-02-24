using UnityEngine;
using Intel.RealSense;
using System.Linq;

public class ColorPointCloudGenerator : MonoBehaviour {

    private int streamWidth;
    private int streamHeight;
    private int totalImageSize;
    private int particleSize;
    private int particleCount;
    private ParticleSystem.Particle[] particles;
    private PointCloud pc = new PointCloud();
    private byte[] colorData;
    private DepthFrame depthFrame;
    private VideoFrame vidFrame;
    private Points.Vertex[] vertices;
    private Align aligner;

    public float pointsSize = 0.01f;
    public int skipParticles = 2;
    public ParticleSystem pointCloudParticles;

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

        aligner = new Align(Stream.Color);
    }

    private void OnStartStreaming(PipelineProfile activeProfile)
    {
        if (InitializeStream(activeProfile))
        {
            RealSenseDevice.Instance.onNewSampleSet += OnFrame;
        }
    }

    private void OnFrame(FrameSet frameset)
    {
        //Align depth frame to color frame
        using (FrameSet aligned = aligner.Process(frameset))
        {
            //DepthFrame
            depthFrame = aligned.Where(f => f.Profile.Stream == Stream.Depth).First() as DepthFrame;

            if (depthFrame == null)
            {
                Debug.Log("depth frame is null");
                return;
            }

            //ColorFrame
            vidFrame = aligned.Where(f => f.Profile.Stream == Stream.Color).First() as VideoFrame;

            if (vidFrame == null)
            {
                Debug.Log("color frame is null");
                return;
            }

            UpdateParticleParams(depthFrame.Width, depthFrame.Height, depthFrame.Profile.Format);

            //CoordinateData
            var points = pc.Calculate(depthFrame);
            vertices = vertices ?? new Points.Vertex[points.Count];
            points.CopyTo(vertices);

            //ColorData
            colorData = colorData ?? new byte[vidFrame.Stride * vidFrame.Height];
            vidFrame.CopyTo(colorData);

            for (int index = 0; index < particleSize; index += skipParticles)
            {
                var v = vertices[index];

                if (v.z > 0)
                {
                    particles[index].position = new Vector3(v.x, v.y, v.z);
                    particles[index].startSize = pointsSize;
                    particles[index].startColor = new Color32(colorData[index * 3], colorData[index * 3 + 1], colorData[index * 3 + 2], 255);
                }
                
                else
                {
                    particles[index].position = new Vector3(0, 0, 0);
                    particles[index].startSize = (float)0.0;
                    particles[index].startColor = new Color32(0, 0, 0, 0);
                }
                
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
        particleCount = particleSize;
    }

    void Update()
    {
        pointCloudParticles.SetParticles(particles, particleCount);
    }

    private bool InitializeStream(PipelineProfile activeProfile)
    {
        var depthStream = activeProfile.Streams.FirstOrDefault(s => s.Stream == Stream.Depth);
        if (depthStream == null)
        {
            Debug.Log("No Depth stream available");
            return false;
        }
        var depthProfile = depthStream as VideoStreamProfile;
        streamWidth = depthProfile.Width;
        streamHeight = depthProfile.Height;
        return true;
    }
}
