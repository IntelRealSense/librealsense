using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Threading;
using UnityEngine;
using Intel.RealSense;
using System.Collections;
using System.Linq;

/// <summary>
/// Manages streaming using a RealSense Device
/// </summary>
[HelpURL("https://github.com/IntelRealSense/librealsense/tree/master/wrappers/unity")]
public class RsDevice : MonoBehaviour
{
    /// <summary>
    /// The Paralllism mode of the module
    /// </summary>
    public enum ProcessMode
    {
        Multithread,
        UnityThread,
    }

    public static RsDevice Instance { get; private set; }

    /// <summary>
    /// Threading mode of operation, Multithreasds or Unitythread
    /// </summary>
    [Tooltip("Threading mode of operation, Multithreads or Unitythread")]
    public ProcessMode processMode;

    public bool Streaming { get; private set; }

    /// <summary>
    /// Notifies upon streaming start
    /// </summary>
    public event Action<PipelineProfile> OnStart;

    /// <summary>
    /// Notifies when streaming has stopped
    /// </summary>
    public event Action OnStop;

    /// <summary>
    /// Fired when a new frame is available
    /// </summary>
    public event Action<Frame> OnNewSample;

    /// <summary>
    /// Fired when a new time-synchronized frame set is available
    /// </summary>
    public event Action<FrameSet> OnNewSampleSet;

    /// <summary>
    /// Provides access to the current pipeline profiles in use by the Manager
    /// </summary>
    public PipelineProfile ActiveProfile { get; private set; } //TODO: Make private and have other classes register OnStart and use that profile.

    /// <summary>
    /// User configuration
    /// </summary>
    public RsConfiguration DeviceConfiguration = new RsConfiguration
    {
        mode = RsConfiguration.Mode.Live,
        RequestedSerialNumber = string.Empty,
        Profiles = new RsVideoStreamRequest[] {
            new RsVideoStreamRequest {Stream = Stream.Depth, StreamIndex = -1, Width = 640, Height = 480, Format = Format.Z16 , Framerate = 30 },
            new RsVideoStreamRequest {Stream = Stream.Infrared, StreamIndex = -1, Width = 640, Height = 480, Format = Format.Y8 , Framerate = 30 },
            new RsVideoStreamRequest {Stream = Stream.Color, StreamIndex = -1, Width = 640, Height = 480, Format = Format.Rgb8 , Framerate = 30 }
        }
    };

    private Thread worker;
    private readonly AutoResetEvent stopEvent = new AutoResetEvent(false);

    private Pipeline m_pipeline;

    public RsProcessingPipe _processingPipe;

    void Awake()
    {
        if (Instance != null && Instance != this)
            throw new Exception(string.Format("{0} singleton already instanced", this.GetType()));
        Instance = this;
    }

    void OnEnable()
    {
        m_pipeline = new Pipeline();

        using (var cfg = DeviceConfiguration.ToPipelineConfig())
        {
            ActiveProfile = m_pipeline.Start(cfg);
        }

        using (var activeStreams = ActiveProfile.Streams)
        {
            DeviceConfiguration.Profiles = new RsVideoStreamRequest[activeStreams.Count];
            for (int i = 0; i < DeviceConfiguration.Profiles.Length; i++)
            {
                var s = activeStreams[i];
                var p = new RsVideoStreamRequest()
                {
                    Stream = s.Stream,
                    Format = s.Format,
                    Framerate = s.Framerate,
                    StreamIndex = s.Index,
                };
                var vs = s as VideoStreamProfile;
                if (vs != null)
                {
                    p.Width = vs.Width;
                    p.Height = vs.Height;
                }
                DeviceConfiguration.Profiles[i] = p;
            }
        }


        if (processMode == ProcessMode.Multithread)
        {
            stopEvent.Reset();
            worker = new Thread(WaitForFrames);
            worker.IsBackground = true;
            worker.Start();
        }

        StartCoroutine(WaitAndStart());
    }

    IEnumerator WaitAndStart()
    {
        yield return new WaitForEndOfFrame();
        Streaming = true;
        if (OnStart != null)
            OnStart(ActiveProfile);
    }

    void OnDisable()
    {
        OnNewSample = null;
        OnNewSampleSet = null;

        if (worker != null)
        {
            stopEvent.Set();
            worker.Join();
        }

        if (Streaming && OnStop != null)
            OnStop();

        if (m_pipeline != null)
        {
            if (Streaming)
                m_pipeline.Stop();
            m_pipeline.Release();
            m_pipeline = null;
        }

        Streaming = false;

        if (ActiveProfile != null)
        {
            ActiveProfile.Dispose();
            ActiveProfile = null;
        }
    }

    void OnDestroy()
    {
        OnStart = null;
        OnStop = null;

        if (m_pipeline != null)
            m_pipeline.Release();
        m_pipeline = null;

        Instance = null;
    }

    /// <summary>
    /// Handles the current frame
    /// </summary>
    /// <param name="frame">The frame instance</param>
    private void HandleFrame(Frame frame)
    {
        var s = OnNewSample;
        if (s != null)
        {
            s(frame);
        }
    }

    private void HandleFrameSet(FrameSet frames)
    {
        var s = OnNewSampleSet;
        if (s != null)
        {
            s(frames);
        }
    }

    /// <summary>
    /// Worker Thread for multithreaded operations
    /// </summary>
    private void WaitForFrames()
    {
        while (!stopEvent.WaitOne(0))
        {
            using (var frames = m_pipeline.WaitForFrames())
            {
                OnNewFrameSet(frames);
            }
        }
    }

    void Update()
    {
        if (!Streaming)
            return;

        if (processMode != ProcessMode.UnityThread)
            return;

        FrameSet frames;
        if (m_pipeline.PollForFrames(out frames))
        {
            using (frames)
            {
                OnNewFrameSet(frames);
            }
        }
    }

    private void OnNewFrameSet(FrameSet frames)
    {
        if (_processingPipe != null)
            _block.ProcessFrames(frames);
        else
        {
            HandleFrameSet(frames);
            foreach (var fr in frames)
            {
                using (fr)
                    HandleFrame(fr);
            }
        }
    }

    private CustomProcessingBlock _block = new CustomProcessingBlock((f1, src) =>
    {
        using (var releaser = new FramesReleaser())
        {
            var frames = FrameSet.FromFrame(f1, releaser);
            Instance._processingPipe.ProcessFrames(frames, src, releaser, Instance.HandleFrame, Instance.HandleFrameSet);
        }
    });
}
