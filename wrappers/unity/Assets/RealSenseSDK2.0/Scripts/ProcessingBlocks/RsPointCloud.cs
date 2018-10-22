using Intel.RealSense;

[ProcessingBlockDataAttribute(typeof(PointCloud))]
public class RsPointCloud : RsProcessingBlock
{
    public enum OcclusionRemoval
    {
        Off = 0,
        Heuristic = 1,
        Exhaustive = 2
    }

    public Stream TextureStream = Stream.Color;
    public Format TextureFormat = Format.Any;
    public OcclusionRemoval _occlusionRemoval = OcclusionRemoval.Off;

    PointCloud _pb;
    private IOption filterMag;
    private IOption streamFilter;
    private IOption formatFilter;

    private readonly object _lock = new object();

    public void Init()
    {
        lock (_lock)
        {
            _pb = new PointCloud();
            filterMag = _pb.Options[Option.FilterMagnitude];
            streamFilter = _pb.Options[Option.StreamFilter];
            formatFilter = _pb.Options[Option.StreamFormatFilter];
        }
    }

    void OnDisable()
    {
        lock (_lock)
        {
            if (_pb != null)
            {
                _pb.Dispose();
                _pb = null;
            }
        }
    }

    public override Frame Process(Frame frame, FrameSource frameSource)
    {
        lock (_lock)
        {
            if (_pb == null)
            {
                Init();
            }
        }

        UpdateOptions(frame.IsComposite);

        return _pb.Process(frame);
    }

    private void UpdateOptions(bool isComposite)
    {
        filterMag.Value = (float)_occlusionRemoval;
        if (isComposite)
        {
            streamFilter.Value = (float)TextureStream;
            formatFilter.Value = (float)TextureFormat;
        }
    }
}
