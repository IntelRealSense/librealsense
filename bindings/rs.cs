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
        Z16 = 1, // 16 bit linear depth values. The depth is meters is equal to depth scale * pixel value
        Disparity16 = 2, // 16 bit linear disparity values. The depth in meters is equal to depth scale / pixel value
        YUYV = 3,
        RGB8 = 4,
        BGR8 = 5,
        RGBA8 = 6,
        BGRA8 = 7,
        Y8 = 8,
        Y16 = 9,
        Raw10 = 10 // Four 10-bit luminance values encoded into a 5-byte macropixel
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
        ColorExposure = 3, // Controls exposure time of color camera. Setting any value will disable auto exposure.
        ColorGain = 4,
        ColorGamma = 5,
        ColorHue = 6,
        ColorSaturation = 7,
        ColorSharpness = 8,
        ColorWhiteBalance = 9, // Controls white balance of color image. Setting any value will disable auto white balance.
        ColorEnableAutoExposure = 10, // Set to 1 to enable automatic exposure control, or 0 to return to manual control
        ColorEnableAutoWhiteBalance = 11, // Set to 1 to enable automatic white balance control, or 0 to return to manual control
        F200LaserPower = 12, // 0 - 15
        F200Accuracy = 13, // 0 - 3
        F200MotionRange = 14, // 0 - 100
        F200FilterOption = 15, // 0 - 7
        F200ConfidenceThreshold = 16, // 0 - 15
        F200DynamicFPS = 17, // {2, 5, 15, 30, 60}
        R200LRAutoExposureEnabled = 18, // {0, 1}
        R200LRGain = 19, // 100 - 1600 (Units of 0.01)
        R200LRExposure = 20, // > 0 (Units of 0.1 ms)
        R200EmitterEnabled = 21, // {0, 1}
        R200DepthUnits = 22, // micrometers per increment in integer depth values, 1000 is default (mm scale)
        R200DepthClampMin = 23, // 0 - USHORT_MAX
        R200DepthClampMax = 24, // 0 - USHORT_MAX
        R200DisparityMultiplier = 25, // 0 - 1000, the increments in integer disparity values corresponding to one pixel of disparity
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

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct F200AutoRangeParameters
    {
        public int EnableMotionVersusRange; // 
        public int EnableLaser; // 
        public int MinMotionVersusRange; // 
        public int MaxMotionVersusRange; // 
        public int StartMotionVersusRange; // 
        public int MinLaser; // 
        public int MaxLaser; // 
        public int StartLaser; // 
        public int AutoRangeUpperThreshold; // 
        public int AutoRangeLowerThreshold; // 
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct R200LRAutoExposureParameters
    {
        public float MeanIntensitySetPoint; // 
        public float BrightRatioSetPoint; // 
        public float KPGain; // 
        public float KPExposure; // 
        public float KPDarkThreshold; // 
        public int ExposureTopEdge; // 
        public int ExposureBottomEdge; // 
        public int ExposureLeftEdge; // 
        public int ExposureRightEdge; // 
    }

    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    public struct R200DepthControlParameters
    {
        public int EstimateMedianDecrement; // 
        public int EstimateMedianIncrement; // 
        public int MedianThreshold; // 
        public int ScoreMinimumThreshold; // 
        public int ScoreMaximumThreshold; // 
        public int TextureCountThreshold; // 
        public int TextureDifferenceThreshold; // 
        public int SecondPeakThreshold; // 
        public int NeighborThreshold; // 
        public int LRThreshold; // 
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
            handle = rs_create_context(4, ref e);
            Error.Handle(e);
        }

        public void Dispose()
        {
            rs_delete_context(handle, IntPtr.Zero);
        }

        /// <summary> determine number of connected devices </summary>
        /// <returns> the count of devices </returns>
        public int GetDeviceCount()
        {
            IntPtr e = IntPtr.Zero;
            var r = rs_get_device_count(handle, ref e);
            Error.Handle(e);
            return r;
        }

        /// <summary> retrieve connected device by index </summary>
        /// <param name="index"> the zero based index of device to retrieve </param>
        /// <returns> the requested device </returns>
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

        /// <summary> retrieve a human readable device model string </summary>
        /// <returns> the model string, such as "Intel RealSense F200" or "Intel RealSense R200" </returns>
        public string GetName()
        {
            IntPtr e = IntPtr.Zero;
            var r = rs_get_device_name(handle, ref e);
            Error.Handle(e);
            return Marshal.PtrToStringAnsi(r);
        }

        /// <summary> retrieve the unique serial number of the device </summary>
        /// <returns> the serial number, in a format specific to the device model </returns>
        public string GetSerial()
        {
            IntPtr e = IntPtr.Zero;
            var r = rs_get_device_serial(handle, ref e);
            Error.Handle(e);
            return Marshal.PtrToStringAnsi(r);
        }

        /// <summary> retrieve the version of the firmware currently installed on the device </summary>
        /// <returns> firmware version string, in a format is specific to device model </returns>
        public string GetFirmwareVersion()
        {
            IntPtr e = IntPtr.Zero;
            var r = rs_get_device_firmware_version(handle, ref e);
            Error.Handle(e);
            return Marshal.PtrToStringAnsi(r);
        }

        /// <summary> retrieve extrinsic transformation between the viewpoints of two different streams </summary>
        /// <param name="fromStream"> stream whose coordinate space we will transform from </param>
        /// <param name="toStream"> stream whose coordinate space we will transform to </param>
        /// <returns> the transformation between the two streams </returns>
        public Extrinsics GetExtrinsics(Stream fromStream, Stream toStream)
        {
            IntPtr e = IntPtr.Zero;
            Extrinsics extrin;
            rs_get_device_extrinsics(handle, fromStream, toStream, out extrin, ref e);
            Error.Handle(e);
            return extrin;
        }

        /// <summary> retrieve mapping between the units of the depth image and meters </summary>
        /// <returns> depth in meters corresponding to a depth value of 1 </returns>
        public float GetDepthScale()
        {
            IntPtr e = IntPtr.Zero;
            var r = rs_get_device_depth_scale(handle, ref e);
            Error.Handle(e);
            return r;
        }

        /// <summary> determine if the device allows a specific option to be queried and set </summary>
        /// <param name="option"> the option to check for support </param>
        /// <returns> true if the option can be queried and set </returns>
        public bool SupportsOption(Option option)
        {
            IntPtr e = IntPtr.Zero;
            var r = rs_device_supports_option(handle, option, ref e);
            Error.Handle(e);
            return r != 0;
        }

        /// <summary> determine the range of acceptable values for an option on this device </summary>
        /// <param name="option"> the option whose range to query </param>
        /// <param name="min"> the minimum acceptable value, attempting to set a value below this will take no effect and raise an error </param>
        /// <param name="max"> the maximum acceptable value, attempting to set a value above this will take no effect and raise an error </param>
        public void GetOptionRange(Option option, out int min, out int max)
        {
            IntPtr e = IntPtr.Zero;
            rs_get_device_option_range(handle, option, out min, out max, ref e);
            Error.Handle(e);
        }

        /// <summary> determine the number of streaming modes available for a given stream </summary>
        /// <param name="stream"> the stream whose modes will be enumerated </param>
        /// <returns> the count of available modes </returns>
        public int GetStreamModeCount(Stream stream)
        {
            IntPtr e = IntPtr.Zero;
            var r = rs_get_stream_mode_count(handle, stream, ref e);
            Error.Handle(e);
            return r;
        }

        /// <summary> determine the properties of a specific streaming mode </summary>
        /// <param name="stream"> the stream whose mode will be queried </param>
        /// <param name="index"> the zero based index of the streaming mode </param>
        /// <param name="width"> the width of a frame image in pixels </param>
        /// <param name="height"> the height of a frame image in pixels </param>
        /// <param name="format"> the pixel format of a frame image </param>
        /// <param name="framerate"> the number of frames which will be streamed per second </param>
        public void GetStreamMode(Stream stream, int index, out int width, out int height, out Format format, out int framerate)
        {
            IntPtr e = IntPtr.Zero;
            rs_get_stream_mode(handle, stream, index, out width, out height, out format, out framerate, ref e);
            Error.Handle(e);
        }

        /// <summary> enable a specific stream and request specific properties </summary>
        /// <param name="stream"> the stream to enable </param>
        /// <param name="width"> the desired width of a frame image in pixels, or 0 if any width is acceptable </param>
        /// <param name="height"> the desired height of a frame image in pixels, or 0 if any height is acceptable </param>
        /// <param name="format"> the pixel format of a frame image, or ANY if any format is acceptable </param>
        /// <param name="framerate"> the number of frames which will be streamed per second, or 0 if any framerate is acceptable </param>
        public void EnableStream(Stream stream, int width, int height, Format format, int framerate)
        {
            IntPtr e = IntPtr.Zero;
            rs_enable_stream(handle, stream, width, height, format, framerate, ref e);
            Error.Handle(e);
        }

        /// <summary> enable a specific stream and request properties using a preset </summary>
        /// <param name="stream"> the stream to enable </param>
        /// <param name="preset"> the preset to use to enable the stream </param>
        public void EnableStream(Stream stream, Preset preset)
        {
            IntPtr e = IntPtr.Zero;
            rs_enable_stream_preset(handle, stream, preset, ref e);
            Error.Handle(e);
        }

        /// <summary> disable a specific stream </summary>
        /// <param name="stream"> the stream to disable </param>
        public void DisableStream(Stream stream)
        {
            IntPtr e = IntPtr.Zero;
            rs_disable_stream(handle, stream, ref e);
            Error.Handle(e);
        }

        /// <summary> determine if a specific stream is enabled </summary>
        /// <param name="stream"> the stream to check </param>
        /// <returns> true if the stream is currently enabled </returns>
        public bool IsStreamEnabled(Stream stream)
        {
            IntPtr e = IntPtr.Zero;
            var r = rs_is_stream_enabled(handle, stream, ref e);
            Error.Handle(e);
            return r != 0;
        }

        /// <summary> retrieve intrinsic camera parameters for a specific stream </summary>
        /// <param name="stream"> the stream whose parameters to retrieve </param>
        /// <returns> the intrinsic parameters of the stream </returns>
        public Intrinsics GetStreamIntrinsics(Stream stream)
        {
            IntPtr e = IntPtr.Zero;
            Intrinsics intrin;
            rs_get_stream_intrinsics(handle, stream, out intrin, ref e);
            Error.Handle(e);
            return intrin;
        }

        /// <summary> retrieve the pixel format for a specific stream </summary>
        /// <param name="stream"> the stream whose format to retrieve </param>
        /// <returns> the pixel format of the stream </returns>
        public Format GetStreamFormat(Stream stream)
        {
            IntPtr e = IntPtr.Zero;
            var r = rs_get_stream_format(handle, stream, ref e);
            Error.Handle(e);
            return r;
        }

        /// <summary> retrieve the framerate for a specific stream </summary>
        /// <param name="stream"> the stream whose framerate to retrieve </param>
        /// <returns> the framerate of the stream, in frames per second </returns>
        public int GetStreamFramerate(Stream stream)
        {
            IntPtr e = IntPtr.Zero;
            var r = rs_get_stream_framerate(handle, stream, ref e);
            Error.Handle(e);
            return r;
        }

        /// <summary> begin streaming on all enabled streams for this device </summary>
        public void Start()
        {
            IntPtr e = IntPtr.Zero;
            rs_start_device(handle, ref e);
            Error.Handle(e);
        }

        /// <summary> end streaming on all streams for this device </summary>
        public void Stop()
        {
            IntPtr e = IntPtr.Zero;
            rs_stop_device(handle, ref e);
            Error.Handle(e);
        }

        /// <summary> determine if the device is currently streaming </summary>
        /// <returns> true if the device is currently streaming </returns>
        public bool IsStreaming()
        {
            IntPtr e = IntPtr.Zero;
            var r = rs_is_device_streaming(handle, ref e);
            Error.Handle(e);
            return r != 0;
        }

        /// <summary> set the value of a specific device option </summary>
        /// <param name="option"> the option whose value to set </param>
        /// <param name="value"> the desired value to set </param>
        public void SetOption(Option option, int value)
        {
            IntPtr e = IntPtr.Zero;
            rs_set_device_option(handle, option, value, ref e);
            Error.Handle(e);
        }

        /// <summary> query the current value of a specific device option </summary>
        /// <param name="option"> the option whose value to retrieve </param>
        /// <returns> the current value of the option </returns>
        public int GetOption(Option option)
        {
            IntPtr e = IntPtr.Zero;
            var r = rs_get_device_option(handle, option, ref e);
            Error.Handle(e);
            return r;
        }

        public void SetAutoRangeParameters(F200AutoRangeParameters parameters)
        {
            IntPtr e = IntPtr.Zero;
            rs_set_auto_range_parameters(handle, ref parameters, ref e);
            Error.Handle(e);
        }

        public F200AutoRangeParameters GetAutoRangeParameters()
        {
            IntPtr e = IntPtr.Zero;
            F200AutoRangeParameters parameters;
            rs_get_auto_range_parameters(handle, out parameters, ref e);
            Error.Handle(e);
            return parameters;
        }

        public void SetLRAutoExposureParameters(R200LRAutoExposureParameters parameters)
        {
            IntPtr e = IntPtr.Zero;
            rs_set_lr_auto_exposure_parameters(handle, ref parameters, ref e);
            Error.Handle(e);
        }

        public R200LRAutoExposureParameters GetLRAutoExposureParameters()
        {
            IntPtr e = IntPtr.Zero;
            R200LRAutoExposureParameters parameters;
            rs_get_lr_auto_exposure_parameters(handle, out parameters, ref e);
            Error.Handle(e);
            return parameters;
        }

        public void SetDepthControlParameters(R200DepthControlParameters parameters)
        {
            IntPtr e = IntPtr.Zero;
            rs_set_depth_control_parameters(handle, ref parameters, ref e);
            Error.Handle(e);
        }

        public R200DepthControlParameters GetDepthControlParameters()
        {
            IntPtr e = IntPtr.Zero;
            R200DepthControlParameters parameters;
            rs_get_depth_control_parameters(handle, out parameters, ref e);
            Error.Handle(e);
            return parameters;
        }

        /// <summary> block until new frames are available </summary>
        public void WaitForFrames()
        {
            IntPtr e = IntPtr.Zero;
            rs_wait_for_frames(handle, ref e);
            Error.Handle(e);
        }

        /// <summary> retrieve the time at which the latest frame on a stream was captured </summary>
        /// <param name="stream"> the stream whose latest frame we are interested in </param>
        /// <returns> the timestamp of the frame, in milliseconds since the device was started </returns>
        public int GetFrameTimestamp(Stream stream)
        {
            IntPtr e = IntPtr.Zero;
            var r = rs_get_frame_timestamp(handle, stream, ref e);
            Error.Handle(e);
            return r;
        }

        /// <summary> retrieve the contents of the latest frame on a stream </summary>
        /// <param name="stream"> the stream whose latest frame we are interested in </param>
        /// <returns> the pointer to the start of the frame data </returns>
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
        [DllImport("realsense")] private static extern void rs_get_device_option_range(IntPtr device, Option option, out int min, out int max, ref IntPtr error);
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
        [DllImport("realsense")] private static extern void rs_set_auto_range_parameters(IntPtr device, ref F200AutoRangeParameters parameters, ref IntPtr error);
        [DllImport("realsense")] private static extern void rs_get_auto_range_parameters(IntPtr device, out F200AutoRangeParameters parameters, ref IntPtr error);
        [DllImport("realsense")] private static extern void rs_set_lr_auto_exposure_parameters(IntPtr device, ref R200LRAutoExposureParameters parameters, ref IntPtr error);
        [DllImport("realsense")] private static extern void rs_get_lr_auto_exposure_parameters(IntPtr device, out R200LRAutoExposureParameters parameters, ref IntPtr error);
        [DllImport("realsense")] private static extern void rs_set_depth_control_parameters(IntPtr device, ref R200DepthControlParameters parameters, ref IntPtr error);
        [DllImport("realsense")] private static extern void rs_get_depth_control_parameters(IntPtr device, out R200DepthControlParameters parameters, ref IntPtr error);
        [DllImport("realsense")] private static extern void rs_wait_for_frames(IntPtr device, ref IntPtr error);
        [DllImport("realsense")] private static extern int rs_get_frame_timestamp(IntPtr device, Stream stream, ref IntPtr error);
        [DllImport("realsense")] private static extern IntPtr rs_get_frame_data(IntPtr device, Stream stream, ref IntPtr error);
    }
}
