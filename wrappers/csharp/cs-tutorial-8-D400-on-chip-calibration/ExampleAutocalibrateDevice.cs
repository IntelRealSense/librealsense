//#define CalibrationTestAndBurns2FW

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
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
                                { CalibrationSpeed.VeryFast,    Convert.ToInt32(TimeSpan.FromSeconds(0.66).TotalMilliseconds) },
                                { CalibrationSpeed.Fast,        Convert.ToInt32(TimeSpan.FromMinutes(1.33).TotalMilliseconds) },
                                { CalibrationSpeed.Medium,      Convert.ToInt32(TimeSpan.FromMinutes(2.84).TotalMilliseconds) },
                                { CalibrationSpeed.Slow,        Convert.ToInt32(TimeSpan.FromMinutes(2.84).TotalMilliseconds) },
                            };
        private enum DeviceHealth
        {
            Good,
            CouldBePerformed,
            NeedRecalibration,
            Failed
        }
        private Dictionary<DeviceHealth, string> _deviceHealthDescription = new Dictionary<DeviceHealth, string> {
                                {DeviceHealth.Good,              "Good" },
                                {DeviceHealth.CouldBePerformed,  "Could be performed" },
                                {DeviceHealth.NeedRecalibration, "Need recalibration" },
                                {DeviceHealth.Failed,            "Failed" },
                            };
        #endregion Consts

        public void Start(Device dev = null)
        {
            PrintStartProdures();

            var key = ConsoleGetKey(new[] { ConsoleKey.D1, ConsoleKey.D2, ConsoleKey.D3, ConsoleKey.D4 },
                    $"{Environment.NewLine}" +
                    $"Please choose:{Environment.NewLine}" +
                    $"1 - Calibrate{Environment.NewLine}" +
                    $"2 - Reset calibration to factory defaults{Environment.NewLine}" +
                    $"3 - Calibration modes test{Environment.NewLine}" +
                    $"4 - Exit{Environment.NewLine}");
            switch (key)
            {
                case ConsoleKey.D1:
                    Calibrate(dev);
                    break;
                case ConsoleKey.D2:
                    ResetToFactoryCalibration(dev);
                    break;
                case ConsoleKey.D3:
                    TestCalibrationModes(dev);
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
                if (dev == null)
                {
                    Console.WriteLine($"{Environment.NewLine}Getting context...");
                    dev = new Context().QueryDevices().First();
                }

                if (!IsTheDeviceD400Series(dev))
                    return;

                Console.WriteLine($"{Environment.NewLine}Calibration device {dev.Info[CameraInfo.Name]}" +
                                  $"{Environment.NewLine}\tSerial number: {dev.Info[CameraInfo.SerialNumber]}" +
                                  $"{Environment.NewLine}\tFirmware version: {dev.Info[CameraInfo.FirmwareVersion]}");

                VideoStreamProfile depthProfile = dev.QuerySensors()
                    .SelectMany(s => s.StreamProfiles)
                    .Where(sp => sp.Stream == Stream.Depth)
                    .Select(sp => sp.As<VideoStreamProfile>())
                    .Where(p =>
                        p.Width == 256
                        && p.Height == 144
                        && p.Framerate == 90
                        && p.Format == Format.Z16
                        ).Last();

                Console.WriteLine($"{Environment.NewLine}Starting pipeline for calibration mode");
                using (var pipeline = StartPipeline(depthProfile, dev))
                {
                    var aCalibratedDevice = AutoCalibratedDevice.FromDevice(dev);


                    // 1. Calibration table before running on-chip calibration. 
                    Console.WriteLine($"{Environment.NewLine}1. Calibration table before running on-chip calibration.");
                    var calTableBefore = aCalibratedDevice.CalibrationTable;
                    Console.WriteLine("Step 1 :  done");

                    // 2. Runs the on-chip self-calibration routine that returns pointer to new calibration table.

                    //calibratoin config
                    var calibrationSpeed = CalibrationSpeed.Slow;
                    var calibrationScanParameter = CalibrationScanParameter.Intrinsic;
                    var calibrationDataSampling = CalibrationDataSampling.WindowsAndLinux;
                    string calibrationConfig = GetCalibrationConfig(calibrationSpeed, calibrationScanParameter, calibrationDataSampling);

                    var timeout = _timeoutForCalibrationSpeed[calibrationSpeed];
                    float health = 100;
                    var succeedOnChipCalibration = false;
                    var abortRequested = false;
                    byte[] calTableAfter = null;
                    while (!succeedOnChipCalibration && !abortRequested)
                    {
                        Console.WriteLine($"{Environment.NewLine}2. Runs the on-chip self-calibration routine that returns pointer to new calibration table.");
                        Console.WriteLine($"\t Format                    : {depthProfile.Format}");
                        Console.WriteLine($"\t Framerate                 : {depthProfile.Framerate}");
                        Console.WriteLine($"\t Width                     : {depthProfile.Width}");
                        Console.WriteLine($"\t Height                    : {depthProfile.Height}");
                        Console.WriteLine($"\t Calibration Speed         : {calibrationSpeed}");
                        Console.WriteLine($"\t Calibration Scan Parameter: {calibrationScanParameter}");
                        Console.WriteLine($"\t Calibration Data Sampling : {calibrationDataSampling}");
                        Console.WriteLine($"\t Calibration Timeout       : {TimeSpan.FromMilliseconds(timeout).ToString(@"mm\:ss\.fff")} (min:sec.millisec)");

                        var sw = new Stopwatch();
                        var thisCalibrationfault = false;
                        try
                        {
                            sw.Start();
                            ProgressCallback pc = (x) => { Console.WriteLine("Progress: {0} percents",x); };
                            // The following line performs the same calibration flow but does not report progress"
                            //calTableAfter = aCalibratedDevice.RunOnChipCalibration(calibrationConfig, out health, timeout);
                            calTableAfter = aCalibratedDevice.RunOnChipCalibration(calibrationConfig, out health, pc,  timeout);
                            sw.Stop();
                        }
                        catch (Exception ex)
                        {
                            sw.Stop();
                            thisCalibrationfault = true;
                            Console.WriteLine($"\n\t Error during calibration: {ex.Message.Replace("\n", "\t")}");
                            Console.WriteLine($"\t Please try to change distance to target or light conditions{Environment.NewLine}");
                            if (ConsoleKey.N == ConsoleGetKey(new[] { ConsoleKey.Y, ConsoleKey.N },
                                                                @"Let's try calibrate one more time? (Y\N)"
                                                                ))
                            {
                                Console.WriteLine("");
                                Console.WriteLine($"Calibration failed");
                                Console.WriteLine($"Stopping calibration pipeline...");
                                pipeline.Stop();
                                return;
                            }
                        }

                        if (!thisCalibrationfault)
                        {
                            Console.WriteLine($"\n\t Time spend: {sw.Elapsed.ToString(@"mm\:ss\.fff")}  (min:sec.millisec)");
                            Console.WriteLine($"\t Device health: {health} ({_deviceHealthDescription[GetDeviceHealth(health)]})");

                            var res = ConsoleGetKey(new[] { ConsoleKey.Y, ConsoleKey.N, ConsoleKey.A }, @"Accept calibration ? Yes/No/Abort");
                            Console.WriteLine("User's selection = {0}", res);
                            if (res == ConsoleKey.A)
                                abortRequested = true;
                            else
                                succeedOnChipCalibration = (res == ConsoleKey.Y);
                        }
                    }

                    Console.WriteLine("Step 2 :  done");

                    // 3. Toggle between calibration tables to assess which is better. This is optional.   
                    Console.WriteLine($"{Environment.NewLine}3. Toggle between calibration tables to assess which is better.");
                    aCalibratedDevice.CalibrationTable = calTableAfter;
                    Console.WriteLine("Step 3 :  done");

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


        #region TestCalibrationModes
        public void TestCalibrationModes(Device dev = null)
        {

            try
            {

                if (dev == null)
                {
                    Console.WriteLine($"{Environment.NewLine}Getting context...");
                    dev = new Context().QueryDevices().First();
                }

                if (!IsTheDeviceD400Series(dev))
                    return;

                Console.WriteLine($"{Environment.NewLine}Test calibration modes using device {dev.Info[CameraInfo.Name]}" +
                                  $"{Environment.NewLine}\tSerial number: {dev.Info[CameraInfo.SerialNumber]}" +
                                  $"{Environment.NewLine}\tFirmware version: {dev.Info[CameraInfo.FirmwareVersion]}");
                var depthProfiles = dev.QuerySensors()
                    .SelectMany(s => s.StreamProfiles)
                    .Where(sp => sp.Stream == Stream.Depth)
                    .Select(sp => sp.As<VideoStreamProfile>())
                    .OrderByDescending(p => p.Height)
                    .OrderByDescending(p => p.Width)
                    .OrderByDescending(p => p.Framerate)
                    .OrderByDescending(p => p.Format)
                    .ToList();

                var fileName = Path.Combine(Directory.GetCurrentDirectory(), "ReportTestCalibrationModes.csv");
                Console.WriteLine($"Report about testing calibration modes will be written to file:\n\t{fileName}");

                var calibrationSetCount = 0;
                if (IsLinux)
                    calibrationSetCount = CountEnumEntries<CalibrationSpeed>() * CountEnumEntries<CalibrationScanParameter>();
                else
                    calibrationSetCount = CountEnumEntries<CalibrationSpeed>() * CountEnumEntries<CalibrationScanParameter>() * CountEnumEntries<CalibrationDataSampling>();

                var depthProfileSet = 0;
                var depthProfileSetCount = depthProfiles.Count;
                using (var file = new StreamWriter(fileName))
                {
                    var headers = GetHeader4FileReport();
                    file.WriteLine(headers);
                    file.Flush();

                    foreach (var depthProfile in depthProfiles)
                    {
                        Console.WriteLine($"{Environment.NewLine}Starting pipeline for calibration mode, depth profile set {++depthProfileSet} of {depthProfiles.Count}");
                        Console.WriteLine($"\t Format:\t {depthProfile.Format}");
                        Console.WriteLine($"\t Framerate:\t {depthProfile.Framerate}");
                        Console.WriteLine($"\t Width:\t\t {depthProfile.Width}");
                        Console.WriteLine($"\t Height:\t {depthProfile.Height}");
                        using (var pipeline = StartPipeline(depthProfile, dev))
                        {
                            var aCalibratedDevice = AutoCalibratedDevice.FromDevice(dev);

                            //calibratoin config set
                            var calibrationSet = 0;
                            foreach (var calibrationSpeed in GetEnumEntries<CalibrationSpeed>())
                            {
                                foreach (var calibrationScanParameter in GetEnumEntries<CalibrationScanParameter>())
                                {
                                    foreach (var calibrationDataSampling in GetEnumEntries<CalibrationDataSampling>())
                                    {
                                        if (calibrationDataSampling == CalibrationDataSampling.Windows && IsLinux)
                                            continue;
                                        Console.WriteLine("");
#if (CalibrationTestAndBurns2FW)
                                        // 1. Calibration table before running on-chip calibration. 
                                        Console.WriteLine($"{Environment.NewLine}1. Calibration table before running on-chip calibration.");
                                        var calTableBefore = aCalibratedDevice.CalibrationTable;
                                        Console.WriteLine("\t Step 1 :  done");
#endif
                                        // 2. Runs the on-chip self-calibration routine that returns pointer to new calibration table.
                                        Console.WriteLine($"2. Runs the on-chip self-calibration routine for depth profile {depthProfileSet} of {depthProfileSetCount} calibration, set {++calibrationSet} of {calibrationSetCount} ");
                                        Console.WriteLine($"\t Format                    : {depthProfile.Format}");
                                        Console.WriteLine($"\t Framerate                 : {depthProfile.Framerate}");
                                        Console.WriteLine($"\t Width                     : {depthProfile.Width}");
                                        Console.WriteLine($"\t Height                    : {depthProfile.Height}");
                                        Console.WriteLine($"\t Calibration Speed         : {calibrationSpeed}");
                                        Console.WriteLine($"\t Calibration Scan Parameter: {calibrationScanParameter}");
                                        Console.WriteLine($"\t Calibration Data Sampling : {calibrationDataSampling}");

                                        string calibrationConfig = GetCalibrationConfig(calibrationSpeed, calibrationScanParameter, calibrationDataSampling);

                                        //var timeout = _timeoutForCalibrationSpeed[calibrationSpeed];
                                        var timeout = Convert.ToInt32(TimeSpan.FromMinutes(2).TotalMilliseconds);
                                        float health = 888;
                                        var deviceHealth = DeviceHealth.Failed;
                                        byte[] calTableAfter = null;
                                        var sw = new Stopwatch();
                                        try
                                        {
                                            sw.Start();
                                            calTableAfter = aCalibratedDevice.RunOnChipCalibration(calibrationConfig, out health, timeout);
                                            sw.Stop();
                                            deviceHealth = GetDeviceHealth(health);
                                        }
                                        catch (Exception ex)
                                        {
                                            sw.Stop();
                                            //write to report file
                                            file.WriteLine(GetRow4FileReport(depthProfileSet, depthProfile, calibrationSet, calibrationSpeed, calibrationScanParameter, calibrationDataSampling, timeout, sw.Elapsed, "", _deviceHealthDescription[DeviceHealth.Failed], ex.Message.Replace("\n", "\t")));
                                            file.Flush();

                                            Console.WriteLine($"\t Time spend: {sw.Elapsed.ToString(@"mm\:ss\.fff")}");
                                            Console.WriteLine($"\t Calibration failed: {ex.Message.Replace("\n", "\t")}");
                                            Console.WriteLine("\t Step 2: failed");
                                            continue;
                                        }
                                        file.WriteLine(GetRow4FileReport(depthProfileSet, depthProfile, calibrationSet, calibrationSpeed, calibrationScanParameter, calibrationDataSampling, timeout, sw.Elapsed, health.ToString(), _deviceHealthDescription[GetDeviceHealth(health)]));
                                        file.Flush();

                                        Console.WriteLine($"\t Time spend: {sw.Elapsed.ToString(@"mm\:ss\.fff")}");
                                        Console.WriteLine($"\t Device health: {health} ({_deviceHealthDescription[GetDeviceHealth(health)]})");
                                        Console.WriteLine("\t Step 2: done");

#if (CalibrationTestAndBurns2FW)
                                        // 3. Toggle between calibration tables to assess which is better. This is optional.   
                                        Console.WriteLine("3. Toggle between calibration tables to assess which is better.");
                                        if (deviceHealth <= DeviceHealth.CouldBePerformed)
                                        {
                                            aCalibratedDevice.CalibrationTable = calTableAfter;
                                            Console.WriteLine("\t Step 3: done");
                                        }
                                        else
                                            Console.WriteLine("\t Step 3: skipped");

                                        // 4. burns the new calibration to FW persistently.
                                        Console.WriteLine("4. Burns the new calibration to FW persistently.");
                                        if (deviceHealth <= DeviceHealth.CouldBePerformed)
                                        {
                                            aCalibratedDevice.WriteCalibration();
                                            Console.WriteLine("\t Step 4: done");
                                        }
                                        else
                                            Console.WriteLine("\t Step 4: skipped");
#endif
                                    }
                                }
                            }

                            Console.WriteLine($"{Environment.NewLine}Stopping calibration pipeline for depth profile set {depthProfileSet} of {depthProfiles.Count}...");
                            pipeline.Stop();
                        }
                    }
                }
                Console.WriteLine($"Report about testing calibration modes has been written to file:\n\t{fileName}");
                Console.WriteLine("Testing calibration modes complete");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"{Environment.NewLine} Error during calibration:{Environment.NewLine} {ex.Message}");
            }

        }

        private string GetHeader4FileReport()
        {
            var row = new List<string>();
            row.Add("depthProfileSet");
            row.Add("Width");
            row.Add("Height");
            row.Add("Framerate");
            row.Add("Format");
            row.Add("calibrationSet");
            row.Add("(int)CalibrationSpeed");
            row.Add("(int)CalibrationScanParameter");
            row.Add("(int)CalibrationDataSampling");
            row.Add("CalibrationSpeed");
            row.Add("CalibrationScanParameter");
            row.Add("CalibrationDataSampling");
            //row.Add("timeout (min:sec.millisec)");
            row.Add("calibrationTakes (min:sec.millisec)");
            row.Add("Device health");
            row.Add("Device health description");
            row.Add("Comments");

            return string.Join(",", row);
        }
        private string GetRow4FileReport(
            int depthProfileSet,
            VideoStreamProfile profile,
            int calibrationSet,
            CalibrationSpeed calibrationSpeed,
            CalibrationScanParameter calibrationScanParameter,
            CalibrationDataSampling calibrationDataSampling,
            int timeout,
            TimeSpan calibrationTakes,
            string deviceHealth,
            string deviceHealthDescription,
            string comments = ""
            )
        {
            var row = new List<string>();
            row.Add($"{depthProfileSet}");
            row.Add($"{profile.Width}");
            row.Add($"{profile.Height}");
            row.Add($"{profile.Framerate}");
            row.Add($"{profile.Format}");
            row.Add($"{calibrationSet}");
            row.Add($"{(int)calibrationSpeed}");
            row.Add($"{(int)calibrationScanParameter}");
            row.Add($"{(int)calibrationDataSampling}");
            row.Add($"{calibrationSpeed}");
            row.Add($"{calibrationScanParameter}");
            row.Add($"{calibrationDataSampling}");
            //row.Add($@"{timeout:mm\:ss\.fff}"); 
            row.Add($@"{calibrationTakes:mm\:ss\.fff}");
            row.Add($"{deviceHealth}");
            row.Add($"{deviceHealthDescription}");
            row.Add($"{comments}");

            return string.Join(",", row);
        }
        private static bool IsLinux
        {
            get
            {
                //Spec https://docs.microsoft.com/en-us/dotnet/api/system.platformid?view=net-5
                //MacOSX  6
                //Other   7
                //Unix    4
                //Win32NT 2
                //Win32S  0
                //Win32Windows    1
                //WinCE   3
                //Xbox    5
                int p = (int)Environment.OSVersion.Platform;
                return (p == 4) || (p == 6) || (p == 128);
            }
        }
        private static int CountEnumEntries<T>() where T : struct, IConvertible
        {
            if (!typeof(T).IsEnum)
                throw new ArgumentException("T must be an enumerated type");

            return Enum.GetNames(typeof(T)).Length;
        }
        private static T[] GetEnumEntries<T>() where T : struct, IConvertible
        {
            if (!typeof(T).IsEnum)
                throw new ArgumentException("T must be an enumerated type");

            return (T[])Enum.GetValues(typeof(T));
        }

        #endregion CalibrationModesTest

        private Pipeline StartPipeline(VideoStreamProfile depthProfile, Device dev)
        {
            var cfg = new Config();
            cfg.EnableDevice(dev.Info[CameraInfo.SerialNumber]);
            cfg.EnableStream(Stream.Depth, depthProfile.Width, depthProfile.Height, depthProfile.Format, depthProfile.Framerate);

            var pipeline = new Pipeline();
            var pp = pipeline.Start(cfg);

            return pipeline;
        }
        private DeviceHealth GetDeviceHealth(double health)
        {
            health = System.Math.Abs(health);
            if (health < 0.25)
                return DeviceHealth.Good;
            else if (health < 0.75)
                return DeviceHealth.CouldBePerformed;
            else
                return DeviceHealth.NeedRecalibration;
        }
        private string GetCalibrationConfig(CalibrationSpeed calibrationSpeed, CalibrationScanParameter calibrationScanParameter, CalibrationDataSampling calibrationDataSampling)
        =>
                        $@"{{
	                        ""speed"": {(int)calibrationSpeed},
	                        ""scan parameter"": {(int)calibrationScanParameter},
	                        ""data sampling"": {(int)calibrationDataSampling}
                        }}";

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
            bool res = false;
            if (null != dev)
            {
                res = Regex.IsMatch(dev.Info[CameraInfo.Name].ToString(), @"(D4\d\d)");
                if (!res)
                    Console.WriteLine($"{Environment.NewLine}Device is not a D400 series model");
            }
            return res;
        }
        private void PrintStartProdures()
        {
            Console.WriteLine($"{Environment.NewLine}Preparing Procedures:");
            Console.WriteLine("\tRead carefully: https://dev.intelrealsense.com/docs/self-calibration-for-depth-cameras");
            Console.WriteLine("\tPrint the target: https://dev.intelrealsense.com/docs/self-calibration-for-depth-cameras#section-appendix-a-example-target");
            Console.WriteLine("\tPlace statically target around 1-1.5m away from your camera");
        }
    }
}

