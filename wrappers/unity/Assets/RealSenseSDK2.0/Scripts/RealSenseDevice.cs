using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Threading;
using UnityEngine;
using Intel.RealSense;

/// <summary>
/// Manages streaming using a RealSense Device
/// </summary>
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

    /// <summary>
    /// Returns the current Instance
    /// </summary>
    public static RealSenseDevice Instance { get; private set; }

    /// <summary>
    /// Threading mode of operation, Multithreasds or Unitythread
    /// </summary>
    [Tooltip("Threading mode of operation, Multithreasds or Unitythread")]
    public ProcessMode processMode;

    public bool pause = false;

    /// <summary>
    /// Depth Texture
    /// </summary>
    //public Texture2D m_depthTex;
    /// <summary>
    /// Infra red texture
    /// </summary>
    //public Texture2D m_IRTex;

    /// <summary>
    /// Called when new depth data is ready
    /// </summary>
    public Action DepthImageUpdated { get; set; }

    /// <summary>
    /// Notifies upon streaming start
    /// </summary>
    public event Action<PipelineProfile> OnStart;

    /// <summary>
    /// Provides access to the current pipeline profiles in use by the Manager
    /// </summary>
    public PipelineProfile ActiveProfile { get; private set; } //TODO: Make private and have other classes register OnStart and use that profile.

    [Space]
    [Header("Configuration")]
    /// <summary>
    /// User configuration
    /// </summary>
    public RealSenseConfiguration DeviceConfiguration = new RealSenseConfiguration
    {
        Profiles = new VideoStreamRequest[] {
            new VideoStreamRequest {Stream = Stream.Depth, StreamIndex = -1, Width = 640, Height = 480, Format = Format.Z16 , Framerate = 30 },
            new VideoStreamRequest {Stream = Stream.Infrared, StreamIndex = -1, Width = 640, Height = 480, Format = Format.Y8 , Framerate = 30 },
            new VideoStreamRequest {Stream = Stream.Color, StreamIndex = -1, Width = 640, Height = 480, Format = Format.Rgb8 , Framerate = 30 }
        }
    };

    private BackgroundWorker worker;
    private Pipeline m_pipeline;
    private Config m_config;
    public event Action<Frame> onNewSample;
    public event Action<FrameSet> onNewSampleSet;

    void Awake()
    {
        if (Instance != null)
            throw new Exception(string.Format("{0} singleton already instanced", this.GetType()));
        Instance = this;

        m_pipeline = new Pipeline();
        m_config = DeviceConfiguration.ToPipelineConfig();
        ActiveProfile = m_config.Resolve(m_pipeline);
    }

    void Start()
    {
        try
        {
            ActiveProfile = m_pipeline.Start(m_config);

            //Start thread for multithread option
            if (processMode == ProcessMode.Multithread)
            {
                worker = new BackgroundWorker();
                worker.DoWork += Worker_DoWork;
                worker.WorkerSupportsCancellation = true;//Support cancel
                worker.RunWorkerAsync();//Start the thread
            }
        }
        catch (Exception e)
        {
            Debug.Log("Failed to start. Error: " + e.Message);
        }

        if (OnStart != null)
            OnStart(ActiveProfile);
    }

    void OnDestroy()
    {
        Debug.Log("RealSenseDevice OnDestory");
        if (worker != null)
        {
            //Destroy BG thread
            worker.CancelAsync();
        }
        try
        {
            m_pipeline.Stop();
            m_pipeline.Release();
        }
        catch (Exception e)
        {
            Debug.Log(e.Message);
        }
    }

    /// <summary>
    /// Handles the current frame
    /// </summary>
    /// <param name="frame">The frame instance</param>
    private void HandleFrame(Frame frame)
    {
        var s = onNewSample;
        if (s != null)
        {
            s(frame);
        }
    }

    private void HandleFrameSet(FrameSet frames)
    {
        var s = onNewSampleSet;
        if (s != null)
        {
            s(frames);
        }
    }
    /// <summary>
    /// Process frame on each new frame, ends by calling the event
    /// </summary>
    public void OnFrames(FrameSet frames)
    {
        HandleFrameSet(frames);

        foreach (var frame in frames)
        {
            using (frame)
            {
                HandleFrame(frame);
            }
        }
    }

    /// <summary>
    /// Worker Thread for multithreaded operations
    /// </summary>
    /// <param name="sender">the thread instance owner</param>
    /// <param name="e">arguments</param>
    private void Worker_DoWork(object sender, DoWorkEventArgs e)
    {
        while (worker.CancellationPending == false)
        {
            if (pause)
                continue;

            using (var frames = m_pipeline.WaitForFrames())
            {
                OnFrames(frames);
            }
        }
        Debug.Log("RealSenseDevice thread ended");
    }

    void Update()
    {
        //Call Directly in non threaded mode
        if (processMode != ProcessMode.UnityThread)
            return;

        if (pause)
            return;

        FrameSet frames;
        if (m_pipeline.PollForFrames(out frames))
        {
            using (frames)
                OnFrames(frames);
        }
    }
}
