using Intel.RealSense;
using UnityEngine;

[ProcessingBlockDataAttribute(typeof(Colorizer))]
public class RsColorizer : RsProcessingBlock
{
    public enum ColorScheme
    {
        Jet,
        Classic,
        WhiteToBlack,
        BlackToWhite,
        Bio,
        Cold,
        Warm,
        Quantized,
        Pattern,
        Hue
    }

    public enum VisualPreset
    {
        Dynamic = 0,
        Fixed = 1,
        Near = 2,
        Far = 3,
    }

    private Colorizer _pb;

    public VisualPreset visualPreset = VisualPreset.Dynamic;
    public ColorScheme colorScheme = ColorScheme.Jet;

    public bool histogramEqualization = true;

    [Range(0, 16)]
    public float minDist = 0f;

    [Range(0, 16)]
    public float maxDist = 6f;

    private IOption presetOption;
    private IOption schemeOption;
    private IOption histEqOption;
    private IOption minDistOption;
    private IOption maxDistOption;


    public void Init()
    {
        _pb = new Colorizer();
        presetOption = _pb.Options[Option.VisualPreset];
        schemeOption = _pb.Options[Option.ColorScheme];
        histEqOption = _pb.Options[Option.HistogramEqualizationEnabled];
        minDistOption = _pb.Options[Option.MinDistance];
        maxDistOption = _pb.Options[Option.MaxDistance];
    }

    void OnDisable()
    {
        if (_pb != null)
        {
            _pb.Dispose();
        }
    }


    public override Frame Process(Frame frame, FrameSource frameSource)
    {
        if (_pb == null)
        {
            Init();
        }

        UpdateOptions();

        return _pb.Process(frame);
    }

    private void UpdateOptions()
    {
        if (presetOption.Value != (float)visualPreset)
        {
            presetOption.Value = (float)visualPreset;

            colorScheme = (ColorScheme)schemeOption.Value;
            histogramEqualization = histEqOption.Value != 0f;
            minDist = minDistOption.Value;
            maxDist = maxDistOption.Value;
        }
        else
        {
            if (schemeOption.Value != (float)colorScheme)
                schemeOption.Value = (float)colorScheme;

            if (histEqOption.Value != (float)(histogramEqualization ? 1 : 0))
                histEqOption.Value = (float)(histogramEqualization ? 1 : 0);

            if (minDistOption.Value != minDist)
                minDistOption.Value = minDist;

            if (maxDistOption.Value != maxDist)
                maxDistOption.Value = maxDist;
        }
    }
}