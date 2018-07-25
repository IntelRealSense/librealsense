﻿using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Intel.RealSense;

public enum ProcessingBlockType
{
    Single,
    Multi
}

public interface IVideoProcessingBlock {
    bool Enabled { get; }
    int Order { get; }

    List<Stream> Requirments();
}

public abstract class VideoProcessingBlock : MonoBehaviour, IVideoProcessingBlock
{
    protected bool _enabled = true;
    public int _order = 0;
    public bool _fork = false;

    public bool Enabled { get { return _enabled; } }
    public int Order { get { return _order; } }

    public abstract Frame Process(Frame frame);
    public abstract List<Stream> Requirments();

    public bool Fork() { return _fork; }

    public bool CanProcess(Frame frame)
    {
        return Requirments().Contains(frame.Profile.Stream);
    }

    public void OnEnable()
    {
        _enabled = true;
        RealSenseDevice.Instance.AddProcessingBlock(this);
    }

    public void OnDisable()
    {
        _enabled = false;
        RealSenseDevice.Instance.RemoveProcessingBlock(this);
    }
}

public abstract class MultiFrameVideoProcessingBlock : MonoBehaviour, IVideoProcessingBlock
{
    protected bool _enabled = true;
    public int _order = 0;

    public bool Enabled { get { return _enabled; } }
    public int Order { get { return _order; } }

    public abstract FrameSet Process(FrameSet frameset, FramesReleaser releaser);
    public abstract List<Stream> Requirments();

    public void OnEnable()
    {
        _enabled = true;
        RealSenseDevice.Instance.AddProcessingBlock(this);
    }

    public void OnDisable()
    {
        _enabled = false;
        RealSenseDevice.Instance.RemoveProcessingBlock(this);
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