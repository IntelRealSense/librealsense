using System.Collections.Generic;
using System.Linq;
using Intel.RealSense;
using UnityEngine;

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

    PointCloud _pb;
    private IOption filterMag;
    public OcclusionRemoval _occlusionRemoval = OcclusionRemoval.Off;
    private readonly object _lock = new object();

    public void Init()
    {
        lock (_lock)
        {
            _pb = new PointCloud();
            filterMag = _pb.Options[Option.FilterMagnitude];
            _pb.Options[Option.StreamFilter].Value = (float)TextureStream;
            _pb.Options[Option.StreamFormatFilter].Value = (float)TextureFormat;
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

        filterMag.Value = (float)_occlusionRemoval;

        if (frame.IsComposite)
        {
            _pb.Options[Option.StreamFilter].Value = (float)TextureStream;
            _pb.Options[Option.StreamFormatFilter].Value = (float)TextureFormat;
            // _pb.Options[Option.StreamIndexFilter].Value = (float)0;
        }

        return _pb.Process(frame);
    }

}
