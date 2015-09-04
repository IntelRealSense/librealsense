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

    public class Intrinsics
    {
        public int[] ImageSize;
        public float[] FocalLength;
        public float[] PrincipalPoint;
        public float[] DistortionCoeffs;
        public Distortion DistortionModel;    
    }

    public unsafe struct Extrinsics
    {
        public fixed float rotation[9];
        public fixed float translation[3];
    };

    public class Context : IDisposable
    {
        public int CameraCount
        {
            get
            {
                IntPtr error = IntPtr.Zero;
                var result = rs_get_camera_count(handle, ref error);
                Error.Handle(error);
                return result;
            }
        }

        public Context()
        {
            IntPtr error = IntPtr.Zero;
            handle = rs_create_context(ApiVersion, ref error);
            Error.Handle(error);
        }

        public void Dispose()
        {
            IntPtr error = IntPtr.Zero;
            rs_delete_context(handle, ref error);
            Error.Ignore(error);
        }

        public Camera GetCamera(int index)
        {
            IntPtr error = IntPtr.Zero;
            var result = rs_get_camera(handle, index, ref error);
            Error.Handle(error);
            return new Camera(result);
        }
        
        private const int ApiVersion = 2;

        #region P/Invoke declarations for backing C API
        private IntPtr handle;
        [DllImport("realsense.dll")] private static extern IntPtr rs_create_context(int api_version, ref IntPtr error);
        [DllImport("realsense.dll")] private static extern int rs_get_camera_count(IntPtr context, ref IntPtr error);
        [DllImport("realsense.dll")] private static extern IntPtr rs_get_camera(IntPtr context, int index, ref IntPtr error);
        [DllImport("realsense.dll")] private static extern void rs_delete_context(IntPtr context, ref IntPtr error);
        #endregion
    }

    public class Camera
    {
        public string Name
        {
            get
            {
                IntPtr error = IntPtr.Zero;
                var result = Marshal.PtrToStringAnsi(rs_get_camera_name(handle, ref error));
                Error.Handle(error);
                return result;
            }
        }

        public bool IsCapturing
        {
            get
            {
                IntPtr error = IntPtr.Zero;
                int result = rs_is_capturing(handle, ref error);
                Error.Handle(error);
                return result != 0;
            }
        }

        public float DepthScale
        {
            get
            {
                IntPtr error = IntPtr.Zero;
                var result = rs_get_depth_scale(handle, ref error);
                Error.Handle(error);
                return result;
            }
        }

        public Camera(IntPtr handle) { this.handle = handle; }

        public void EnableStream(Stream stream, int width, int height, Format format, int fps)
        {
            IntPtr error = IntPtr.Zero;
            rs_enable_stream(handle, stream, width, height, format, fps, ref error);
            Error.Handle(error);
        }

        public void EnableStreamPreset(Stream stream, Preset preset)
        {
            IntPtr error = IntPtr.Zero;
            rs_enable_stream_preset(handle, stream, preset, ref error);
            Error.Handle(error);
        }

        public bool IsStreamEnabled(Stream stream)
        {
            IntPtr error = IntPtr.Zero;
            var result = rs_is_stream_enabled(handle, stream, ref error);
            Error.Handle(error);
            return result != 0;
        }

        public void StartCapture()
        {
            IntPtr error = IntPtr.Zero;
            rs_start_capture(handle, ref error);
            Error.Handle(error);
        }

        public void StopCapture()
        {
            IntPtr error = IntPtr.Zero;
            rs_stop_capture(handle, ref error);
            Error.Handle(error);
        }

        public bool SupportsOption(Option option)
        {
            IntPtr error = IntPtr.Zero;
            var result = rs_camera_supports_option(handle, option, ref error);
            Error.Handle(error);
            return result != 0;
        }

        public int GetOption(Option option)
        {
            IntPtr error = IntPtr.Zero;
            var result = rs_get_camera_option(handle, option, ref error);
            Error.Handle(error);
            return result;
        }

        public void SetOption(Option option, int value)
        {
            IntPtr error = IntPtr.Zero;
            rs_set_camera_option(handle, option, value, ref error);
            Error.Handle(error);
        }

        public Format GetStreamFormat(Stream stream)
        {
            IntPtr error = IntPtr.Zero;
            var result = rs_get_stream_format(handle, stream, ref error);
            Error.Handle(error);
            return result;
        }

        public Intrinsics GetStreamIntrinsics(Stream stream)
        {
            rs_intrinsics i = new rs_intrinsics();
            IntPtr error = IntPtr.Zero;
            rs_get_stream_intrinsics(handle, stream, ref i, ref error);
            Error.Handle(error);

            unsafe
            {
                return new Intrinsics
                {
                    ImageSize = new int[] { i.image_size[0], i.image_size[1] },
                    FocalLength = new float[] { i.focal_length[0], i.focal_length[1] },
                    PrincipalPoint = new float[] { i.principal_point[0], i.principal_point[1] },
                    DistortionCoeffs = new float[] { i.distortion_coeff[0], i.distortion_coeff[1], i.distortion_coeff[2], i.distortion_coeff[3], i.distortion_coeff[4] },
                    DistortionModel = i.distortion_model
                };
            }
        }

        public Extrinsics GetStreamExtrinsics(Stream from, Stream to)
        {
            Extrinsics extrin = new Extrinsics();
            IntPtr error = IntPtr.Zero;
            rs_get_stream_extrinsics(handle, from, to, ref extrin, ref error);
            Error.Handle(error);
            return extrin;
        }

        public void WaitAllStreams()
        {
            IntPtr error = IntPtr.Zero;
            rs_wait_all_streams(handle, ref error);
            Error.Handle(error);
        }

        public IntPtr GetImagePixels(Stream stream)
        {
            IntPtr error = IntPtr.Zero;
            var result = rs_get_image_pixels(handle, stream, ref error);
            Error.Handle(error);
            return result;
        }

        public int GetImageFrameNumber(Stream stream)
        {
            IntPtr error = IntPtr.Zero;
            var result = rs_get_image_frame_number(handle, stream, ref error);
            Error.Handle(error);
            return result;
        }

        #region P/Invoke declarations for backing C API
        private IntPtr handle;
        private unsafe struct rs_intrinsics
        {
            public fixed int image_size[2];
            public fixed float focal_length[2];
            public fixed float principal_point[2];
            public fixed float distortion_coeff[5];
            public Distortion distortion_model;
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
        [DllImport("realsense.dll")] private static extern void rs_get_stream_intrinsics(IntPtr camera, Stream stream, ref rs_intrinsics intrin, ref IntPtr error);
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

        public static void Ignore(IntPtr error)
        {
            if (error != IntPtr.Zero)
            {
                rs_free_error(error);
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
}
