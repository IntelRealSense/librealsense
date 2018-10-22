using Intel.RealSense;
using UnityEngine;

[ProcessingBlockDataAttribute(typeof(DisparityTransform))]
public class RsDisparityTransform : RsProcessingBlock
{
    public enum DisparityMode
    {
        DisparityToDepth = 0,
        DepthToDisparity = 1,
    }

    DisparityTransform _pb;

    [Tooltip("Stereoscopic Transformation Mode")]
    public DisparityMode Mode = DisparityMode.DepthToDisparity;

    private DisparityMode currMode;
    private readonly object _lock = new object();

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

    public void Init()
    {
        lock (_lock)
        {
            _pb = new DisparityTransform(Mode != 0);
            currMode = Mode;
        }
    }

    public override Frame Process(Frame frame, FrameSource frameSource)
    {
        lock (_lock)
        {
            if (currMode != Mode) {
                if(_pb != null) {
                    _pb.Dispose();
                    _pb = null;
                }
            }

            if (_pb == null)
                Init();

            return _pb.Process(frame);
        }
    }
}

