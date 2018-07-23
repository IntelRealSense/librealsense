using Intel.RealSense;
using System.Collections.Generic;

public class TemporalFilter : VideoProcessingBlock
{
    private List<Stream> _requirments = new List<Stream>() { Stream.Depth };
    private Intel.RealSense.TemporalFilter _pb = new Intel.RealSense.TemporalFilter();

    public override Frame Process(Frame frame)
    {
        return _enabled ? _pb.ApplyFilter(frame as VideoFrame) : frame;
    }

    public override List<Stream> Requirments()
    {
        return _requirments;
    }
}
