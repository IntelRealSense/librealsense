using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.Windows.Threading;

namespace Intel.RealSense
{
    /// <summary>
    /// Interaction logic for Window.xaml
    /// </summary>
    public partial class CaptureWindow : Window
    {
        private Pipeline pipeline;
        private Colorizer colorizer;
        private CancellationTokenSource tokenSource = new CancellationTokenSource();

        static Action<VideoFrame> UpdateImage(Image img)
        {
            var wbmp = img.Source as WriteableBitmap;
            return new Action<VideoFrame>(frame =>
            {
                if (frame == null) return;
                
                var rect = new Int32Rect(0, 0, frame.Width, frame.Height);
                wbmp.WritePixels(rect, frame.Data, frame.Stride * frame.Height, frame.Stride);
            });
        }

        public CaptureWindow()
        {
            InitializeComponent();

            try
            {
                Action<VideoFrame> updateDepth;
                Action<VideoFrame> updateColor;

                // The colorizer processing block will be used to visualize the depth frames.
                colorizer = new Colorizer();

                // Create and config the pipeline to strem color and depth frames.
                pipeline = new Pipeline();
                var cfg = new Config();

                using (var ctx = new Context())
                {
                    var devices = ctx.QueryDevices();

                    if (( devices.Count != 1) || (!ExampleAutocalibrateDevice.IsTheDeviceD400Series(devices[0])))
                    {
                        Console.WriteLine("The tutorial {0} requires a single Realsense D400 device to run.\nFix the setup and rerun",
                            System.Diagnostics.Process.GetCurrentProcess().ProcessName);
                        Environment.Exit(1);
                    }

                    var dev = devices[0];
                    Console.WriteLine("Using device 0, an {0}", dev.Info[CameraInfo.Name]);
                    Console.WriteLine("    Serial number: {0}", dev.Info[CameraInfo.SerialNumber]);
                    Console.WriteLine("    Firmware version: {0}", dev.Info[CameraInfo.FirmwareVersion]);

                    var sensors = dev.QuerySensors();

                    var depthProfile = sensors
                                            .SelectMany(s => s.StreamProfiles)
                                            .Where(sp => sp.Stream == Stream.Depth)
                                            .Select(sp => sp.As<VideoStreamProfile>())
                                            .OrderBy(p => p.Framerate)
                                            .First();

                    var colorProfile = sensors
                                            .SelectMany(s => s.StreamProfiles)
                                            .Where(sp => sp.Stream == Stream.Color)
                                            .Select(sp => sp.As<VideoStreamProfile>())
                                            .OrderBy(p => p.Framerate)
                                            .First();


                    cfg.EnableDevice(dev.Info[CameraInfo.SerialNumber]);
                    cfg.EnableStream(Stream.Depth, depthProfile.Width, depthProfile.Height, depthProfile.Format, depthProfile.Framerate);
                    cfg.EnableStream(Stream.Color, colorProfile.Width, colorProfile.Height, colorProfile.Format, colorProfile.Framerate);


                    var pp = pipeline.Start(cfg);

                    SetupWindow(pp, out updateDepth, out updateColor);
                }

                // Rendering task
                var renderingPause = false;
                var rendering = Task.Factory.StartNew(() =>
                {
                    while (!tokenSource.Token.IsCancellationRequested)
                    {
                        if (renderingPause) continue; //pause the rendering

                        // We wait for the next available FrameSet and using it as a releaser object that would track
                        // all newly allocated .NET frames, and ensure deterministic finalization
                        // at the end of scope. 
                        using (var frames = pipeline.WaitForFrames())
                        {
                            var colorFrame = frames.ColorFrame.DisposeWith(frames);
                            var depthFrame = frames.DepthFrame.DisposeWith(frames);

                            // Render the frames.
                            if (depthFrame != null)
                            {
                                // We colorize the depth frame for visualization purposes
                                var colorizedDepth = colorizer.Process<VideoFrame>(depthFrame).DisposeWith(frames);
                                Dispatcher.Invoke(DispatcherPriority.Render, updateDepth, colorizedDepth);
                            }
                            if (colorFrame != null)
                                Dispatcher.Invoke(DispatcherPriority.Render, updateColor, colorFrame);

                            if (depthFrame != null)
                            {
                                Dispatcher.Invoke(new Action(() =>
                                {
                                    String depth_dev_sn = depthFrame.Sensor.Info[CameraInfo.SerialNumber];
                                    txtTimeStamp.Text = $"{depth_dev_sn} : {depthFrame.Timestamp,-20:0.00}({depthFrame.TimestampDomain})" +
                                    $"{Environment.NewLine}To start Auto-Calibration flow, switch focus to the application console and press C";
                                }));
                            }
                        }
                    }
                }, tokenSource.Token);

                // Input to calibration mode task
                Task.Factory.StartNew(() =>
                {
                    while (!tokenSource.Token.IsCancellationRequested)
                    {
                        if (ConsoleKey.C == ExampleAutocalibrateDevice.ConsoleGetKey(new[] { ConsoleKey.C},
                                         "To start Auto-Calibration flow, switch focus to the application console and press C"))
                        {
                            renderingPause = true;
                            Console.WriteLine($"{Environment.NewLine}Stopping rendering pipeline...");
                            pipeline.Stop();

                            new ExampleAutocalibrateDevice().Start();

                            Console.WriteLine($"{Environment.NewLine}Starting rendering pipeline...");
                            pipeline.Start(cfg);
                            renderingPause = false;
                        }
                    }
                }, tokenSource.Token);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
                Application.Current.Shutdown();
            }
        }

        private void control_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            tokenSource.Cancel();
        }

        private void SetupWindow(PipelineProfile pipelineProfile, out Action<VideoFrame> depth, out Action<VideoFrame> color)
        {
            using (var p = pipelineProfile.GetStream(Stream.Depth).As<VideoStreamProfile>())
                imgDepth.Source = new WriteableBitmap(p.Width, p.Height, 96d, 96d, PixelFormats.Rgb24, null);
            depth = UpdateImage(imgDepth);

            using (var p = pipelineProfile.GetStream(Stream.Color).As<VideoStreamProfile>())
                imgColor.Source = new WriteableBitmap(p.Width, p.Height, 96d, 96d, PixelFormats.Rgb24, null);
            color = UpdateImage(imgColor);
        }
    }
}
