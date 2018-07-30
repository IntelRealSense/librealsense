using Intel.RealSense;
using System;

[Serializable]
public class RsVideoStreamRequest
{
    public Stream Stream;
    public Format Format;
    public int Framerate;
    public int StreamIndex;
    public int Width;
    public int Height;

    public RsVideoStreamRequest()
    {

    }

    public RsVideoStreamRequest(VideoFrame f)
    {
        this.CopyProfile(f);
    }

    public RsVideoStreamRequest Clone()
    {
        return new RsVideoStreamRequest()
        {
            Stream = this.Stream,
            Format = this.Format,
            Framerate = this.Framerate,
            StreamIndex = this.StreamIndex,
            Width = this.Width,
            Height = this.Height
        };
    }

    public void CopyProfile(VideoFrame f)
    {
        var vf = f as VideoFrame;

        Stream = vf.Profile.Stream;
        Format = vf.Profile.Format;
        Framerate = vf.Profile.Framerate;
        StreamIndex = vf.Profile.Index;
        Width = vf.Width;
        Height = vf.Height;
    }

    public override bool Equals(object other)
    {
        if (!(other is RsVideoStreamRequest))
            return false;
        RsVideoStreamRequest vsr = other as RsVideoStreamRequest;
        if (Stream != vsr.Stream)
            return false;
        if (Format != vsr.Format)
            return false;
        if (Width != vsr.Width)
            return false;
        if (Height != vsr.Height)
            return false;
        if (Framerate != vsr.Framerate)
            return false;
        if (StreamIndex != vsr.StreamIndex)
            return false;
        return true;
    }

    public bool HasConflict(VideoFrame f)
    {
        var vf = f as VideoFrame;
        if (Stream != Stream.Any && Stream != vf.Profile.Stream)
            return true;
        if (Format != Format.Any && Format != vf.Profile.Format)
            return true;
        if (Width != 0 && Width != vf.Width)
            return true;
        if (Height != 0 && Height != vf.Height)
            return true;
        if (Framerate != 0 && Framerate != vf.Profile.Framerate)
            return true;
        if (StreamIndex != 0 && StreamIndex != vf.Profile.Index)
            return true;
        return false;
    }

    public bool HasConflict(RsVideoStreamRequest other)
    {
        if (Stream != Stream.Any && Stream != other.Stream)
            return true;
        if (Format != Format.Any && Format != other.Format)
            return true;
        if (Width != 0 && Width != other.Width)
            return true;
        if (Height != 0 && Height != other.Height)
            return true;
        if (Framerate != 0 && Framerate != other.Framerate)
            return true;
        if (StreamIndex != 0 && StreamIndex != other.StreamIndex)
            return true;
        return false;
    }
}
