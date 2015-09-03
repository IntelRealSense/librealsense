using System;
using System.Runtime.InteropServices;

namespace RealSense
{
    enum Stream : int
    {
        Depth                         = 0,
        Color                         = 1,
        Infrared                      = 2,
        Infrared2                     = 3,
    };

    enum Format : int
    {
        Any                           = 0,
        Z16                           = 1,
        Y8                            = 2,
        RGB8                          = 3,
    };

    enum Preset : int
    {
        BestQuality                   = 0,
        LargestImage                  = 1,
        HighestFramerate              = 2,
    };

    enum Distortion : int
    {
        None                          = 0,
        ModifiedBrownConrady          = 1,
        InverseBrownConrady           = 2,
    };

    enum Option : int
    {
        F200_LASER_POWER,                 /* 0 - 15 */
        F200_ACCURACY,                    /* 0 - 3 */
        F200_MOTION_RANGE,                /* 0 - 100 */
        F200_FILTER_OPTION,               /* 0 - 7 */
        F200_CONFIDENCE_THRESHOLD,        /* 0 - 15 */
        F200_DYNAMIC_FPS,                 /* {2, 5, 15, 30, 60} */
        R200_LR_AUTO_EXPOSURE_ENABLED,    /* {0, 1} */
        R200_LR_GAIN,                     /* 100 - 1600 (Units of 0.01) */
        R200_LR_EXPOSURE,                 /* > 0 (Units of 0.1 ms) */
        R200_EMITTER_ENABLED,             /* {0, 1} */
        R200_DEPTH_CONTROL_PRESET,        /* {0, 5}, 0 is default, 1-5 is low to high outlier rejection */
        R200_DEPTH_UNITS,                 /* > 0 */
        R200_DEPTH_CLAMP_MIN,             /* 0 - USHORT_MAX */
        R200_DEPTH_CLAMP_MAX,             /* 0 - USHORT_MAX */
        R200_DISPARITY_MODE_ENABLED,      /* {0, 1} */
        R200_DISPARITY_MULTIPLIER,
        R200_DISPARITY_SHIFT,
    };

    unsafe struct Intrinsics
    {
        public fixed int image_size[2];        
        public fixed float focal_length[2];    
        public fixed float principal_point[2]; 
        public fixed float distortion_coeff[5];
        public Distortion distortion_model;    
    };

    unsafe struct Extrinsics
    {
        public fixed float rotation[9];
        public fixed float translation[3];
    };

    class Context : IDisposable
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
        
        private const int ApiVersion = 1;

        #region P/Invoke declarations for backing C API
        private IntPtr handle;
        [DllImport("realsense.dll")] private static extern IntPtr rs_create_context(int api_version, ref IntPtr error);
        [DllImport("realsense.dll")] private static extern int rs_get_camera_count(IntPtr context, ref IntPtr error);
        [DllImport("realsense.dll")] private static extern IntPtr rs_get_camera(IntPtr context, int index, ref IntPtr error);
        [DllImport("realsense.dll")] private static extern void rs_delete_context(IntPtr context, ref IntPtr error);
        #endregion
    }

    class Camera
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
            int result = rs_is_stream_enabled(handle, stream, ref error);
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

        #region P/Invoke declarations for backing C API
        private IntPtr handle;
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

    class Error : Exception
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
