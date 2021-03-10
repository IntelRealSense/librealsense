using Intel.RealSense;

[ProcessingBlockDataAttribute(typeof(Align))]
public class RsAlign : RsProcessingBlock
{
    public Stream _alignTo = Stream.Depth;

    public bool AlignOtherToDepth { set { _alignTo = Stream.Depth; } }
    public bool AlignDepthToColor { set { _alignTo = Stream.Color; } }
    public bool AlignDepthToInfrared { set { _alignTo = Stream.Infrared; } }

    private Stream _currAlignTo;
    private Align _pb;

    public void Init()
    {
        if(_pb != null) {
            _pb.Dispose();
        }
        _pb = new Align(_alignTo);
        _currAlignTo = _alignTo;

    }

    void OnDisable()
    {
        if (_pb != null)
            _pb.Dispose();
        _pb = null;
    }

    public void AlignTo(Stream s)
    {
        _alignTo = s;
        Init();
    }

    public override Frame Process(Frame frame, FrameSource frameSource)
    {
        if (_pb == null || _alignTo != _currAlignTo)
            Init();

        return _pb.Process(frame);
    }
}
