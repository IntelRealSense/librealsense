using System;
using System.Runtime.InteropServices;

namespace Intel.RealSense
{
    public delegate void frame_callback(IntPtr frame, IntPtr user_data);
    //public delegate void frame_callback([Out, MarshalAs(UnmanagedType.CustomMarshaler, MarshalTypeRef = typeof(FrameMarshaler))] out Frame frame, IntPtr user_data);

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
        FirmwareVersion = 2,
        PhysicalPort = 3,
        DebugOpCode = 4,
        AdvancedMode = 5,
        ProductId = 6,
        CameraLocked = 7,
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
        Rs2FrameMetadataFrameCounter = 0,
        Rs2FrameMetadataFrameTimestamp = 1,
        Rs2FrameMetadataSensorTimestamp = 2,
        Rs2FrameMetadataActualExposure = 3,
        Rs2FrameMetadataGainLevel = 4,
        Rs2FrameMetadataAutoExposure = 5,
        Rs2FrameMetadataWhiteBalance = 6,
        Rs2FrameMetadataTimeOfArrival = 7,
        Rs2FrameMetadataBackendTimestamp= 8,
        Rs2FrameMetadataActualFps = 9,
        Rs2FrameMetadataCount = 10,
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
}
