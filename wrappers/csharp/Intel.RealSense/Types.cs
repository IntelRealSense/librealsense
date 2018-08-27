using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void frame_callback(IntPtr frame, IntPtr user_data);
    //public delegate void frame_callback([Out, MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(FrameMarshaler))] out Frame frame, IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void frame_processor_callback(IntPtr frame, IntPtr user, IntPtr user_data);

    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void frame_deleter(IntPtr frame);
    
    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void rs2_devices_changed_callback(IntPtr removed, IntPtr added, IntPtr user_data);

    public enum NotificationCategory
    {
        FramesTimeout = 0,
        FrameCorrupted = 1,
        HardwareError = 2,
        UnknownError = 3,
    }

    public enum ExceptionType
    {
        Unknown = 0,
        CameraDisconnected = 1,
        Backend = 2,
        InvalidValue = 3,
        WrongApiCallSequence = 4,
        NotImplemented = 5,
        DeviceInRecoveryMode = 6,
        Io = 7,
    }

    public enum Distortion
    {
        None = 0,
        ModifiedBrownConrady = 1,
        InverseBrownConrady = 2,
        Ftheta = 3,
        BrownConrady = 4,
    }

    public enum LogSeverity
    {
        Debug = 0,
        Info = 1,
        Warn = 2,
        Error = 3,
        Fatal = 4,
        None = 5,
    }

    public enum Extension
    {
        Unknown = 0,            
        Debug = 1,
        Info = 2,
        Motion = 3,
        Options = 4,
        Video = 5,
        Roi = 6,
        DepthSensor = 7,
        VideoFrame = 8,
        MotionFrame = 9,
        CompositeFrame = 10,
        Points = 11,
        DepthFrame = 12,
        AdvancedMode = 13,
        Record = 14,
        VideoProfile = 15,
        Playback = 16,        
    }

    public enum CameraInfo
    {
        Name = 0,
        SerialNumber = 1,
        RecommendedFirmwareVersion = 2,
        FirmwareVersion = 3,
        PhysicalPort = 4,
        DebugOpCode = 5,
        AdvancedMode = 6,
        ProductId = 7,
        CameraLocked = 8,
        UsbTypeDescriptor = 9,
    }

    public enum Stream
    {
        Any = 0,
        Depth = 1,
        Color = 2,
        Infrared = 3,
        Fisheye = 4,
        Gyro = 5,
        Accel = 6,
        Gpio = 7,
        Pose = 8,
        Confidence = 9,
    }

    public enum Format
    {
        Any = 0,
        Z16 = 1,
        Disparity16 = 2,
        Xyz32f = 3,
        Yuyv = 4,
        Rgb8 = 5,
        Bgr8 = 6,
        Rgba8 = 7,
        Bgra8 = 8,
        Y8 = 9,
        Y16 = 10,
        Raw10 = 11,
        Raw16 = 12,
        Raw8 = 13,
        Uyvy = 14,
        MotionRaw = 15,
        MotionXyz32f = 16,
        GpioRaw = 17,
    }

    public enum TimestampDomain
    {
        HardwareClock = 0,
        SystemTime = 1,
    }

    public enum FrameMetadataValue
    {
        FrameCounter = 0,
        FrameTimestamp = 1,
        SensorTimestamp = 2,
        ActualExposure = 3,
        GainLevel = 4,
        AutoExposure = 5,
        WhiteBalance = 6,
        TimeOfArrival = 7,
        Temperature = 8,
        BackendTimestamp = 9,
        ActualFps = 10,
        FrameLaserPower = 11,
        FrameLaserPowerMode = 12,
        ExposurePriority = 13,
        ExposureRoiLeft = 14,
        ExposureRoiRight = 15,
        ExposureRoiTop = 16,
        ExposureRoiBottom = 17,
        Brightness = 18,
        Contrast = 19,
        Saturation = 20,
        Sharpness = 21,
        AutoWhiteBalanceTemperature = 22,
        BacklightCompensation = 23,
        Hue = 24,
        Gamma = 25,
        ManualWhiteBalance = 26,
        PowerLineFrequency = 27,
        LowLightCompensation = 28,
    }

    public enum Option
    {
        BacklightCompensation = 0,
        Brightness = 1,
        Contrast = 2,
        Exposure = 3,
        Gain = 4,
        Gamma = 5,
        Hue = 6,
        Saturation = 7,
        Sharpness = 8,
        WhiteBalance = 9,
        EnableAutoExposure = 10,
        EnableAutoWhiteBalance = 11,
        VisualPreset = 12,
        LaserPower = 13,
        Accuracy = 14,
        MotionRange = 15,
        FilterOption = 16,
        ConfidenceThreshold = 17,
        EmitterEnabled = 18,
        FramesQueueSize = 19,
        TotalFrameDrops = 20,
        AutoExposureMode = 21,
        PowerLineFrequency = 22,
        AsicTemperature = 23,
        ErrorPollingEnabled = 24,
        ProjectorTemperature = 25,
        OutputTriggerEnabled = 26,
        MotionModuleTemperature = 27,
        DepthUnits = 28,
        EnableMotionCorrection = 29,
        AutoExposurePriority = 30,
        ColorScheme = 31,
        HistogramEqualizationEnabled = 32,
        MinDistance = 33,
        MaxDistance = 34,
        TextureSource = 35,
        FilterMagnitude = 36,
        FilterSmoothAlpha = 37,
        FilterSmoothDelta = 38,
        HolesFill = 39,
        StereoBaseline = 40,
        AutoExposureConvergeStep = 41,
        InterCamSyncMode = 42,
        StreamFilter = 43,
        StreamFormatFilter = 44,
        StreamIndexFilter = 45
    }

    public enum Sr300VisualPreset
    {
        ShortRange = 0,
        LongRange = 1,
        BackgroundSegmentation = 2,
        GestureRecognition = 3,
        ObjectScanning = 4,
        FaceAnalytics = 5,
        FaceLogin = 6,
        GrCursor = 7,
        Default = 8,
        MidRange = 9,
        IrOnly = 10,
    }

    public enum Rs400VisualPreset
    {
        Custom = 0,
        ShortRange = 1,
        Hand = 2,
        HighAccuracy = 3,
        HighDensity = 4,
        MediumDensity = 5,
    }

    public enum PlaybackStatus
    {
        Unknown = 0,
        Playing = 1,
        Paused = 2,
        Stopped = 3,
    }

    public enum RecordingMode
    {
        BlankFrames = 0,
        Compressed = 1,
        BestQuality = 2,
    }


    /// <summary>
    /// Video stream intrinsics
    /// </summary>
    [System.Serializable]
    [StructLayout(LayoutKind.Sequential)]
    public struct Intrinsics
    {
        public int width;       /** Width of the image in pixels */
        public int height;      /** Height of the image in pixels */
        public float ppx;       /** Horizontal coordinate of the principal point of the image, as a pixel offset from the left edge */
        public float ppy;       /** Vertical coordinate of the principal point of the image, as a pixel offset from the top edge */
        public float fx;        /** Focal length of the image plane, as a multiple of pixel width */
        public float fy;        /** Focal length of the image plane, as a multiple of pixel height */
        public Distortion model;     /** Distortion model of the image */

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 5)]
        public float[] coeffs; /** Distortion coefficients */

        public override string ToString()
        {
            return String.Format("(width:{0}, height:{1}, ppx:{2}, ppy:{3}, fx:{4}, fy:{5}, model:{6}, coeffs:[{7}])",
                width,
                height,
                ppx,
                ppy,
                fx,
                fy,
                model,
                String.Join(", ", Array.ConvertAll(coeffs, Convert.ToString))
            );
        }
    }

    /// <summary>
    /// Cross-stream extrinsics: encode the topology describing how the different devices are connected.
    /// </summary>
    [System.Serializable]
    [StructLayout(LayoutKind.Sequential)]
    public struct Extrinsics
    {
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 9)]
        public float[] rotation;    // Column-major 3x3 rotation matrix

        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)]
        public float[] translation; // Three-element translation vector, in meters
    }

    [System.Serializable]
    [StructLayout(LayoutKind.Sequential)]
    public struct VideoStream
    {
        public Stream type;
        public int index;
        public int uid;
        public int width;
        public int height;
        public int fps;
        public int bpp;
        public Format fmt;
        public Intrinsics intrinsics;
    }

    public enum Matchers
    {
        DI,      //compare depth and ir based on frame number
        DI_C,    //compare depth and ir based on frame number,
                 //compare the pair of corresponding depth and ir with color based on closest timestamp,
                 //commonlly used by SR300
        DLR_C,   //compare depth, left and right ir based on frame number,
                 //compare the set of corresponding depth, left and right with color based on closest timestamp,
                 //commonlly used by RS415, RS435
        DLR,     //compare depth, left and right ir based on frame number,
                 //commonlly used by RS400, RS405, RS410, RS420, RS430
        Default, //the default matcher compare all the streams based on closest timestamp
    }


    [System.Serializable]
    [StructLayout(LayoutKind.Sequential)]
    public struct SoftwareVideoStream
    {
        public Stream type;
        public int index;
        public int uid;
        public int width;
        public int height;
        public int fps;
        public int bpp;
        public Format format;
        public Intrinsics intrinsics;
    }


    [UnmanagedFunctionPointer(CallingConvention.Cdecl)]
    public delegate void Deleter();

    [System.Serializable]
    [StructLayout(LayoutKind.Sequential)]
    public class SoftwareVideoFrame
    {
        public IntPtr pixels;
        public Deleter deleter = delegate { };
        public int stride;
        public int bpp;
        public double timestamp;
        public TimestampDomain domain;
        public int frame_number;
        public IntPtr profile;
    }


}
