using System;
using Intel.RealSense;

[Serializable]
public struct VideoStreamRequest
{
    public Stream Stream;
    public Format Format;
    public int Framerate;
    public int StreamIndex;
    public int Width;
    public int Height;
}

[Serializable]
public struct OptionValue
{
    public Option option;
    public float value;
}

[Serializable]
public struct RealSenseConfiguration
{
    public VideoStreamRequest[] Profiles;
    public string RequestedSerialNumber;
    //public string PlaybackFile;
    //public string RecordPath;
    public OptionValue[] SensorOptions;


    public Config ToPipelineConfig()
    {
        Config cfg = new Config();
        if (!String.IsNullOrEmpty(RequestedSerialNumber))
        {
            cfg.EnableDevice(RequestedSerialNumber);
        }
        //if (!String.IsNullOrEmpty(PlaybackFile))
        //{
        //    cfg.EnableDeviceFromFile(PlaybackFile);
        //}
        //if (!String.IsNullOrEmpty(RecordPath))
        //{
        //    cfg.EnableRecordToFile(RecordPath);
        //}
        foreach (var p in Profiles)
        {
            cfg.EnableStream(p.Stream, p.StreamIndex, p.Width, p.Height, p.Format, p.Framerate);
        }
        return cfg;
    }
}