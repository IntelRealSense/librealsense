using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Intel.RealSense;
using System;

[DefaultStream(Stream.Depth, TextureFormat.RGB24)]
public class RealSenseColorizeDepth : RealSenseStreamTexture
{

    [Serializable]
    public enum ColorScheme //TOOD: remove and make more robust using option.ValueDescription
    {
        Jet,
        Classic,
        WhiteToBlack,
        BlackToWhite,
        Bio,
        Cold,
        Warm,
        Quantized,
        Pattern
    }

    public ColorScheme colorMap;
    private ColorScheme prevColorMap;

    Colorizer m_colorizer;

    protected override void OnStartStreaming(PipelineProfile activeProfile)
    {
        m_colorizer = new Colorizer();
        prevColorMap = colorMap;
        m_colorizer.Options[Option.ColorScheme].Value = (float)colorMap;

        base.OnStartStreaming(activeProfile);
    }

    protected override void OnStopStreaming()
    {
        base.OnStopStreaming();
        
        m_colorizer.Dispose();
    }

    override protected Frame ProcessFrame(Frame frame)
    {
        if (prevColorMap != colorMap)
        {
            try
            {
                m_colorizer.Options[Option.ColorScheme].Value = (float)colorMap;
                prevColorMap = colorMap;
            }
            catch (Exception e)
            {
                Debug.LogError("Failed to change color scheme for colorizer. " + e.Message);
                colorMap = prevColorMap;
            }
        }
        return m_colorizer.Colorize(frame as VideoFrame);
    }
}
