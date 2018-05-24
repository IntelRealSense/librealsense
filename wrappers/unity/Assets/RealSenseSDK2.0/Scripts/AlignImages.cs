using UnityEngine;
using Intel.RealSense;
using System.Collections.Generic;

public class AlignImages : MonoBehaviour
{

    Align aligner;
    public RealsenseStreamTexture from;
    public RealsenseStreamTexture to;

    // Use this for initialization
    void Start()
    {
        aligner = new Align(Stream.Color);
        RealSenseDevice.Instance.onNewSampleSet += OnFrameSet;
    }

    private void OnFrameSet(FrameSet frames)
    {
        using (var aligned = aligner.Process(frames))
        {
            using (var depth = aligned.DepthFrame)
                from.OnFrame(depth);

            using (var color = aligned.ColorFrame)
                to.OnFrame(color);
        }
    }

    // Update is called once per frame
    void Update()
    {
        //TODO:
    }
}
