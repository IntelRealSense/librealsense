using System;
using System.Runtime.InteropServices;

namespace RealSense
{

    public enum Stream : int
    {
        Depth = 0, // Native stream of depth data produced by RealSense device
        Color = 1, // Native stream of color data captured by RealSense device
        Infrared = 2, // Native stream of infrared data captured by RealSense device
        Infrared2 = 3, // Native stream of infrared data captured from a second viewpoint by RealSense device
        RectifiedColor = 4, // Synthetic stream containing undistorted color data with no extrinsic rotation from the depth stream
        ColorAlignedToDepth = 5, // Synthetic stream containing color data but sharing intrinsics of depth stream
        DepthAlignedToColor = 6, // Synthetic stream containing depth data but sharing intrinsics of color stream
        DepthAlignedToRectifiedColor = 7 // Synthetic stream containing depth data but sharing intrinsics of rectified color stream
    }

    public enum Format : int
    {
        Any = 0,
        Z16 = 1,
        YUYV = 2,
        RGB8 = 3,
        BGR8 = 4,
        RGBA8 = 5,
        BGRA8 = 6,
        Y8 = 7,
        Y16 = 8,
        Raw10 = 9 // Four 10-bit luminance values encoded into a 5-byte macropixel
    }

    public enum Preset : int
    {
        BestQuality = 0,
        LargestImage = 1,
        HighestFramerate = 2
    }

    public enum Distortion : int
    {
        None = 0, // Rectilinear images, no distortion compensation required
        ModifiedBrownConrady = 1, // Equivalent to Brown-Conrady distortion, except that tangential distortion is applied to radially distorted points
        InverseBrownConrady = 2 // Equivalent to Brown-Conrady distortion, except undistorts image instead of distorting it
    }

    public enum Option : int
    {
        ColorBacklightCompensation = 0,
        ColorBrightness = 1,
        ColorContrast = 2,
        ColorExposure = 3,
        ColorGain = 4,
        ColorGamma = 5,
        ColorHue = 6,
        ColorSaturation = 7,
        ColorSharpness = 8,
        ColorWhiteBalance = 9,
        F200LaserPower = 10, // 0 - 15
        F200Accuracy = 11, // 0 - 3
        F200MotionRange = 12, // 0 - 100
        F200FilterOption = 13, // 0 - 7
        F200ConfidenceThreshold = 14, // 0 - 15
        F200DynamicFPS = 15, // {2, 5, 15, 30, 60}
        R200LRAutoExposureEnabled = 16, // {0, 1}
        R200LRGain = 17, // 100 - 1600 (Units of 0.01)
        R200LRExposure = 18, // > 0 (Units of 0.1 ms)
        R200EmitterEnabled = 19, // {0, 1}
        R200DepthControlPreset = 20, // {0, 5}, 0 is default, 1-5 is low to high outlier rejection
        R200DepthUnits = 21, // micrometers per increment in integer depth values, 1000 is default (mm scale)
        R200DepthClampMin = 22, // 0 - USHORT_MAX
        R200DepthClampMax = 23, // 0 - USHORT_MAX
        R200DisparityModeEnabled = 24, // {0, 1}
        R200DisparityMultiplier = 25,
        R200DisparityShift = 26
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct Intrinsics
    {
        public int Width; // width of the image in pixels
        public int Height; // height of the image in pixels
        public float PPX; // horizontal coordinate of the principal point of the image, as a pixel offset from the left edge
        public float PPY; // vertical coordinate of the principal point of the image, as a pixel offset from the top edge
        public float FX; // focal length of the image plane, as a multiple of pixel width
        public float FY; // focal length of the image plane, as a multiple of pixel height
        public Distortion Model; // distortion model of the image
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 5)] public float[] Coeffs; // distortion coefficients
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct Extrinsics
    {
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 9)] public float[] Rotation; // column-major 3x3 rotation matrix
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)] public float[] Translation; // 3 element translation vector, in meters
    }


    public class Error : Exception
    {
        public string FailedFunction;
        public string FailedArgs;

        public static void Handle(IntPtr error)
        {
            if (error != IntPtr.Zero)
            {
                var errorMessage = Marshal.PtrToStringAnsi(rs_get_error_message(error));
                var failedFunction = Marshal.PtrToStringAnsi(rs_get_failed_function(error));
                var failedArgs = Marshal.PtrToStringAnsi(rs_get_failed_args(error));                
                rs_free_error(error);
                throw new Error(errorMessage) { FailedFunction = failedFunction, FailedArgs = failedArgs };
            }
        }

        private Error(string message) : base(message) {}
        
        [DllImport("realsense")] private static extern void rs_free_error(IntPtr error);
        [DllImport("realsense")] private static extern IntPtr rs_get_failed_function(IntPtr error);
        [DllImport("realsense")] private static extern IntPtr rs_get_failed_args(IntPtr error);
        [DllImport("realsense")] private static extern IntPtr rs_get_error_message(IntPtr error);
    }

    public class Context : IDisposable
    {
        private IntPtr handle;

        public Context()
        {
            IntPtr e = IntPtr.Zero;
            handle = rs_create_context(3, ref e);
            Error.Handle(e);
        }

        public void Dispose()
        {
            rs_delete_context(handle, IntPtr.Zero);
        }

        public int GetDeviceCount()
        {
            IntPtr e = IntPtr.Zero;
            var r = rs_get_device_count(handle, ref e);
            Error.Handle(e);
            return r;
        }

        public Device GetDevice(int index)
        {
            IntPtr e = IntPtr.Zero;
            var r = rs_get_device(handle, index, ref e);
            Error.Handle(e);
            return new Device(r);
        }

        [DllImport("realsense")] private static extern IntPtr rs_create_context(int apiVersion, ref IntPtr error);
        [DllImport("realsense")] private static extern void rs_delete_context(IntPtr context, IntPtr error);
        [DllImport("realsense")] private static extern int rs_get_device_count(IntPtr context, ref IntPtr error);
        [DllImport("realsense")] private static extern IntPtr rs_get_device(IntPtr context, int index, ref IntPtr error);
    }

    public class Device
    {
        private IntPtr handle;
        public Device(IntPtr handle) { this.handle = handle; }

        public string GetName()
        {
            IntPtr e = IntPtr.Zero;
            var r = rs_get_device_name(handle, ref e);
            Error.Handle(e);
            return Marshal.PtrToStringAnsi(r);
        }

        public string GetSerial()
        {
            IntPtr e = IntPtr.Zero;
            var r = rs_get_device_serial(handle, ref e);
            Error.Handle(e);
            return Marshal.PtrToStringAnsi(r);
        }

        public string GetFirmwareVersion()
        {
            IntPtr e = IntPtr.Zero;
            var r = rs_get_device_firmware_version(handle, ref e);
            Error.Handle(e);
            return Marshal.PtrToStringAnsi(r);
        }

        public Extrinsics GetExtrinsics(Stream fromStream, Stream toStream)
        {
            IntPtr e = IntPtr.Zero;
            Extrinsics extrin;
            rs_get_device_extrinsics(handle, fromStream, toStream, out extrin, ref e);
            Error.Handle(e);
            return extrin;
        }

        public float GetDepthScale()
        {
            IntPtr e = IntPtr.Zero;
            var r = rs_get_device_depth_scale(handle, ref e);
            Error.Handle(e);
            return r;
        }

        public bool SupportsOption(Option option)
        {
            IntPtr e = IntPtr.Zero;
            var r = rs_device_supports_option(handle, option, ref e);
            Error.Handle(e);
            return r != 0;
        }

        public int GetStreamModeCount(Stream stream)
        {
            IntPtr e = IntPtr.Zero;
            var r = rs_get_stream_mode_count(handle, stream, ref e);
            Error.Handle(e);
            return r;
        }

        public void GetStreamMode(Stream stream, int index, out int width, out int height, out Format format, out int framerate)
        {
            IntPtr e = IntPtr.Zero;
            rs_get_stream_mode(handle, stream, index, out width, out height, out format, out framerate, ref e);
            Error.Handle(e);
        }

        public void EnableStream(Stream stream, int width, int height, Format format, int framerate)
        {
            IntPtr e = IntPtr.Zero;
            rs_enable_stream(handle, stream, width, height, format, framerate, ref e);
            Error.Handle(e);
        }

        public void EnableStream(Stream stream, Preset preset)
        {
            IntPtr e = IntPtr.Zero;
            rs_enable_stream_preset(handle, stream, preset, ref e);
            Error.Handle(e);
        }

        public void DisableStream(Stream stream)
        {
            IntPtr e = IntPtr.Zero;
            rs_disable_stream(handle, stream, ref e);
            Error.Handle(e);
        }

        public bool IsStreamEnabled(Stream stream)
        {
            IntPtr e = IntPtr.Zero;
            var r = rs_is_stream_enabled(handle, stream, ref e);
            Error.Handle(e);
            return r != 0;
        }

        public Intrinsics GetStreamIntrinsics(Stream stream)
        {
            IntPtr e = IntPtr.Zero;
            Intrinsics intrin;
            rs_get_stream_intrinsics(handle, stream, out intrin, ref e);
            Error.Handle(e);
            return intrin;
        }

        public Format GetStreamFormat(Stream stream)
        {
            IntPtr e = IntPtr.Zero;
            var r = rs_get_stream_format(handle, stream, ref e);
            Error.Handle(e);
            return r;
        }

        public int GetStreamFramerate(Stream stream)
        {
            IntPtr e = IntPtr.Zero;
            var r = rs_get_stream_framerate(handle, stream, ref e);
            Error.Handle(e);
            return r;
        }

        public void Start()
        {
            IntPtr e = IntPtr.Zero;
            rs_start_device(handle, ref e);
            Error.Handle(e);
        }

        public void Stop()
        {
            IntPtr e = IntPtr.Zero;
            rs_stop_device(handle, ref e);
            Error.Handle(e);
        }

        public bool IsStreaming()
        {
            IntPtr e = IntPtr.Zero;
            var r = rs_is_device_streaming(handle, ref e);
            Error.Handle(e);
            return r != 0;
        }

        public void SetOption(Option option, int value)
        {
            IntPtr e = IntPtr.Zero;
            rs_set_device_option(handle, option, value, ref e);
            Error.Handle(e);
        }

        public int GetOption(Option option)
        {
            IntPtr e = IntPtr.Zero;
            var r = rs_get_device_option(handle, option, ref e);
            Error.Handle(e);
            return r;
        }

        public void WaitForFrames()
        {
            IntPtr e = IntPtr.Zero;
            rs_wait_for_frames(handle, ref e);
            Error.Handle(e);
        }

        public int GetFrameTimestamp(Stream stream)
        {
            IntPtr e = IntPtr.Zero;
            var r = rs_get_frame_timestamp(handle, stream, ref e);
            Error.Handle(e);
            return r;
        }

        public IntPtr GetFrameData(Stream stream)
        {
            IntPtr e = IntPtr.Zero;
            var r = rs_get_frame_data(handle, stream, ref e);
            Error.Handle(e);
            return r;
        }

        [DllImport("realsense")] private static extern IntPtr rs_get_device_name(IntPtr device, ref IntPtr error);
        [DllImport("realsense")] private static extern IntPtr rs_get_device_serial(IntPtr device, ref IntPtr error);
        [DllImport("realsense")] private static extern IntPtr rs_get_device_firmware_version(IntPtr device, ref IntPtr error);
        [DllImport("realsense")] private static extern void rs_get_device_extrinsics(IntPtr device, Stream from_stream, Stream to_stream, out Extrinsics extrin, ref IntPtr error);
        [DllImport("realsense")] private static extern float rs_get_device_depth_scale(IntPtr device, ref IntPtr error);
        [DllImport("realsense")] private static extern int rs_device_supports_option(IntPtr device, Option option, ref IntPtr error);
        [DllImport("realsense")] private static extern int rs_get_stream_mode_count(IntPtr device, Stream stream, ref IntPtr error);
        [DllImport("realsense")] private static extern void rs_get_stream_mode(IntPtr device, Stream stream, int index, out int width, out int height, out Format format, out int framerate, ref IntPtr error);
        [DllImport("realsense")] private static extern void rs_enable_stream(IntPtr device, Stream stream, int width, int height, Format format, int framerate, ref IntPtr error);
        [DllImport("realsense")] private static extern void rs_enable_stream_preset(IntPtr device, Stream stream, Preset preset, ref IntPtr error);
        [DllImport("realsense")] private static extern void rs_disable_stream(IntPtr device, Stream stream, ref IntPtr error);
        [DllImport("realsense")] private static extern int rs_is_stream_enabled(IntPtr device, Stream stream, ref IntPtr error);
        [DllImport("realsense")] private static extern void rs_get_stream_intrinsics(IntPtr device, Stream stream, out Intrinsics intrin, ref IntPtr error);
        [DllImport("realsense")] private static extern Format rs_get_stream_format(IntPtr device, Stream stream, ref IntPtr error);
        [DllImport("realsense")] private static extern int rs_get_stream_framerate(IntPtr device, Stream stream, ref IntPtr error);
        [DllImport("realsense")] private static extern void rs_start_device(IntPtr device, ref IntPtr error);
        [DllImport("realsense")] private static extern void rs_stop_device(IntPtr device, ref IntPtr error);
        [DllImport("realsense")] private static extern int rs_is_device_streaming(IntPtr device, ref IntPtr error);
        [DllImport("realsense")] private static extern void rs_set_device_option(IntPtr device, Option option, int value, ref IntPtr error);
        [DllImport("realsense")] private static extern int rs_get_device_option(IntPtr device, Option option, ref IntPtr error);
        [DllImport("realsense")] private static extern void rs_wait_for_frames(IntPtr device, ref IntPtr error);
        [DllImport("realsense")] private static extern int rs_get_frame_timestamp(IntPtr device, Stream stream, ref IntPtr error);
        [DllImport("realsense")] private static extern IntPtr rs_get_frame_data(IntPtr device, Stream stream, ref IntPtr error);
    }
}
