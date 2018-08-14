using Intel.RealSense;
using System.Collections.Generic;
using UnityEngine;

public class RsTemporalFilter : RsProcessingBlock
{
    /// <summary>
    /// The Alpha factor in an exponential moving average with Alpha=1 - no filter . Alpha = 0 - infinite filter
    /// </summary>
    [Range(0, 1)]
    public float _filterSmoothAlpha = 0.4f;

    /// <summary>
    /// Step-size boundary. Establishes the threshold used to preserve surfaces (edges)
    /// </summary>
    [Range(1, 100)]
    public int _filterSmoothDelta = 20;

    /// <summary>
    /// A set of predefined rules (masks) that govern when missing pixels will be replace with the last valid value so that the data will remain persistent over time:
    /// Disabled - Persistency filter is not activated and no hole filling occurs.
    /// Valid in 8/8 - Persistency activated if the pixel was valid in 8 out of the last 8 frames
    /// Valid in 2/last 3 - Activated if the pixel was valid in two out of the last 3 frames
    /// Valid in 2/last 4 - Activated if the pixel was valid in two out of the last 4 frames
    /// Valid in 2/8 - Activated if the pixel was valid in two out of the last 8 frames
    /// Valid in 1/last 2 - Activated if the pixel was valid in one of the last two frames
    /// Valid in 1/last 5 - Activated if the pixel was valid in one out of the last 5 frames
    /// Valid in 1/last 8 - Activated if the pixel was valid in one out of the last 8 frames
    /// Persist Indefinitely - Persistency will be imposed regardless of the stored history(most aggressive filtering)
    /// </summary>
    [Range(0, 8)]
    public int _temporalPersistence = 3;

    private List<Stream> _requirments = new List<Stream>() { Stream.Depth };
    private TemporalFilter _pb = new TemporalFilter();

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
        _pb.Options[Option.FilterSmoothAlpha].Value = _filterSmoothAlpha;
        _pb.Options[Option.FilterSmoothDelta].Value = _filterSmoothDelta;
        _pb.Options[Option.HolesFill].Value = _temporalPersistence;
    }

    public void SetSmoothAlpha(float val)
    {
        _filterSmoothAlpha = val;
    }

    public void SetSmoothDelta(float val)
    {
        _filterSmoothDelta = (int)val;
    }

    public void SetTemporalPersistence(float val)
    {
        _temporalPersistence = (int)val;
    }
}
