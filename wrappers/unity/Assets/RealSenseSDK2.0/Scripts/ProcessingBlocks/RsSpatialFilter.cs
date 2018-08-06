using Intel.RealSense;
using System.Collections.Generic;
using UnityEngine;

public class RsSpatialFilter : RsProcessingBlock
{
    /// <summary>
    /// Number of filter iterations
    /// </summary>
    [Range(1, 5)]
    public int _filterMagnitude = 2;

    /// <summary>
    /// The Alpha factor in an exponential moving average with Alpha=1 - no filter . Alpha = 0 - infinite filter
    /// </summary>
    [Range(0.25f, 1)]
    public float _filterSmoothAlpha = 0.5f;

    /// <summary>
    /// Step-size boundary. Establishes the threshold used to preserve "edges"
    /// </summary>
    [Range(1, 50)]
    public int _filterSmoothDelta = 20;

    /// <summary>
    /// An in-place heuristic symmetric hole-filling mode applied horizontally during the filter passes.
    /// Intended to rectify minor artefacts with minimal performance impact
    /// </summary>
    [Range(0, 5)]
    public int _holesFill = 0;

    private List<Stream> _requirments = new List<Stream>() { Stream.Depth };
    private SpatialFilter _pb = new SpatialFilter();

    public override ProcessingBlockType ProcessingType { get { return ProcessingBlockType.Single; } }

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
        _pb.Options[Option.FilterSmoothAlpha].Value = _filterSmoothAlpha;
        _pb.Options[Option.FilterSmoothDelta].Value = _filterSmoothDelta;
        _pb.Options[Option.HolesFill].Value = _holesFill;
    }

    public void SetMagnitude(float val)
    {
        _filterMagnitude = (int)val;
    }

    public void SetSmoothAlpha(float val)
    {
        _filterSmoothAlpha = val;
    }

    public void SetSmoothDelta(float val)
    {
        _filterSmoothDelta = (int)val;
    }

    public void SetHolesFill(float val)
    {
        _holesFill = (int)val;
    }
}
