using System.Collections;
using System.Collections.Generic;
using Intel.RealSense;
using UnityEngine;

public class PointCloudFilter : Multi2SingleVideoProcessingBlock
{
    private List<Stream> _requirments = new List<Stream>() { Stream.Depth, Stream.Color };
    private PointCloud _pb;

    public Stream _textureStream;
    private Stream _currTextureStream;

    private object _lock = new object();

    public void Awake()
    {
        _pb = new PointCloud();
        _currTextureStream = _textureStream;
    }

    public override Frame Process(FrameSet frameset, FramesReleaser releaser)
    {
        lock (_lock)
        {
            if (_enabled)
            {
                using (var depthFrame = frameset.DepthFrame)
                using (var f = frameset.FirstOrDefault<VideoFrame>(_currTextureStream))
                {
                    Points points = _pb.Calculate(depthFrame, releaser);
                    _pb.MapTexture(f);
                    return points;
                }
            }
            return null;
        }
    }

    public override List<Stream> Requirments()
    {
        return _requirments;
    }
	
	// Update is called once per frame
	void Update ()
    {
        if (_textureStream == _currTextureStream)
            return;
        lock (_lock)
        {
            _pb = new PointCloud();
            _currTextureStream = _textureStream;
        }
    }
}
