using Intel.RealSense;
using System;

[Serializable]
public struct RsVideoStreamRequest : IEquatable<RsVideoStreamRequest>
{
    public Stream Stream;
    public Format Format;
    public int Framerate;
    public int StreamIndex;
    public int Width;
    public int Height;

    public RsVideoStreamRequest(Stream stream, Format format, int framerate, int streamIndex, int width, int height)
    {
        Stream = stream;
        Format = format;
        Framerate = framerate;
        StreamIndex = streamIndex;
        Width = width;
        Height = height;
    }

    public static RsVideoStreamRequest FromFrame(VideoFrame f)
    {
        using (var p = f.Profile)
            return new RsVideoStreamRequest(
                p.Stream,
                p.Format,
                p.Framerate,
                p.Index,
                f.Width,
                f.Height
            );
    }


    public static RsVideoStreamRequest FromProfile(StreamProfile p)
    {
        var isVideo = p.Is(Extension.VideoProfile);
        using (p)
        using (var v = isVideo ? p.As<VideoStreamProfile>() : null)
            return new RsVideoStreamRequest(
                p.Stream,
                p.Format,
                p.Framerate,
                p.Index,
                isVideo ? v.Width : 0,
                isVideo ? v.Height : 0
            );
    }

    public override bool Equals(object other)
    {
        return (other is RsVideoStreamRequest) && Equals((RsVideoStreamRequest)other);
    }

    public bool Equals(RsVideoStreamRequest other)
    {
        return
            Stream == other.Stream &&
            Format == other.Format &&
            Framerate == other.Framerate &&
            StreamIndex == other.StreamIndex &&
            Width == other.Width &&
            Height == other.Height;
    }

    public bool HasConflict(VideoFrame f)
    {
        var vf = f as VideoFrame;
        using (var p = vf.Profile)
        {
            if (Stream != Stream.Any && Stream != p.Stream)
                return true;
            if (Format != Format.Any && Format != p.Format)
                return true;
            if (Width != 0 && Width != vf.Width)
                return true;
            if (Height != 0 && Height != vf.Height)
                return true;
            if (Framerate != 0 && Framerate != p.Framerate)
                return true;
            if (StreamIndex != 0 && StreamIndex != p.Index)
                return true;
            return false;
        }
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

    public override int GetHashCode()
    {
        // https://stackoverflow.com/questions/263400/what-is-the-best-algorithm-for-an-overridden-system-object-gethashcode
        return new { Stream, Format, Framerate, StreamIndex, Width, Height }.GetHashCode();
    }
}
