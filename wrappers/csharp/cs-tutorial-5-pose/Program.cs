using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;

namespace Intel.RealSense
{
    class Program
    {
        static void Main(string[] args)
        {
            using (var ctx = new Context())
            {
                DeviceList devices = ctx.QueryDevices();
                if (devices.Count == 0)
                {
                    Console.WriteLine("RealSense devices are not connected.");
                    return;
                }

                using (var config = new Config())
                using (var pipeline = new Pipeline(ctx))
                {
                    // Add pose stream
                    config.EnableStream(Stream.Pose, Format.SixDOF);
                    // Start pipeline with chosen configuration
                    using (var profile = pipeline.Start(config))
                    using (var streamprofile = profile.GetStream(Stream.Pose).As<PoseStreamProfile>())
                    {
                        Console.WriteLine($"\nDevice : {profile.Device.Info[CameraInfo.Name]}");
                        Console.WriteLine($"    Serial number: {profile.Device.Info[CameraInfo.SerialNumber]}");
                        Console.WriteLine($"    Firmware version: {profile.Device.Info[CameraInfo.FirmwareVersion]}");
                        Console.WriteLine($"    Pose stream framerate: {streamprofile.Framerate}\n");
                    }

                    while (true)
                    {
                        // Wait for the next set of frames from the camera
                        using (FrameSet frameset = pipeline.WaitForFrames())
                        // Get a frame from the pose stream
                        using (Frame frame = frameset.FirstOrDefault(Stream.Pose))
                            if (frame != null)
                            {
                                // Cast the frame to pose_frame and get its data
                                Pose data = frame.As<PoseFrame>().PoseData;

                                // Print the x, y, z values of the translation, relative to initial position
                                Console.WriteLine("Device Position: {0} {1} {2} (meters)", data.translation.x.ToString("N3"), data.translation.y.ToString("N3"), data.translation.z.ToString("N3"));
                            }

                        Thread.Sleep(100);
                    }
                }
            }
        }
    }
}
