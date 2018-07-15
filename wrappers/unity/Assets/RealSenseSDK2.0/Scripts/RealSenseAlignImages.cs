using UnityEngine;
using Intel.RealSense;
using System.Collections.Generic;
using System;
using System.Linq;

public class RealSenseAlignImages : RealSenseStreamTexture
{

    Align aligner;

    [Space]
    public Stream alignTo;

    void OnDestroy()
    {
        OnStopStreaming();
    }

    protected override void OnStopStreaming()
    {
        if (aligner != null)
        {
            aligner.Dispose();
            aligner = null;
        }
    }

    protected override void OnStartStreaming(PipelineProfile activeProfile)
    {
        aligner = new Align(alignTo);

        var profile = activeProfile.Streams.FirstOrDefault(p => p.Stream == alignTo);
        if (profile == null)
        {
            Debug.LogWarningFormat("Stream {0} not in active profile", sourceStreamType);
            return;
        }
        var videoProfile = profile as VideoStreamProfile;
        texture = new Texture2D(videoProfile.Width, videoProfile.Height, textureFormat, false, true)
        {
            wrapMode = TextureWrapMode.Clamp,
            filterMode = filterMode
        };
        texture.Apply();
        textureBinding.Invoke(texture);

        RealSenseDevice.Instance.onNewSampleSet += OnFrameSet;
    }

    private void OnFrameSet(FrameSet frames)
    {
        using (var aligned = aligner.Process(frames))
        {
            using (var f = aligned[sourceStreamType])
                OnFrame(f);
        }
    }
}
