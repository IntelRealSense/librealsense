using System;
using System.Runtime.InteropServices;

namespace RealSense
{
    #region Public classes modeled on the opaque types specified in rs.h
    public class Context : IDisposable
    {
        public Device[] Devices
        {
            get
            {
                Device[] devices;
                using (var e = new AutoError()) { devices = new Device[rs_get_device_count(handle, ref e.Handle)]; }
                for (int i = 0; i < devices.Length; ++i) 
                {
                    using (var e = new AutoError()) { devices[i] = new Device(rs_get_device(handle, i, ref e.Handle)); }
                }
                return devices;
            }
        }

        public Context() { using (var e = new AutoError()) { handle = rs_create_context(ApiVersion, ref e.Handle); } }
        public void Dispose() { rs_delete_context(handle, IntPtr.Zero); }

        #region P/Invoke declarations for backing C API
        private IntPtr handle;
        private const int ApiVersion = 5;
        [DllImport("realsense")] private static extern IntPtr rs_create_context(int api_version, ref IntPtr error);
        [DllImport("realsense")] private static extern void rs_delete_context(IntPtr context, IntPtr error); // Note: NOT ref IntPtr, since we want to pass 0
        [DllImport("realsense")] private static extern int rs_get_device_count(IntPtr context, ref IntPtr error);
        [DllImport("realsense")] private static extern IntPtr rs_get_device(IntPtr context, int index, ref IntPtr error);
        #endregion
    }

    public class Device
    {
        public Device(IntPtr handle) { this.handle = handle; }

        public string Name
        {
            get { using (var e = new AutoError()) { return Marshal.PtrToStringAnsi(rs_get_device_name(handle, ref e.Handle)); } }
        }

        public Extrinsics GetExtrinsics(Stream from, Stream to)
        {
            using (var e = new AutoError())
            {
                var extrin = new Extrinsics();
                rs_get_device_extrinsics(handle, from, to, out extrin, ref e.Handle);
                return extrin;
            }
        }

        public bool SupportsOption(Option option)
        {
            using (var e = new AutoError()) { return rs_device_supports_option(handle, option, ref e.Handle) != 0; }
        }

        public int GetStreamModeCount(Stream stream)
        {
            using (var e = new AutoError()) { return rs_get_stream_mode_count(handle, stream, ref e.Handle); }
        }

        public void GetStreamMode(Stream stream, int index, out int width, out int height, out Format format, out int framerate)
        {
            using (var e = new AutoError()) { rs_get_stream_mode(handle, stream, index, out width, out height, out format, out framerate, ref e.Handle); }
        }   

        public void EnableStream(Stream stream, int width, int height, Format format, int fps)
        {
            using (var e = new AutoError()) { rs_enable_stream(handle, stream, width, height, format, fps, ref e.Handle); }
        }

        public void EnableStreamPreset(Stream stream, Preset preset)
        {
            using (var e = new AutoError()) { rs_enable_stream_preset(handle, stream, preset, ref e.Handle); }
        }

        public bool StreamIsEnabled(Stream stream)
        {
            using (var e = new AutoError()) { return rs_stream_is_enabled(handle, stream, ref e.Handle) != 0; }
        }

        public Intrinsics GetStreamIntrinsics(Stream stream)
        {
            using (var e = new AutoError())
            {
                var intrin = new Intrinsics();
                rs_get_stream_intrinsics(handle, stream, out intrin, ref e.Handle);
                return intrin;
            }
        }

        public Format GetStreamFormat(Stream stream)
        {
            using (var e = new AutoError()) { return rs_get_stream_format(handle, stream, ref e.Handle); }
        }

        public int GetStreamFramerate(Stream stream)
        {
            using (var e = new AutoError()) { return rs_get_stream_framerate(handle, stream, ref e.Handle); }
        }

        public void Start()
        {
            using (var e = new AutoError()) { rs_start_device(handle, ref e.Handle); }
        }

        public void Stop()
        {
            using (var e = new AutoError()) { rs_stop_device(handle, ref e.Handle); }
        }

        public bool IsStreaming
        {
            get { using (var e = new AutoError()) { return rs_device_is_streaming(handle, ref e.Handle) != 0; } }
        }

        public void SetOption(Option option, int value)
        {
            using (var e = new AutoError()) { rs_set_device_option(handle, option, value, ref e.Handle); }
        }

        public int GetOption(Option option)
        {
            using (var e = new AutoError()) { return rs_get_device_option(handle, option, ref e.Handle); }
        }

        public float DepthScale
        {
            get { using (var e = new AutoError()) { return rs_get_device_depth_scale(handle, ref e.Handle); } }
        }
        
        public void WaitForFrames()
        {
            using (var e = new AutoError()) { rs_wait_for_frames(handle, ref e.Handle); }
        }

        public int GetFrameTimestamp(Stream stream)
        {
            using (var e = new AutoError()) { return rs_get_frame_timestamp(handle, stream, ref e.Handle); }
        }

        public IntPtr GetFrameData(Stream stream)
        {
            using (var e = new AutoError()) { return rs_get_frame_data(handle, stream, ref e.Handle); }
        }

        #region P/Invoke declarations for backing C API
        private IntPtr handle;
        [DllImport("realsense")] private static extern IntPtr rs_get_device_name(IntPtr device, ref IntPtr error);
        [DllImport("realsense")] private static extern void rs_get_device_extrinsics(IntPtr device, Stream from, Stream to, out Extrinsics extrin, ref IntPtr error);
        [DllImport("realsense")] private static extern float rs_get_device_depth_scale(IntPtr device, ref IntPtr error);
        [DllImport("realsense")] private static extern int rs_device_supports_option(IntPtr device, Option option, ref IntPtr error);
        [DllImport("realsense")] private static extern int rs_get_stream_mode_count(IntPtr device, Stream stream, ref IntPtr error);
        [DllImport("realsense")] private static extern void rs_get_stream_mode(IntPtr device, Stream stream, int index, out int width, out int height, out Format format, out int framerate, ref IntPtr error);

        [DllImport("realsense")] private static extern void rs_enable_stream(IntPtr device, Stream stream, int width, int height, Format format, int fps, ref IntPtr error);
        [DllImport("realsense")] private static extern void rs_enable_stream_preset(IntPtr device, Stream stream, Preset preset, ref IntPtr error);
        [DllImport("realsense")] private static extern int rs_stream_is_enabled(IntPtr device, Stream stream, ref IntPtr error);
        [DllImport("realsense")] private static extern void rs_get_stream_intrinsics(IntPtr device, Stream stream, out Intrinsics intrin, ref IntPtr error);
        [DllImport("realsense")] private static extern Format rs_get_stream_format(IntPtr device, Stream stream, ref IntPtr error);
        [DllImport("realsense")] private static extern int rs_get_stream_framerate(IntPtr device, Stream stream, ref IntPtr error);

        [DllImport("realsense")] private static extern void rs_start_device(IntPtr device, ref IntPtr error);
        [DllImport("realsense")] private static extern void rs_stop_device(IntPtr device, ref IntPtr error);        
        [DllImport("realsense")] private static extern int rs_device_is_streaming(IntPtr device, ref IntPtr error);        

        [DllImport("realsense")] private static extern void rs_set_device_option(IntPtr device, Option option, int value, ref IntPtr error);
        [DllImport("realsense")] private static extern int rs_get_device_option(IntPtr device, Option option, ref IntPtr error);

        [DllImport("realsense")] private static extern void rs_wait_for_frames(IntPtr device, ref IntPtr error);
        [DllImport("realsense")] private static extern int rs_get_frame_timestamp(IntPtr device, Stream stream, ref IntPtr error);
        [DllImport("realsense")] private static extern IntPtr rs_get_frame_data(IntPtr device, Stream stream, ref IntPtr error);        
        #endregion
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
        
        #region P/Invoke declarations for backing C API
        [DllImport("realsense")] private static extern void rs_free_error(IntPtr error);
        [DllImport("realsense")] private static extern IntPtr rs_get_failed_function(IntPtr error);
        [DllImport("realsense")] private static extern IntPtr rs_get_failed_args(IntPtr error);
        [DllImport("realsense")] private static extern IntPtr rs_get_error_message(IntPtr error);
        #endregion
    }
    #endregion

    #region Public struct definitions, must exactly match the layouts specified in rs.h
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct Intrinsics
    {
        public int Width, Height;
        public float PrincipalPointX, PrincipalPointY;
        public float FocalLengthX, FocalLengthY;
        public Distortion DistortionModel;    
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 5)] public float[] DistortionCoeffs;
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct Extrinsics
    {
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 9)] public float[] Rotation;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)] public float[] Translation;
    };
    #endregion

    #region Public enum definitions, must exactly match the values specified in rs.h
    public enum Stream : int
    {
        Depth = 0,
        Color = 1,
        Infrared = 2,
        Infrared2 = 3,
    };

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
    };

    public enum Preset : int
    {
        BestQuality = 0,
        LargestImage = 1,
        HighestFramerate = 2,
    };

    public enum Distortion : int
    {
        None = 0,
        ModifiedBrownConrady = 1,
        InverseBrownConrady = 2,
    };

    public enum Option : int
    {
        F200LaserPower,
        F200Accuracy,
        F200MotionRange,
        F200FilterOption,
        F200ConfidenceThreshold,
        F200DynamicFPS,
        R200LRAutoExposureEnabled,
        R200LRGain,
        R200LRExposure,
        R200EmitterEnabled,
        R200DepthControlPreset,
        R200DepthUnits,
        R200DepthClampMin,
        R200DepthClampMax,
        R200DisparityModeEnabled,
        R200DisparityMultiplier,
        R200DisparityShift,
    };
    #endregion

    #region Internal helper functionality
    internal class AutoError : IDisposable
    {
        public IntPtr Handle;
        public void Dispose() { Error.Handle(Handle); }
    }
    #endregion
}
