using Intel.RealSense;
using System.Collections.Generic;

public class HoleFillingFilter : VideoProcessingBlock
{
    private List<Stream> _requirments = new List<Stream>() { Stream.Depth };
    private Intel.RealSense.HoleFillingFilter _pb = new Intel.RealSense.HoleFillingFilter();

    public override Frame Process(Frame frame)
    {
        return _enabled ? _pb.ApplyFilter(frame as VideoFrame) : frame;
    }

    public override List<Stream> Requirments()
    {
        return _requirments;
    }
}
