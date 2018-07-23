using Intel.RealSense;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Threading;
using UnityEngine;
using UnityEngine.Events;

public class RealSenseStreamTexture : MonoBehaviour
{
    private static TextureFormat Convert(Format lrsFormat)
    {
        switch (lrsFormat)
        {
            case Format.Z16: return TextureFormat.R16;
            case Format.Disparity16: return TextureFormat.R16;
            case Format.Rgb8: return TextureFormat.RGB24;
            case Format.Bgr8: return TextureFormat.RGB24;
            case Format.Rgba8: return TextureFormat.RGBA32;
            case Format.Bgra8: return TextureFormat.BGRA32;
            case Format.Y8: return TextureFormat.Alpha8;
            case Format.Y16: return TextureFormat.R16;
            case Format.Raw16: return TextureFormat.R16;
            case Format.Raw8: return TextureFormat.Alpha8;
            case Format.Raw10: 
            case Format.Xyz32f:
            case Format.Yuyv:
            case Format.Uyvy:
            case Format.MotionRaw:
            case Format.MotionXyz32f:
            case Format.GpioRaw: 
            case Format.Any: 
            default:
                throw new Exception(string.Format("{0} librealsense format: " + lrsFormat + ", is not supported by Unity"));
        }
    }
    [System.Serializable]
    public class TextureEvent : UnityEvent<Texture> { }

    /// <summary>
    /// This field set the request which the frames are filtered by, leave ANY/0 to accept any argument. 
    /// </summary>
    public VideoStreamRequest _videoStreamFilter;
    private VideoStreamRequest _currVideoStreamFilter;

    public FilterMode filterMode = FilterMode.Point;

    protected Texture2D texture;

    [Space]
    public TextureEvent textureBinding;

    [System.NonSerialized]
    private byte[] data;

    private object _lock = new object();

    readonly AutoResetEvent f = new AutoResetEvent(false);
    protected int threadId;

    virtual protected void Awake()
    {
        threadId = Thread.CurrentThread.ManagedThreadId;
        _currVideoStreamFilter = _videoStreamFilter.Clone();
    }

    void Start()
    {
        RealSenseDevice.Instance.OnStart += OnStartStreaming;
        RealSenseDevice.Instance.OnStop += OnStopStreaming;
    }

    protected virtual void OnStopStreaming()
    {
        RealSenseDevice.Instance.OnNewSample -= OnNewSampleUnityThread;
        RealSenseDevice.Instance.OnNewSample -= OnNewSampleThreading;

        f.Reset();
        data = null;
    }

    protected virtual void OnStartStreaming(PipelineProfile activeProfile)
    {

        if (RealSenseDevice.Instance.processMode == RealSenseDevice.ProcessMode.UnityThread)
        {
            UnityEngine.Assertions.Assert.AreEqual(threadId, Thread.CurrentThread.ManagedThreadId);
            RealSenseDevice.Instance.OnNewSample += OnNewSampleUnityThread;
        }
        else
            RealSenseDevice.Instance.OnNewSample += OnNewSampleThreading;
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
        lock (_lock)
        {
            var vidFrame = frame as VideoFrame;
            data = data ?? new byte[vidFrame.Stride * vidFrame.Height];
            vidFrame.CopyTo(data);
            if ((vidFrame as Frame) != frame)
                vidFrame.Dispose();
        }
    }

    private void SetTexture(VideoStreamRequest vsr)
    {
        lock (_lock)
        {
            if (texture != null)
            {
                Destroy(texture);
            }

            texture = new Texture2D(vsr.Width, vsr.Height, Convert(vsr.Format), false, true)
            {
                wrapMode = TextureWrapMode.Clamp,
                filterMode = filterMode
            };

            _currVideoStreamFilter = vsr.Clone();

            texture.Apply();
            textureBinding.Invoke(texture);
        }
    }

    private bool HasTextureConflict(Frame frame)
    {
        var vidFrame = frame as VideoFrame;
        if (_videoStreamFilter.Width == vidFrame.Width && _videoStreamFilter.Height == vidFrame.Height && _videoStreamFilter.Format == vidFrame.Profile.Format)
            return false;
        _videoStreamFilter.CopyProfile(vidFrame);
        return true;
    }

    private void OnNewSampleThreading(Frame frame)
    {
        var vidFrame = frame as VideoFrame;
        if (_videoStreamFilter.Stream != frame.Profile.Stream ||
            _videoStreamFilter.Format != vidFrame.Profile.Format ||
            _videoStreamFilter.StreamIndex != vidFrame.Profile.Index)
            return;
        if (HasTextureConflict(frame))
            return;
        UpdateData(frame);
        f.Set();
    }

    private void OnNewSampleUnityThread(Frame frame)
    {
        var vidFrame = frame as VideoFrame;

        if (_videoStreamFilter.Stream != frame.Profile.Stream ||
           _videoStreamFilter.Format != vidFrame.Profile.Format ||
           _videoStreamFilter.StreamIndex != vidFrame.Profile.Index)
            return;
        if (HasTextureConflict(frame))
            return;

        UnityEngine.Assertions.Assert.AreEqual(threadId, Thread.CurrentThread.ManagedThreadId);

        texture.LoadRawTextureData(vidFrame.Data, vidFrame.Stride * vidFrame.Height);

        if ((vidFrame as Frame) != frame)
            vidFrame.Dispose();

        // Applied later (in Update) to sync all gpu uploads
        // texture.Apply();
        f.Set();
    }

    void Update()
    {
        if(!_currVideoStreamFilter.Equals(_videoStreamFilter))
            SetTexture(_videoStreamFilter);

        if (f.WaitOne(0))
        {
            try
            {
                if (data != null)
                    texture.LoadRawTextureData(data);
            }
            catch
            {
                OnStopStreaming();
                Debug.LogError("Error loading texture data, check texture and stream formats");
                throw;
            }
            texture.Apply();
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