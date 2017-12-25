using System;
using System.Linq;

namespace Intel.RealSense
{
    class Program
    {
        static void Main(string[] args)
        {
            FrameQueue q = new FrameQueue();

            using (var ctx = new Context())
            {
                var devices = ctx.QueryDevices();

                Console.WriteLine("There are " + devices.Count + " connected RealSense devices.");
                if (devices.Count == 0) return;
                var dev = devices[0];

                Console.WriteLine("\nUsing device 0, an {0}", dev.Info[CameraInfo.Name]);
                Console.WriteLine("    Serial number: {0}", dev.Info[CameraInfo.SerialNumber]);
                Console.WriteLine("    Firmware version: {0}", dev.Info[CameraInfo.FirmwareVersion]);

                var depthSensor = dev.Sensors[0];

                var sp = depthSensor.VideoStreamProfiles
                                    .Where(p => p.Stream == Stream.Depth)
                                    .OrderByDescending(p => p.Framerate)
                                    .Where(p => p.Width == 640 && p.Height == 480)
                                    .First();
                depthSensor.Open(sp);
                depthSensor.Start(q);

                int one_meter = (int)(1f / depthSensor.DepthScale);

                var run = true;
                Console.CancelKeyPress += (s, e) =>
                {
                    e.Cancel = true;
                    run = false;
                };

                ushort[] depth = new ushort[sp.Width * sp.Height];

                while (run)
                {
                    using (var f = q.WaitForFrame() as VideoFrame)
                    {
                        f.CopyTo(depth);
                    }

                    var buffer = new char[(640 / 10 + 1) * (480 / 20)];
                    var coverage = new int[64];
                    int b = 0;
                    for (int y = 0; y < 480; ++y)
                    {
                        for (int x = 0; x < 640; ++x)
                        {
                            ushort d = depth[x + y * 640];
                            if (d > 0 && d < one_meter)
                                ++coverage[x / 10];
                        }

                        if (y % 20 == 19)
                        {
                            for (int i = 0; i < coverage.Length; i++)
                            {
                                int c = coverage[i];
                                buffer[b++] = " .:nhBXWW"[c / 25];
                                coverage[i] = 0;
                            }
                            buffer[b++] = '\n';
                        }
                    }

                    Console.SetCursorPosition(0, 0);
                    Console.WriteLine();
                    Console.Write(buffer);
                }

                depthSensor.Stop();
                depthSensor.Close();
            }

        }
    }
}
