using Intel.RealSense;
using System.Collections.Generic;

public class DecimationFilter : VideoProcessingBlock
{
    private List<Stream> _requirments = new List<Stream>() { Stream.Depth };
    private Intel.RealSense.DecimationFilter _pb = new Intel.RealSense.DecimationFilter();

    public override Frame Process(Frame frame)
    {
        return _enabled ? _pb.ApplyFilter(frame as VideoFrame) : frame;
    }

    public override List<Stream> Requirments()
    {
        return _requirments;
    }
}

