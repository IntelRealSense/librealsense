using Intel.RealSense;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;

[ProcessingBlockDataAttribute(typeof(DecimationFilter))]
[HelpURL("https://github.com/IntelRealSense/librealsense/blob/master/doc/post-processing-filters.md#decimation-filter")]
public class RsDecimationFilter : RsProcessingBlock
{
    public Stream StreamFilter;
    public Format FormatFilter;

    /// <summary>
    /// Number of filter iterations
    /// </summary>
    [Range(2, 8)]
    [Tooltip("Number of filter iterations")]
    public int _filterMagnitude = 2;


    private DecimationFilter _pb;
    private IOption filterMag;
    private IOption streamFilter;
    private IOption formatFilter;

    public override Frame Process(Frame frame, FrameSource frameSource)
    {
        if (_pb == null)
        {
            Init();
        }

        filterMag.Value = _filterMagnitude;
        streamFilter.Value = (float)StreamFilter;
        formatFilter.Value = (float)FormatFilter;

        return _pb.Process(frame);
    }

    public void Init()
    {
        _pb = new DecimationFilter();
        filterMag = _pb.Options[Option.FilterMagnitude];
        filterMag.Value = _filterMagnitude;

        streamFilter = _pb.Options[Option.StreamFilter];
        streamFilter.Value = (float)StreamFilter;

        formatFilter = _pb.Options[Option.StreamFormatFilter];
        formatFilter.Value = (float)FormatFilter;
    }

    void OnDisable()
    {
        if (_pb != null)
        {
            _pb.Dispose();
            _pb = null;
        }
    }

    public void SetMagnitude(float val)
    {
        _filterMagnitude = (int)val;
    }
}

