using Intel.RealSense;
using System;
using System.Collections.Generic;

public class RsColorizer : RsProcessingBlock
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
    private Colorizer _pb = new Colorizer();

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