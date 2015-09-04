using System;
using System.Runtime.InteropServices;

namespace RealSense
{
    public enum Stream : int
    {
        Depth                         = 0,
        Color                         = 1,
        Infrared                      = 2,
        Infrared2                     = 3,
    };

    public enum Format : int
    {
        Any                           = 0,
        Z16                           = 1,
        YUYV                          = 2,
        RGB8                          = 3,
        BGR8                          = 4,
        RGBA8                         = 5,
        BGRA8                         = 6,
        Y8                            = 7,
        Y16                           = 8,
    };

    public enum Preset : int
    {
        BestQuality                   = 0,
        LargestImage                  = 1,
        HighestFramerate              = 2,
    };

    public enum Distortion : int
    {
        None                          = 0,
        ModifiedBrownConrady          = 1,
        InverseBrownConrady           = 2,
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

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct Intrinsics
    {
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 2)] public int[] ImageSize;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 2)] public float[] FocalLength;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 2)] public float[] PrincipalPoint;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 5)] public float[] DistortionCoeffs;
        public Distortion DistortionModel;    
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct Extrinsics
    {
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 9)] public float[] Rotation;
        [MarshalAs(UnmanagedType.ByValArray, SizeConst = 3)] public float[] Translation;
    };

    public class Context : IDisposable
    {
        public Context() { using (var e = new AutoError()) { handle = rs_create_context(ApiVersion, ref e.Handle); } }
        public int CameraCount { get { using (var e = new AutoError()) { return rs_get_camera_count(handle, ref e.Handle); } } }
        public Camera GetCamera(int index) { using (var e = new AutoError()) { return new Camera(rs_get_camera(handle, index, ref e.Handle)); } }
        public void Dispose() { rs_delete_context(handle, IntPtr.Zero); }

        #region P/Invoke declarations for backing C API
        private IntPtr handle;
        private const int ApiVersion = 2;
        [DllImport("realsense.dll")] private static extern IntPtr rs_create_context(int api_version, ref IntPtr error);
        [DllImport("realsense.dll")] private static extern int rs_get_camera_count(IntPtr context, ref IntPtr error);
        [DllImport("realsense.dll")] private static extern IntPtr rs_get_camera(IntPtr context, int index, ref IntPtr error);
        [DllImport("realsense.dll")] private static extern void rs_delete_context(IntPtr context, IntPtr error); // Note: NOT ref IntPtr, since we want to pass 0
        #endregion
    }

    public class Camera
    {
        public string Name
        {
            get { using (var e = new AutoError()) { return Marshal.PtrToStringAnsi(rs_get_camera_name(handle, ref e.Handle)); } }
        }

        public bool IsCapturing
        {
            get { using (var e = new AutoError()) { return rs_is_capturing(handle, ref e.Handle) != 0; } }
        }

        public float DepthScale
        {
            get { using (var e = new AutoError()) { return rs_get_depth_scale(handle, ref e.Handle); } }
        }

        public Camera(IntPtr handle) { this.handle = handle; }

        public void EnableStream(Stream stream, int width, int height, Format format, int fps)
        {
            using (var e = new AutoError()) { rs_enable_stream(handle, stream, width, height, format, fps, ref e.Handle); }
        }

        public void EnableStreamPreset(Stream stream, Preset preset)
        {
            using (var e = new AutoError()) { rs_enable_stream_preset(handle, stream, preset, ref e.Handle); }
        }

        public bool IsStreamEnabled(Stream stream)
        {
            using (var e = new AutoError()) { return rs_is_stream_enabled(handle, stream, ref e.Handle) != 0; }
        }

        public void StartCapture()
        {
            using (var e = new AutoError()) { rs_start_capture(handle, ref e.Handle); }
        }

        public void StopCapture()
        {
            using (var e = new AutoError()) { rs_stop_capture(handle, ref e.Handle); }
        }

        public bool SupportsOption(Option option)
        {
            using (var e = new AutoError()) { return rs_camera_supports_option(handle, option, ref e.Handle) != 0; }
        }

        public int GetOption(Option option)
        {
            using (var e = new AutoError()) { return rs_get_camera_option(handle, option, ref e.Handle); }
        }

        public void SetOption(Option option, int value)
        {
            using (var e = new AutoError()) { rs_set_camera_option(handle, option, value, ref e.Handle); }
        }

        public Format GetStreamFormat(Stream stream)
        {
            using (var e = new AutoError()) { return rs_get_stream_format(handle, stream, ref e.Handle); }
        }

        public Intrinsics GetStreamIntrinsics(Stream stream)
        {
            using (var e = new AutoError())
            {
                var intrin = new Intrinsics();
                rs_get_stream_intrinsics(handle, stream, ref intrin, ref e.Handle);
                return intrin;
            }
        }

        public Extrinsics GetStreamExtrinsics(Stream from, Stream to)
        {
            using (var e = new AutoError())
            {
                var extrin = new Extrinsics();
                rs_get_stream_extrinsics(handle, from, to, ref extrin, ref e.Handle);
                return extrin;
            }
        }

        public void WaitAllStreams()
        {
            using (var e = new AutoError()) { rs_wait_all_streams(handle, ref e.Handle); }
        }

        public IntPtr GetImagePixels(Stream stream)
        {
            using (var e = new AutoError()) { return rs_get_image_pixels(handle, stream, ref e.Handle); }
        }

        public int GetImageFrameNumber(Stream stream)
        {
            using (var e = new AutoError()) { return rs_get_image_frame_number(handle, stream, ref e.Handle); }
        }

        #region P/Invoke declarations for backing C API
        private IntPtr handle;
        private unsafe struct Intrin
        {
            public fixed int sz[2];
            public fixed float fl[2];
            public fixed float pt[2];
            public fixed float k[5];
            public Distortion m;
        };
        private unsafe struct Extrin
        {
            public fixed float r[9];
            public fixed float t[3];
        };
        [DllImport("realsense.dll")] private static extern IntPtr rs_get_camera_name(IntPtr camera, ref IntPtr error);
        [DllImport("realsense.dll")] private static extern void rs_enable_stream(IntPtr camera, Stream stream, int width, int height, Format format, int fps, ref IntPtr error);
        [DllImport("realsense.dll")] private static extern void rs_enable_stream_preset(IntPtr camera, Stream stream, Preset preset, ref IntPtr error);
        [DllImport("realsense.dll")] private static extern int rs_is_stream_enabled(IntPtr camera, Stream stream, ref IntPtr error);
        [DllImport("realsense.dll")] private static extern void rs_start_capture(IntPtr camera, ref IntPtr error);
        [DllImport("realsense.dll")] private static extern void rs_stop_capture(IntPtr camera, ref IntPtr error);
        [DllImport("realsense.dll")] private static extern int rs_is_capturing(IntPtr camera, ref IntPtr error);
        [DllImport("realsense.dll")] private static extern int rs_camera_supports_option(IntPtr camera, Option option, ref IntPtr error);
        [DllImport("realsense.dll")] private static extern int rs_get_camera_option(IntPtr camera, Option option, ref IntPtr error);
        [DllImport("realsense.dll")] private static extern void rs_set_camera_option(IntPtr camera, Option option, int value, ref IntPtr error);
        [DllImport("realsense.dll")] private static extern float rs_get_depth_scale(IntPtr camera, ref IntPtr error);
        [DllImport("realsense.dll")] private static extern Format rs_get_stream_format(IntPtr camera, Stream stream, ref IntPtr error);
        [DllImport("realsense.dll")] private static extern void rs_get_stream_intrinsics(IntPtr camera, Stream stream, ref Intrinsics intrin, ref IntPtr error);
        [DllImport("realsense.dll")] private static extern void rs_get_stream_extrinsics(IntPtr camera, Stream from, Stream to, ref Extrinsics extrin, ref IntPtr error);
        [DllImport("realsense.dll")] private static extern void rs_wait_all_streams(IntPtr camera, ref IntPtr error);
        [DllImport("realsense.dll")] private static extern IntPtr rs_get_image_pixels(IntPtr camera, Stream stream, ref IntPtr error);
        [DllImport("realsense.dll")] private static extern int rs_get_image_frame_number(IntPtr camera, Stream stream, ref IntPtr error);
        #endregion
    }

    public class Error : Exception
    {
        public readonly string ErrorMessage;
        public readonly string FailedFunction;
        public readonly string FailedArgs;

        public static void Handle(IntPtr error)
        {
            if (error != IntPtr.Zero)
            {
                var errorMessage = Marshal.PtrToStringAnsi(rs_get_error_message(error));
                var failedFunction = Marshal.PtrToStringAnsi(rs_get_failed_function(error));
                var failedArgs = Marshal.PtrToStringAnsi(rs_get_failed_args(error));                
                rs_free_error(error);
                throw new Error(errorMessage, failedFunction, failedArgs);
            }
        }

        private Error(string errorMessage, string failedFunction, string failedArgs) : base(string.Format("{0} ({1}({2}))", errorMessage, failedFunction, failedArgs))
        {
            this.ErrorMessage = errorMessage;
            this.FailedFunction = failedFunction;
            this.FailedArgs = failedArgs;
        }
        
        #region P/Invoke declarations for backing C API
        [DllImport("realsense.dll")] private static extern IntPtr rs_get_failed_function(IntPtr error);
        [DllImport("realsense.dll")] private static extern IntPtr rs_get_failed_args(IntPtr error);
        [DllImport("realsense.dll")] private static extern IntPtr rs_get_error_message(IntPtr error);
        [DllImport("realsense.dll")] private static extern void rs_free_error(IntPtr error);
        #endregion
    }

    internal class AutoError : IDisposable
    {
        public IntPtr Handle;
        public void Dispose() { Error.Handle(Handle); }
    }
}
