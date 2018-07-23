using Intel.RealSense;
using System.Collections.Generic;

public class SpatialFilter : VideoProcessingBlock
{
    private List<Stream> _requirments = new List<Stream>() { Stream.Depth };
    private Intel.RealSense.SpatialFilter _pb = new Intel.RealSense.SpatialFilter();

    public override Frame Process(Frame frame)
    {
        return _enabled ? _pb.ApplyFilter(frame as VideoFrame) : frame;
    }

    public override List<Stream> Requirments()
    {
        return _requirments;
    }
}
