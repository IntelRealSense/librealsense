using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Intel.RealSense;

public enum ProcessingBlockType
{
    Single,
    Multi
}

public interface IVideoProcessingBlock {
    bool IsEnabled();
    void Enable(bool state);
    List<Stream> Requirments();
}

public abstract class VideoProcessingBlock : MonoBehaviour, IVideoProcessingBlock
{
    public bool _enabled = true;
    public bool _fork = false;

    public abstract Frame Process(Frame frame);
    public abstract List<Stream> Requirments();

    public bool Fork() { return _fork; }

    public void Enable(bool state) { _enabled = state; }
    
    public bool IsEnabled() { return _enabled; }

    public void Start()
    {
        RealSenseDevice.Instance.AddProcessingBlock(this);
    }
    public bool CanProcess(Frame frame)
    {
        return Requirments().Contains(frame.Profile.Stream);
    }
}

public abstract class MultiFrameVideoProcessingBlock : MonoBehaviour, IVideoProcessingBlock
{
    public bool _enabled = true;

    public abstract FrameSet Process(FrameSet frameset, FramesReleaser releaser);
    public abstract List<Stream> Requirments();

    public void Enable(bool state) { _enabled = state; }

    public bool IsEnabled() { return _enabled; }

    public void Start()
    {
        RealSenseDevice.Instance.AddProcessingBlock(this);
    }

    public bool CanProcess(FrameSet frameset)
    {
        List<Stream> streams = new List<Stream>();

        foreach (var f in frameset)
        {
            using (f)
                streams.Add(f.Profile.Stream);
        }

        foreach (var s in Requirments())
        {
            if (!streams.Contains(s))
                return false;
        }
        return true;
    }
}