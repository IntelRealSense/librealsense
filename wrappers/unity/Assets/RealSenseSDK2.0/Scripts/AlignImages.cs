using UnityEngine;
using Intel.RealSense;
using System.Collections.Generic;
using System;
using System.Linq;

public class AlignImages : RealsenseStreamTexture
{

    Align aligner;

    [Space]
    public RealsenseStreamTexture alignTo;

    protected override void OnStopStreaming()
    {
        if (aligner != null)
        {
            aligner.Dispose();
            aligner = null;
        }

        RealSenseDevice.Instance.onNewSampleSet -= OnFrameSet;
    }

    protected override void OnStartStreaming(PipelineProfile activeProfile)
    {
        aligner = new Align(alignTo.sourceStreamType);

        var profile = activeProfile.Streams.FirstOrDefault(p => p.Stream == alignTo.sourceStreamType);
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

            // alignTo.OnFrame(aligned[alignTo.sourceStreamType]);
        }
    }
}
