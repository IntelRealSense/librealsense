using Intel.RealSense;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading;
using UnityEngine;

public class RealsenseStreamTexture : MonoBehaviour
{
    public Stream sourceStreamType;
    public TextureFormat textureFormat;
    public FilterMode filterMode = FilterMode.Point;

    private Texture2D texture;

    [Space]
    public TextureProvider.TextureEvent textureBinding;

    [System.NonSerialized]
    byte[] data;

    readonly AutoResetEvent f = new AutoResetEvent(false);
    int threadId;

    void Awake()
    {
        threadId = Thread.CurrentThread.ManagedThreadId;
    }

    /// <summary>
    /// Called per frame before publishing it
    /// </summary>
    /// <param name="f">The frame to process</param>
    /// <returns>The processed frame</returns>
    virtual protected Frame ProcessFrame(Frame f)
    {
        return f;
    }

    void Start()
    {
        if (RealSenseDevice.Instance.ActiveProfile != null)
            OnStartStreaming(RealSenseDevice.Instance.ActiveProfile);
        else
            RealSenseDevice.Instance.OnStart += OnStartStreaming;
    }

    private void OnStartStreaming(PipelineProfile activeProfile)
    {
        var profile = activeProfile.Streams.FirstOrDefault(p => p.Stream == sourceStreamType);
        if (profile == null)
        {
            Debug.LogWarningFormat("Stream {0} not in active profile", sourceStreamType);
            return;
        }
        var videoProfile = profile as VideoStreamProfile;
        texture = new Texture2D(videoProfile.Width, videoProfile.Height, textureFormat, false, true)
        {
            wrapMode = TextureWrapMode.Clamp,
            filterMode = filterMode
        };
        texture.Apply();
        textureBinding.Invoke(texture);
        if (RealSenseDevice.Instance.processMode == RealSenseDevice.ProcessMode.UnityThread)
            RealSenseDevice.Instance.onNewSample += OnNewSampleUnityThread;
        else
            RealSenseDevice.Instance.onNewSample += onNewSampleThreading;
    }

    public void OnFrame(Frame f)
    {
        if (RealSenseDevice.Instance.processMode == RealSenseDevice.ProcessMode.UnityThread)
        {
            OnNewSampleUnityThread(f);
        }
        else
        {
            onNewSampleThreading(f);
        }
    }

    private void UpdateData(Frame frame)
    {
        if (frame.Profile.Stream != sourceStreamType)
            return;
        if (frame.Data == IntPtr.Zero)
            return;
        var vidFrame = ProcessFrame(frame) as VideoFrame;
        data = data ?? new byte[vidFrame.Stride * vidFrame.Height];
        vidFrame.CopyTo(data);
    }

    private void UploadTexture()
    {
        if (data == null || data.Length == 0)
            return;
        texture.LoadRawTextureData(data);
        texture.Apply();
    }

    private void onNewSampleThreading(Frame frame)
    {
        UpdateData(frame);
        f.Set();
    }

    private void OnNewSampleUnityThread(Frame frame)
    {
        UnityEngine.Assertions.Assert.AreEqual(threadId, Thread.CurrentThread.ManagedThreadId);
        if (frame.Profile.Stream != sourceStreamType)
            return;
        if (frame.Data == IntPtr.Zero)
            return;
        var vidFrame = ProcessFrame(frame) as VideoFrame;
        texture.LoadRawTextureData(frame.Data, vidFrame.Stride * vidFrame.Height);
        texture.Apply();
    }

    // Update is called once per frame
    void Update()
    {
        if (f.WaitOne(0))
        {
            UploadTexture();
        }
    }
}
