using Intel.RealSense;
using System;
using System.Linq;
using System.Threading;
using UnityEngine;
using UnityEngine.Events;

public class RealsenseStreamTexture : MonoBehaviour
{
    [System.Serializable]
    public class TextureEvent : UnityEvent<Texture> { }

    public Stream sourceStreamType;
    public TextureFormat textureFormat;
    public FilterMode filterMode = FilterMode.Point;

    protected Texture2D texture;

    [Space]
    public TextureEvent textureBinding;

    [System.NonSerialized]
    byte[] data;

    readonly AutoResetEvent f = new AutoResetEvent(false);
    // int threadId;
    protected int threadId;

    virtual protected void Awake()
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
        RealSenseDevice.Instance.OnStart += OnStartStreaming;
        RealSenseDevice.Instance.OnStop += OnStopStreaming;
    }

    private void OnStopStreaming()
    {
        RealSenseDevice.Instance.onNewSample -= OnNewSampleUnityThread;
        RealSenseDevice.Instance.onNewSample -= OnNewSampleThreading;

        f.Reset();
        data = null;
    }

    private void OnStartStreaming(PipelineProfile activeProfile)
    {
        using (var profile = activeProfile.Streams.FirstOrDefault(p => p.Stream == sourceStreamType))
        {
            var videoProfile = profile as VideoStreamProfile;

            if (texture != null)
            {
                Destroy(texture);
            }

            texture = new Texture2D(videoProfile.Width, videoProfile.Height, textureFormat, false, true)
            {
                wrapMode = TextureWrapMode.Clamp,
                filterMode = filterMode
            };
            texture.Apply();
            textureBinding.Invoke(texture);
        }

        if (RealSenseDevice.Instance.processMode == RealSenseDevice.ProcessMode.UnityThread)
        {
            UnityEngine.Assertions.Assert.AreEqual(threadId, Thread.CurrentThread.ManagedThreadId);
            RealSenseDevice.Instance.onNewSample += OnNewSampleUnityThread;
        }
        else
            RealSenseDevice.Instance.onNewSample += OnNewSampleThreading;
    }

    public void OnFrame(Frame f)
    {
        if (RealSenseDevice.Instance.processMode == RealSenseDevice.ProcessMode.UnityThread)
        {
            UnityEngine.Assertions.Assert.AreEqual(threadId, Thread.CurrentThread.ManagedThreadId);
            OnNewSampleUnityThread(f);
        }
        else
        {
            OnNewSampleThreading(f);
        }
    }

    private void UpdateData(Frame frame)
    {
        // if (frame.Data == IntPtr.Zero)
        //     return;
        var vidFrame = ProcessFrame(frame) as VideoFrame;
        data = data ?? new byte[vidFrame.Stride * vidFrame.Height];
        vidFrame.CopyTo(data);
        
        if (vidFrame != frame)
            vidFrame.Dispose();
    }

    private void UploadTexture()
    {
        // if (texture == null || data == null || data.Length == 0)
        // return;
        texture.LoadRawTextureData(data);
        texture.Apply();
    }

    private void OnNewSampleThreading(Frame frame)
=======

    private void onNewSampleThreading(Frame frame)
>>>>>>> master
    {
        if (frame.Profile.Stream != sourceStreamType)
            return;
        UpdateData(frame);
        f.Set();
    }

    private void OnNewSampleUnityThread(Frame frame)
    {
        UnityEngine.Assertions.Assert.AreEqual(threadId, Thread.CurrentThread.ManagedThreadId);
        if (frame.Profile.Stream != sourceStreamType)
            return;
        var vidFrame = ProcessFrame(frame) as VideoFrame;
        texture.LoadRawTextureData(vidFrame.Data, vidFrame.Stride * vidFrame.Height);
        if (vidFrame != frame)
            vidFrame.Dispose();
        texture.Apply();
    }

    void Update()
    {
        if (f.WaitOne(0))
        {
            UploadTexture();
        }
    }
}


public class DefaultStreamAttribute : Attribute
{
    public Stream stream;
    public TextureFormat format;

    public DefaultStreamAttribute(Stream stream)
    {
        this.stream = stream;
    }

    public DefaultStreamAttribute(Stream stream, TextureFormat format)
    {
        this.stream = stream;
        this.format = format;
    }
}