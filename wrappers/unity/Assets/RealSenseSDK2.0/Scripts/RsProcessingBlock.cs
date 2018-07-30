using System;
using System.Collections.Generic;
using UnityEngine;
using Intel.RealSense;

[Serializable]
public enum ProcessingBlockType
{
    Single,
    Multi
}

[Serializable]
public abstract class RsProcessingBlock : MonoBehaviour
{
    private RsProcessingPipe _processingPipe;

    protected bool _enabled = false;
    public int _order = 0;
    public bool _fork = false;

    public bool Enabled { get { return _enabled; } }
    public int Order { get { return _order; } }

    public abstract ProcessingBlockType ProcessingType { get; }

    public abstract Frame Process(Frame frame, FrameSource frameSource, FramesReleaser releaser);
    public abstract List<Stream> Requirments();

    public bool Fork() { return _fork; }

    public void Start()
    {
        ChangeState(true);
    }

    public void OnEnable()
    {
        ChangeState(true);
    }

    public void OnDisable()
    {
        ChangeState(false);
    }

    private void ChangeState(bool state)
    {
        _enabled = state;
        if(_processingPipe == null)
            _processingPipe = GetComponent<RsProcessingPipe>();
        if (_processingPipe == null)
        {
            Debug.LogWarning(this.name + " is not binded to a processing pipe");
            return;
        }
        if (state)
            _processingPipe.AddProcessingBlock(this);
        else
            _processingPipe.RemoveProcessingBlock(this);
    }

    public bool CanProcess(Frame frame)
    {
        if(ProcessingType == ProcessingBlockType.Single)
            return Requirments().Contains(frame.Profile.Stream);

        using (var frameset = FrameSet.FromFrame(frame))
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
}