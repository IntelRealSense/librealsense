using Intel.RealSense;
using System;
using System.Collections.Generic;

public class Aligment : MultiFrameVideoProcessingBlock
{
    public Stream _alignTo;
    private Stream _currAlignTo;
    private object _lock = new object();

    private List<Stream> _requirments = new List<Stream>() { Stream.Depth, Stream.Color };
    private Align _pb;

    public void Awake()
    {
        _pb = new Align(_alignTo);
        _currAlignTo = _alignTo;
    }

    public override FrameSet Process(FrameSet frameset, FramesReleaser releaser)
    {
        lock(_lock)
        {
            return _enabled ? _pb.Process(frameset, releaser) : frameset;
        }
    }

    public override List<Stream> Requirments()
    {
        return _requirments;
    }

    public void Update()
    {
        if (_alignTo == _currAlignTo)
            return;
        lock (_lock)
        {
            _pb = new Align(_alignTo);
            _currAlignTo = _alignTo;
        }
    }

    public void SetAlignTo(Int32 at)
    {
        //_alignTo = at;
    }
}
