using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;

namespace Intel.RealSense
{
    class ExampleAutocalibrateDevice
    {
        #region Consts
        private enum CalibrationSpeed
        {
            VeryFast,   // (0.66 seconds), for small depth degradation
            Fast,       // (1.33 seconds), medium depth degradation.This is the recommended setting.
            Medium,     // (2.84 seconds), medium depth degradation.
            Slow,       // (Default) (2.84 seconds), for significant depth degradation. 
        };
        private enum CalibrationScanParameter
        {
            Intrinsic,  // (default) Intrinsic calibration correction. The intrinsic distortion comes about through microscopic shifts in the lens positions
            Extrinsic,  // Extrinsic calibration correction. Extrinsic distortions are related to microscopic bending and twisting of the stiffener on which the two stereo sensors are mounted
        }
        private enum CalibrationDataSampling
        {
            WindowsAndLinux,// default is 0 which uses a polling approach that works on Windows and Linux
            Windows,        // Windows it is possible to select 1 which is interrupt data sampling which is slightly faster
        }
        private Dictionary<CalibrationSpeed, int> _timeoutForCalibrationSpeed = new Dictionary<CalibrationSpeed, int> {
                                { CalibrationSpeed.VeryFast,    Convert.ToInt32(System.Math.Round(0.66*10e3)) },
                                { CalibrationSpeed.Fast,        Convert.ToInt32(System.Math.Round(1.33*10e3)) },
                                { CalibrationSpeed.Medium,      Convert.ToInt32(System.Math.Round(2.84*10e3)) },
                                { CalibrationSpeed.Slow,        Convert.ToInt32(System.Math.Round(2.84*10e3)) },
                            };
        #endregion Consts

        public void Start(Device dev = null)
        {

            var key = ConsoleGetKey(new[] { ConsoleKey.D1, ConsoleKey.D2, ConsoleKey.D3 },
                    $"{Environment.NewLine}" +
                    $"Please choose:{Environment.NewLine}" +
                    $"1 - Calibrate{Environment.NewLine}" +
                    $"2 - Reset calibration to factory defaults{Environment.NewLine}" +
                    $"3 - Exit{Environment.NewLine}");
            switch (key)
            {
                case ConsoleKey.D1:
                    Calibrate(dev);
                    break;
                case ConsoleKey.D2:
                    ResetToFactoryCalibration(dev);
                    break;
                default:
                    return;
            }
        }
        private void ResetToFactoryCalibration(Device dev = null)
        {
            using (var ctx = new Context())
            {
                dev = dev ?? ctx.QueryDevices()[0];

                if (!IsTheDeviceD400Series(dev))
                    return;

                Console.WriteLine($"{Environment.NewLine}Reset to factory settings device an {dev.Info[CameraInfo.Name]}" +
                                  $"{Environment.NewLine}\tSerial number: {dev.Info[CameraInfo.SerialNumber]}" +
                                  $"{Environment.NewLine}\tFirmware version: {dev.Info[CameraInfo.FirmwareVersion]}");
                var aCalibratedDevice = AutoCalibratedDevice.FromDevice(dev);

                aCalibratedDevice.reset_to_factory_calibration();

                Console.WriteLine($"Device calibration has been reset to factory defaults");
            }
        }
        public void Calibrate(Device dev = null)
        {

            try
            {
                PrintStartProdures();
                
                if (dev == null) 
                {
                    Console.WriteLine($"{Environment.NewLine}Getting context...");
                    dev = new Context().QueryDevices()[0]; 
                }

                if (!IsTheDeviceD400Series(dev))
                    return;

                Console.WriteLine($"{Environment.NewLine}Calibration device {dev.Info[CameraInfo.Name]}" +
                                  $"{Environment.NewLine}\tSerial number: {dev.Info[CameraInfo.SerialNumber]}" +
                                  $"{Environment.NewLine}\tFirmware version: {dev.Info[CameraInfo.FirmwareVersion]}");

                VideoStreamProfile depthProfile = GetDepthProfile4CalibrationMode(dev);

                Console.WriteLine($"{Environment.NewLine}Starting pipeline for calibration mode");
                using (var pipeline = StartPipeline(depthProfile, dev))
                {
                    var aCalibratedDevice = AutoCalibratedDevice.FromDevice(dev);


                    // 1. Calibration table before running on-chip calibration. 
                    Console.WriteLine($"{Environment.NewLine}1. Calibration table before running on-chip calibration.");
                    var calTableBefore = aCalibratedDevice.CalibrationTable;
                    Console.WriteLine("Step 1 :  done");

                    // 2. Runs the on-chip self-calibration routine that returns pointer to new calibration table.
                    Console.WriteLine($"{Environment.NewLine}2. Runs the on-chip self-calibration routine that returns pointer to new calibration table.");

                    //calibratoin config
                    var calibrationSpeed = CalibrationSpeed.Slow;
                    var calibrationScanParameter = CalibrationScanParameter.Intrinsic;
                    var calibrationDataSampling = CalibrationDataSampling.WindowsAndLinux;
                    string calibrationConfig = GetCalibrationConfig(calibrationSpeed, calibrationScanParameter, calibrationDataSampling);

                    var timeout = _timeoutForCalibrationSpeed[calibrationSpeed];
                    float health = 100;
                    var succeedOnChipCalibration = false;
                    byte[] calTableAfter = null;
                    while (!succeedOnChipCalibration)
                    {
                        var thisCalibrationfault = false;
                        try
                        {
                            calTableAfter = aCalibratedDevice.RunOnChipCalibration(calibrationConfig, out health, timeout);
                        }
                        catch (Exception ex)
                        {
                            thisCalibrationfault = true;
                            Console.WriteLine($"{Environment.NewLine} Error during calibration:{Environment.NewLine} {ex.Message}");
                            Console.WriteLine($"Please try to change distance to target{Environment.NewLine}");
                            if (ConsoleKey.N == ConsoleGetKey(new[] { ConsoleKey.Y, ConsoleKey.N },
                                                                @"Let's try calibrate one more time? (Y\N)"
                                                                ))
                            {
                                Console.WriteLine($"{Environment.NewLine}Calibration fault");
                                Console.WriteLine($"{Environment.NewLine}Stopping calibration pipeline...");
                                pipeline.Stop();
                                return;
                            }
                        }

                        if (!thisCalibrationfault)
                        {
                            Console.WriteLine($"Device health={health} (< 0.25 Good < 0.75 Could be performed < 0.75 Need Recalibration)");
                            if (ConsoleKey.Y == ConsoleGetKey(new[] { ConsoleKey.Y, ConsoleKey.N },
                                                      @"Accept calibration? (Y\N)"))
                                succeedOnChipCalibration = true;
                        }
                    }

                    Console.WriteLine("Step 2 :  done");

                    // 3. Toggle between calibration tables to assess which is better. This is optional.   
                    Console.WriteLine($"{Environment.NewLine}3. Toggle between calibration tables to assess which is better. This is optional. ");
                    if (ConsoleKey.Y == ConsoleGetKey(new[] { ConsoleKey.Y, ConsoleKey.N },
                                                      @"Compare and select better calibration table between current and FW device (Y\N)"))
                    {
                        aCalibratedDevice.CalibrationTable = calTableAfter;
                        Console.WriteLine("Step 3 :  done");
                    }
                    else
                        Console.WriteLine("Step 3 :  skipped");

                    // 4. burns the new calibration to FW persistently. 
                    Console.WriteLine("");
                    if (ConsoleKey.Y == ConsoleGetKey(new[] { ConsoleKey.Y, ConsoleKey.N },
                                                      @"4. Burns the new calibration to FW persistently. (Y\N)"))
                    {
                        aCalibratedDevice.WriteCalibration();
                        Console.WriteLine("Step 4 :  done");
                    }
                    else
                        Console.WriteLine("Step 4 :  skipped");

                    Console.WriteLine("Calibration complete");
                    Console.WriteLine($"{Environment.NewLine}Stopping calibration pipeline...");

                    pipeline.Stop();
                }

            //}
            }
            catch (Exception ex)
            {
                Console.WriteLine($"{Environment.NewLine} Error during calibration:{Environment.NewLine} {ex.Message}");

            }

        }

        /// <summary>
        /// Get recomended calibration config according suggestions on
        /// https://dev.intelrealsense.com/docs/self-calibration-for-depth-cameras
        /// </summary>
        /// <returns></returns>
        private string GetCalibrationConfig(CalibrationSpeed calibrationSpeed, CalibrationScanParameter calibrationScanParameter, CalibrationDataSampling calibrationDataSampling)
        =>
                        $@"{{
	                        ""speed"": {(int)calibrationSpeed},
	                        ""scan parameter"": {(int)calibrationScanParameter},
	                        ""data sampling"": {(int)calibrationDataSampling}
                        }}";
        /// <summary>
        /// Get the device depth profile, which using in calibration mode
        /// note: the calibration will afects also to other profiles
        /// </summary>
        private VideoStreamProfile GetDepthProfile4CalibrationMode(Device dev)
            => dev.QuerySensors()
                    .SelectMany(s => s.StreamProfiles)
                    .Where(sp => sp.Stream == Stream.Depth)
                    .Select(sp => sp.As<VideoStreamProfile>())
                    .Where(p =>
                        p.Width == 256
                        && p.Height == 144
                        && p.Framerate == 90
                        && p.Format == Format.Z16
                        ).Last();
        private Pipeline StartPipeline(VideoStreamProfile depthProfile, Device dev)
        {
            var cfg = new Config();
            cfg.EnableDevice(dev.Info[CameraInfo.SerialNumber]);
            cfg.EnableStream(Stream.Depth, depthProfile.Width, depthProfile.Height, depthProfile.Format, depthProfile.Framerate);

            var pipeline = new Pipeline();
            var pp = pipeline.Start(cfg);

            return pipeline;
        }

        public static ConsoleKey ConsoleGetKey(ConsoleKey[] possibleKeys, string prompt)
        {
            ConsoleKey res = ConsoleKey.Enter;
            do
            {
                Console.WriteLine(prompt);
                res = Console.ReadKey(true).Key;
            }
            while (!possibleKeys.Contains(res));

            return res;
        }
        public static bool IsTheDeviceD400Series(Device dev)
        {
            var res = Regex.IsMatch(dev.Info[CameraInfo.Name].ToString(), @"(D4\d\d)");
            if (!res)
                Console.WriteLine($"{Environment.NewLine}The devices of D400 series can be calibrated only!");
            return res;
        }
        private void PrintStartProdures()
        {
            Console.WriteLine($"{Environment.NewLine}Preparing Procedures:");
            Console.WriteLine("\tRead carefully: https://dev.intelrealsense.com/docs/self-calibration-for-depth-cameras");
            Console.WriteLine("\tPrint the target: https://dev.intelrealsense.com/docs/self-calibration-for-depth-cameras#section-appendix-a-example-target");
            Console.WriteLine("\tPlace statically target around 1-1.5m away from your camera");
            Console.WriteLine("Press enter...");
            Console.ReadLine();
        }
    }
}

