using Intel.RealSense;
using System.Collections.Generic;
using UnityEngine;

public class RsDecimationFilter : RsProcessingBlock
{
    /// <summary>
    /// Number of filter iterations
    /// </summary>
    [Range(2, 8)]
    public int _filterMagnitude = 2;

    public override ProcessingBlockType ProcessingType { get { return ProcessingBlockType.Single; } }

    private List<Stream> _requirments = new List<Stream>() { Stream.Depth };
    private DecimationFilter _pb = new DecimationFilter();

    public override Frame Process(Frame frame, FrameSource frameSource, FramesReleaser releaser)
    {
        return _enabled ? _pb.ApplyFilter(frame as VideoFrame) : frame;
    }

    public override List<Stream> Requirments()
    {
        return _requirments;
    }

    public void Update()
    {
        _pb.Options[Option.FilterMagnitude].Value = _filterMagnitude;
    }

    public void SetMagnitude(float val)
    {
        _filterMagnitude = (int)val;
    }
}

