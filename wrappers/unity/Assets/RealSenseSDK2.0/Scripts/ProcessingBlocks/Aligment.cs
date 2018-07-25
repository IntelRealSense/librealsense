﻿using Intel.RealSense;
using System;
using System.Collections.Generic;

public class Aligment : MultiFrameVideoProcessingBlock
{
    public Stream _alignTo;

    private Stream _currAlignTo;
    private Align _pb;
    private List<Stream> _requirments = new List<Stream>() { Stream.Depth, Stream.Color };

    private object _lock = new object();

    private Dictionary<Stream, int> _profilesIds = new Dictionary<Stream, int>();
    public void Awake()
    {
        ResetAligner();
    }

    private void ResetAligner()
    {
        if(_pb != null)
            _pb.Dispose();
        _pb = new Align(_alignTo);
        _currAlignTo = _alignTo;
    }

    public override FrameSet Process(FrameSet frameset, FramesReleaser releaser)
    {
        lock (_lock)
        {
            using (var depth = frameset.DepthFrame)
            using (var color = frameset.ColorFrame)
                if (_profilesIds.Count == 0 != !_profilesIds.ContainsValue(color.Profile.UniqueID) || !_profilesIds.ContainsValue(depth.Profile.UniqueID))
                {
                    ResetAligner();
                    _profilesIds[Stream.Depth] = depth.Profile.UniqueID;
                    _profilesIds[Stream.Color] = color.Profile.UniqueID;
                }
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
            ResetAligner();
        }
    }

    public void ToggleAlignTo()
    {
        _alignTo = _alignTo == Stream.Color ? Stream.Depth : Stream.Color;
    }
}
