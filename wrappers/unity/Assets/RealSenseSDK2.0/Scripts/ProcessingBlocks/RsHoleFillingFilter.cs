using Intel.RealSense;
using System.Collections.Generic;
using UnityEngine;

public class RsHoleFillingFilter : RsProcessingBlock
{
    /// <summary>
    /// Control the data that will be used to fill the invalid pixels
    /// [0-2] enumerated:
    /// fill_from_left - Use the value from the left neighbor pixel to fill the hole
    /// farest_from_around - Use the value from the neighboring pixel which is furthest away from the sensor
    /// nearest_from_around - - Use the value from the neighboring pixel closest to the sensor
    /// </summary>
    [Range(0, 2)]
    public int _holesFill = 0;

    private List<Stream> _requirments = new List<Stream>() { Stream.Depth };
    private HoleFillingFilter _pb = new HoleFillingFilter();

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
        _pb.Options[Option.HolesFill].Value = _holesFill;
    }

    public void SetHoleFill(float val)
    {
        _holesFill = (int)val;
    }
}
