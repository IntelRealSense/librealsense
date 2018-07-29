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

    private Thread worker;
    private readonly AutoResetEvent stopEvent = new AutoResetEvent(false);

    private Pipeline m_pipeline;

    private HashSet<VideoProcessingBlock> m_processingBlocks = new HashSet<VideoProcessingBlock>();

    public void AddProcessingBlock(VideoProcessingBlock processingBlock)
    {
        m_processingBlocks.Add(processingBlock);
    }
    public void RemoveProcessingBlock(VideoProcessingBlock processingBlock)
    {
        m_processingBlocks.Remove(processingBlock);
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

        using (var activeStreams = ActiveProfile.Streams)
        {
            DeviceConfiguration.Profiles = new VideoStreamRequest[activeStreams.Count];
            for (int i = 0; i < DeviceConfiguration.Profiles.Length; i++)
            {
                var s = activeStreams[i];
                var p = new VideoStreamRequest()
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

    private Frame ApplyFilter(Frame frame, FrameSource frameSource, FramesReleaser framesReleaser, VideoProcessingBlock vpb)
    {
        if (!vpb.CanProcess(frame))
            return frame;

        // run the processing block.
        var processedFrame = vpb.Process(frame, frameSource, framesReleaser);

        // incase fork is requested, notify on new frame and use the original frame for the new frameset.
        if (vpb.Fork())
        {
            Instance.HandleFrame(processedFrame);
            processedFrame.Dispose();
            return frame;
        }

        // avoid disposing the frame incase the filter returns the original frame.
        if (processedFrame == frame)
            return frame;

        // replace the current frame with the processed one to be used as the input to the next iteration (next filter)
        frame.Dispose();
        return processedFrame;
    }

    private FrameSet HandleSingleFrameProcessingBlocks(FrameSet frameSet, FrameSource frameSource, FramesReleaser framesReleaser, VideoProcessingBlock videoProcessingBlock)
    {
        // single frame filters
        List<Frame> processedFrames = new List<Frame>();
        foreach (var frame in frameSet)
        {
            var currFrame = Instance.ApplyFilter(frame, frameSource, framesReleaser, videoProcessingBlock);

            // cache the pocessed frame
            processedFrames.Add(currFrame);
            if (frame != currFrame)
                frame.Dispose();
        }

        // Combine the frames into a single frameset
        var newFrameSet = frameSource.AllocateCompositeFrame(framesReleaser, processedFrames.ToArray());

        foreach (var f in processedFrames)
            f.Dispose();

        return newFrameSet;
    }

    private FrameSet HandleMultiFramesProcessingBlocks(FrameSet frameSet, FrameSource frameSource, FramesReleaser framesReleaser, VideoProcessingBlock videoProcessingBlock)
    {
        using (var frame = frameSet.AsFrame())
        {
            if (videoProcessingBlock.CanProcess(frame))
            {
                using (var f = videoProcessingBlock.Process(frame, frameSource, framesReleaser))
                {
                    if (videoProcessingBlock.Fork())
                    {
                        Instance.HandleFrameSet(FrameSet.FromFrame(f, framesReleaser));
                    }
                    else
                        return FrameSet.FromFrame(f, framesReleaser);
                }
            }
        }
        return frameSet;
    }

    private CustomProcessingBlock _block = new CustomProcessingBlock((f1, src) =>
    {
        using (var releaser = new FramesReleaser())
        {
            var frames = FrameSet.FromFrame(f1, releaser);

            var pbs = Instance.m_processingBlocks.OrderBy(i => i.Order).Where(i => i.Enabled).ToList();
            foreach (var vpb in pbs)
            {
                FrameSet processedFrames = frames;
                switch (vpb.ProcessingType)
                {
                    case ProcessingBlockType.Single:
                        processedFrames = Instance.HandleSingleFrameProcessingBlocks(frames, src, releaser, vpb);
                        break;
                    case ProcessingBlockType.Multi:
                        processedFrames = Instance.HandleMultiFramesProcessingBlocks(frames, src, releaser, vpb);
                        break;
                }
                frames = processedFrames;
            }

            Instance.HandleFrameSet(frames);
            foreach (var fr in frames)
            {
                using (fr)
                    Instance.HandleFrame(fr);
            }
        }
    });
}
