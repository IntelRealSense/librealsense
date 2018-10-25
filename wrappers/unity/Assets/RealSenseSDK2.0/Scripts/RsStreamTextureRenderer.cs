using Intel.RealSense;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Threading;
using UnityEngine;
using UnityEngine.Events;

public class RsStreamTextureRenderer : MonoBehaviour
{
    private static TextureFormat Convert(Format lrsFormat)
    {
        switch (lrsFormat)
        {
            case Format.Z16: return TextureFormat.R16;
            case Format.Disparity16: return TextureFormat.R16;
            case Format.Rgb8: return TextureFormat.RGB24;
            case Format.Rgba8: return TextureFormat.RGBA32;
            case Format.Bgra8: return TextureFormat.BGRA32;
            case Format.Y8: return TextureFormat.Alpha8;
            case Format.Y16: return TextureFormat.R16;
            case Format.Raw16: return TextureFormat.R16;
            case Format.Raw8: return TextureFormat.Alpha8;
            case Format.Disparity32: return TextureFormat.RFloat;
            case Format.Yuyv:
            case Format.Bgr8:
            case Format.Raw10:
            case Format.Xyz32f:
            case Format.Uyvy:
            case Format.MotionRaw:
            case Format.MotionXyz32f:
            case Format.GpioRaw:
            case Format.Any:
            default:
                throw new ArgumentException(string.Format("librealsense format: {0}, is not supported by Unity", lrsFormat));
        }
    }

    public RsFrameProvider Source;

    [System.Serializable]
    public class TextureEvent : UnityEvent<Texture> { }

    public Stream _stream;
    public Format _format;
    public int _streamIndex;

    public FilterMode filterMode = FilterMode.Point;

    protected Texture2D texture;


    [Space]
    public TextureEvent textureBinding;

    FrameQueue q;
    Predicate<Frame> matcher;

    void Start()
    {
        Source.OnStart += OnStartStreaming;
        Source.OnStop += OnStopStreaming;
    }

    void OnDestroy()
    {
        if (texture != null)
        {
            Destroy(texture);
            texture = null;
        }

        if (q != null)
        {
            q.Dispose();
        }
    }

    protected void OnStopStreaming()
    {
        Source.OnNewSample -= OnNewSample;
        // RsDevice.Instance.OnNewSampleSet -= OnNewSampleSet;

        // e.Set();

        if (q != null)
        {
            // foreach (var f in q)
            // f.Dispose();

            q.Dispose();
            q = null;
        }
    }




    public void OnStartStreaming(PipelineProfile activeProfile)
    {
        q = new FrameQueue(1);

        matcher = new Predicate<Frame>(Matches);

        Source.OnNewSample += OnNewSample;
        // e.Reset();

        // RsDevice.Instance.OnNewSampleSet += OnNewSampleSet;
    }

    private bool Matches(Frame f)
    {
        using (var p = f.Profile)
            return p.Stream == _stream && p.Format == _format && p.Index == _streamIndex;
    }

    void OnNewSample(Frame frame)
    {
        try
        {
            if (frame.IsComposite)
            {
                using (var fs = FrameSet.FromFrame(frame))
                // using (var f = fs[_stream, _format, _streamIndex])
                using (var f = fs.FirstOrDefault<VideoFrame>(matcher))
                {
                    if (f != null)
                        q.Enqueue(f);
                    return;
                }
            }

            // using (var p = frame.Profile)
            // {
            //     if (p.Stream != _stream || p.Format != _format || p.Index != _streamIndex)
            //     {
            //         return;
            //     }
            // }
            if (!matcher(frame))
                return;

            using (frame)
            {
                q.Enqueue(frame);
            }
        }
        catch (Exception e)
        {
            Debug.LogException(e, this);
            // throw;
        }

    }

    bool HasTextureConflict(VideoFrame vf)
    {
        using (var p = vf.Profile)
        {
            return !texture ||
                texture.width != vf.Width ||
                texture.height != vf.Height ||
                Convert(p.Format) != texture.format;
        }
    }

    protected void Update()
    {
        // if (e.WaitOne(0, false))
        // return;

        if (q != null)
        {
            Frame frame;
            if (q.PollForFrame(out frame))
                using (frame)
                    ProcessFrame(frame as VideoFrame);
        }
    }

    private void ProcessFrame(VideoFrame frame)
    {
        if (HasTextureConflict(frame))
        {
            if (texture != null)
            {
                Destroy(texture);
            }

            texture = new Texture2D(frame.Width, frame.Height, Convert(frame.Profile.Format), false, true)
            {
                wrapMode = TextureWrapMode.Clamp,
                filterMode = filterMode
            };

            textureBinding.Invoke(texture);
        }

        texture.LoadRawTextureData(frame.Data, frame.Stride * frame.Height);
        texture.Apply();
    }
}
