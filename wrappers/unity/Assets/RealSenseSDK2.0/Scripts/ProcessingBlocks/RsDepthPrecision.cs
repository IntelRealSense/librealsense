using Intel.RealSense;
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using UnityEngine;

public class RsDepthPrecision : RsProcessingBlock
{
    [Range(0, 15)]
    public int _depthResolution = 0;

    private List<Stream> _requirments = new List<Stream>() { Stream.Depth };

    private short[] _pixels;
    public override ProcessingBlockType ProcessingType { get { return ProcessingBlockType.Single; } }

    private int _uniqueID { get; set; }

    public override Frame Process(Frame frame, FrameSource frameSource, FramesReleaser releaser)
    {
        if (!_enabled)
            return frame;
        var org = frame as VideoFrame;
        var stride = org.Width * org.BitsPerPixel / 8;
        var newFrame = frameSource.AllocateVideoFrame(org.Profile, org, org.BitsPerPixel, org.Width, org.Height, stride, Extension.DepthFrame);

        if (_pixels == null || org.Profile.UniqueID != _uniqueID)
            InitPixels(org);
        Marshal.Copy(org.Data, _pixels, 0, _pixels.Length);
        for (int i = 0; i < _pixels.Length; i++)
        {
            _pixels[i] = (short)(_pixels[i] >> _depthResolution);
            _pixels[i] = (short)(_pixels[i] << _depthResolution);
        }
        Marshal.Copy(_pixels, 0, newFrame.Data, _pixels.Length);
        return newFrame;
    }

    private void InitPixels(VideoFrame frame)
    {
        _uniqueID = frame.Profile.UniqueID;
        _pixels = new short[frame.Width * frame.Height];
    }

    public override List<Stream> Requirments()
    {
        return _requirments;
    }

    public void SetResolution(float val)
    {
        _depthResolution = (int)val;
    }
}
