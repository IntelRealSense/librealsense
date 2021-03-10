using System;
using Intel.RealSense;

[Serializable]
public struct RsConfiguration
{
    public enum Mode
    {
        Live, Playback, Record
    }

    public Mode mode;
    public RsVideoStreamRequest[] Profiles;
    public string RequestedSerialNumber;
    public string PlaybackFile;
    public string RecordPath;


    public Config ToPipelineConfig()
    {
        Config cfg = new Config();

        switch (mode)
        {
            case Mode.Live:
                cfg.EnableDevice(RequestedSerialNumber);
                foreach (var p in Profiles)
                    cfg.EnableStream(p.Stream, p.StreamIndex, p.Width, p.Height, p.Format, p.Framerate);
                break;

            case Mode.Playback:
                if (String.IsNullOrEmpty(PlaybackFile))
                {
                    mode = Mode.Live;
                }
                else
                {
                    cfg.EnableDeviceFromFile(PlaybackFile);
                }
                break;

            case Mode.Record:
                foreach (var p in Profiles)
                    cfg.EnableStream(p.Stream, p.StreamIndex, p.Width, p.Height, p.Format, p.Framerate);
                if (!String.IsNullOrEmpty(RecordPath))
                    cfg.EnableRecordToFile(RecordPath);
                break;

        }

        return cfg;
    }
}