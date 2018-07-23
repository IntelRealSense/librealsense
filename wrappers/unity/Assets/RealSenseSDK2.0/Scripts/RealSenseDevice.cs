﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Threading;
using UnityEngine;
using Intel.RealSense;
using System.Collections;

/// <summary>
/// Manages streaming using a RealSense Device
/// </summary>
[HelpURL("https://github.com/IntelRealSense/librealsense/tree/master/wrappers/unity")]
public class RealSenseDevice : MonoBehaviour
{
    /// <summary>
    /// The Paralllism mode of the module
    /// </summary>
    public enum ProcessMode
    {
        Multithread,
        UnityThread,
    }

    public static RealSenseDevice Instance { get; private set; }

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
    public RealSenseConfiguration DeviceConfiguration = new RealSenseConfiguration
    {
        mode = RealSenseConfiguration.Mode.Live,
        RequestedSerialNumber = string.Empty,
        Profiles = new VideoStreamRequest[] {
            new VideoStreamRequest {Stream = Stream.Depth, StreamIndex = -1, Width = 640, Height = 480, Format = Format.Z16 , Framerate = 30 },
            new VideoStreamRequest {Stream = Stream.Infrared, StreamIndex = -1, Width = 640, Height = 480, Format = Format.Y8 , Framerate = 30 },
            new VideoStreamRequest {Stream = Stream.Color, StreamIndex = -1, Width = 640, Height = 480, Format = Format.Rgb8 , Framerate = 30 }
        }
    };

    //private List<VideoStreamRequest> _pipelineOutputProfiles { get; set; }

    private Thread worker;
    private readonly AutoResetEvent stopEvent = new AutoResetEvent(false);

    private Pipeline m_pipeline;

    private List<IVideoProcessingBlock> m_processingBlocks = new List<IVideoProcessingBlock>();

    public void AddProcessingBlock(IVideoProcessingBlock processingBlock)
    {
        m_processingBlocks.Add(processingBlock);
    }

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

        DeviceConfiguration.Profiles = new VideoStreamRequest[ActiveProfile.Streams.Count];
        for (int i = 0; i < DeviceConfiguration.Profiles.Length; i++)
        {
            DeviceConfiguration.Profiles[i] = new VideoStreamRequest();
            var p = DeviceConfiguration.Profiles[i];
            var s = ActiveProfile.Streams[i];
            p.Stream = s.Stream;
            p.Format = s.Format;
            p.Framerate = s.Framerate;
            p.StreamIndex = s.Index;
            var vs = s as VideoStreamProfile;
            if (vs != null)
            {
                p.Width = vs.Width;
                p.Height = vs.Height;
            }
            DeviceConfiguration.Profiles[i] = p;
        }


        if (processMode == ProcessMode.Multithread)
        {
            stopEvent.Reset();
            worker = new Thread(WaitForFrames);
            worker.IsBackground = true;
            worker.Start();
        }

        // Register to results of processing via a callback:
        _block.Start(_cb);

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

    private VideoStreamRequest FrameToVideoStreamRequest(Frame f)
    {
        var vf = f as VideoFrame;
        return new VideoStreamRequest
        {
            Stream = vf.Profile.Stream,
            StreamIndex = vf.Profile.Index,
            Width = vf.Width,
            Height = vf.Height,
            Format = vf.Profile.Format,
            Framerate = vf.Profile.Framerate
        };
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
                _block.ProcessFrames(frames);
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
                _block.ProcessFrames(frames);
        }
    }

    private CustomProcessingBlock _block = new CustomProcessingBlock((f1, src) =>
    {
        using (var releaser = new FramesReleaser())
        {
            var frames = FrameSet.FromFrame(f1, releaser);

            List<Frame> processedFrames = new List<Frame>();
            foreach (var frame in frames)
            {
                var f = frame;
                foreach (var vpb in Instance.m_processingBlocks)
                {
                    if (!(vpb is VideoProcessingBlock))
                        continue;
                    var pb = vpb as VideoProcessingBlock;
                    if (pb.CanProcess(f) && pb.IsEnabled())
                    {
                        var newFrame = pb.Process(f);
                        if (pb.Fork())
                        {
                            Instance.HandleFrame(newFrame);
                            newFrame.Dispose();
                            continue;
                        }
                        if (newFrame == f)
                            continue;
                        f.Dispose();
                        f = newFrame;
                    }
                }
                processedFrames.Add(f);
                if (frame != f)
                    frame.Dispose();
            }

            // Combine the frames into a single result
            var res = src.AllocateCompositeFrame(releaser, processedFrames.ToArray());
            // Send it to the next processing stage
            src.FramesReady(res);
            foreach (var f in processedFrames)
            {
                f.Dispose();
            }
        }
    });

    CustomProcessingBlock.FrameCallback _cb = f =>
    {
        using (var releaser = new FramesReleaser())
        {
            var frames = FrameSet.FromFrame(f, releaser);

            foreach (var vpb in Instance.m_processingBlocks)
            {
                if (!(vpb is MultiFrameVideoProcessingBlock))
                    continue;
                var pb = vpb as MultiFrameVideoProcessingBlock;
                if (pb.CanProcess(frames) && pb.IsEnabled())
                {
                    frames = pb.Process(frames, releaser);
                }
            }

            Instance.HandleFrameSet(frames);
            foreach (var fr in frames)
            {
                using (fr)
                {
                    Instance.HandleFrame(fr);
                }
            }
        }
    };
}
