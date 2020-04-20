using System;
using System.Collections.Generic;
using System.Diagnostics;
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

                pipeline = new Pipeline();
                colorizer = new Colorizer();

                var cfg = new Config();
                cfg.EnableStream(Stream.Depth, 640, 480, Format.Z16, 30);
                cfg.EnableStream(Stream.Color, 640, 480, Format.Rgb8, 30);

                var profile = pipeline.Start(cfg);

                SetupWindow(profile, out updateDepth, out updateColor);

                // Setup the SW device and sensors
                var software_dev = new SoftwareDevice();
                var depth_sensor = software_dev.AddSensor("Depth");
                var depth_profile = depth_sensor.AddVideoStream(new SoftwareVideoStream
                {
                    type = Stream.Depth,
                    index = 0,
                    uid = 100,
                    width = 640,
                    height = 480,
                    fps = 30,
                    bpp = 2,
                    format = Format.Z16,
                    intrinsics = profile.GetStream(Stream.Depth).As<VideoStreamProfile>().GetIntrinsics()
                });
                var color_sensor = software_dev.AddSensor("Color");
                var color_profile = color_sensor.AddVideoStream(new SoftwareVideoStream
                {
                    type = Stream.Color,
                    index = 0,
                    uid = 101,
                    width = 640,
                    height = 480,
                    fps = 30,
                    bpp = 3,
                    format = Format.Rgb8,
                    intrinsics = profile.GetStream(Stream.Color).As<VideoStreamProfile>().GetIntrinsics()
                });

                // Note about the Syncer: If actual FPS is significantly different from reported FPS in AddVideoStream
                // this can confuse the syncer and prevent it from producing synchronized pairs
                software_dev.SetMatcher(Matchers.Default);

                var sync = new Syncer();

                depth_sensor.Open(depth_profile);
                color_sensor.Open(color_profile);

                // Push the SW device frames to the syncer
                depth_sensor.Start(sync.SubmitFrame);
                color_sensor.Start(sync.SubmitFrame);

                var token = tokenSource.Token;

                ushort[] depthData = null;
                byte[] colorData = null;

                var t = Task.Factory.StartNew(() =>
                {
                    while (!token.IsCancellationRequested)
                    {
                        // We use the frames that are captured from live camera as the input data for the SW device
                        using (var frames = pipeline.WaitForFrames())
                        {
                            var depthFrame = frames.DepthFrame.DisposeWith(frames);
                            var colorFrame = frames.ColorFrame.DisposeWith(frames);

                            depthData = depthData ?? new ushort[depthFrame.Width * depthFrame.Height];
                            depthFrame.CopyTo(depthData);
                            depth_sensor.AddVideoFrame(depthData, depthFrame.Stride, depthFrame.BitsPerPixel / 8, depthFrame.Timestamp,
                                depthFrame.TimestampDomain, (int)depthFrame.Number, depth_profile);

                            colorData = colorData ?? new byte[colorFrame.Stride * colorFrame.Height];
                            colorFrame.CopyTo(colorData);
                            color_sensor.AddVideoFrame(colorData, colorFrame.Stride, colorFrame.BitsPerPixel / 8, colorFrame.Timestamp,
                                colorFrame.TimestampDomain, (int)colorFrame.Number, color_profile);
                        }

                        // Dispaly the frames that come from the SW device after synchronization
                        using (var new_frames = sync.WaitForFrames())
                        {
                            if (new_frames.Count == 2)
                            {
                                var depthFrame = new_frames.DepthFrame.DisposeWith(new_frames);
                                var colorFrame = new_frames.ColorFrame.DisposeWith(new_frames);

                                var colorizedDepth = colorizer.Process<VideoFrame>(depthFrame).DisposeWith(new_frames);

                                // Render the frames.
                                Dispatcher.Invoke(DispatcherPriority.Render, updateDepth, colorizedDepth);
                                Dispatcher.Invoke(DispatcherPriority.Render, updateColor, colorFrame);
                            }
                        }
                    }
                }, token);
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
