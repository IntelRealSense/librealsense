using Intel.RealSense;
using System;
using System.Collections.Generic;
using UnityEngine;

public class ColorizerFilter : VideoProcessingBlock
{
    [Serializable]
    public enum ColorScheme //TOOD: remove and make more robust using option.ValueDescription
    {
        Jet,
        Classic,
        WhiteToBlack,
        BlackToWhite,
        Bio,
        Cold,
        Warm,
        Quantized,
        Pattern
    }

    public ColorScheme colorMap;
    private List<Stream> _requirments = new List<Stream>() { Stream.Depth };
    private Intel.RealSense.Colorizer _pb = new Intel.RealSense.Colorizer();

    public override ProcessingBlockType ProcessingType { get { return ProcessingBlockType.Single; } }

    public void Awake()
    {
        _pb.Options[Option.ColorScheme].Value = (float)colorMap;
    }

    public override List<Stream> Requirments()
    {
        return _requirments;
    }

    public void Update()
    {
        _pb.Options[Option.ColorScheme].Value = (float)colorMap;
    }

    public override Frame Process(Frame frame, FrameSource frameSource, FramesReleaser releaser)
    {
        return _enabled ? _pb.Colorize(frame as VideoFrame) : frame;
    }
}