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


namespace Intel.RealSense
{
    /// <summary>
    /// Interaction logic for Window.xaml
    /// </summary>
    public partial class CaptureWindow : Window
    {
        private Pipeline  pipeline;
        private Colorizer colorizer;
        private CancellationTokenSource tokenSource = new CancellationTokenSource();

        private void UploadImage(Image img, VideoFrame frame)
        {
            Dispatcher.Invoke(new Action(() =>
            {
                if (frame.Width == 0) return;

                var bytes = new byte[frame.Stride * frame.Height];
                frame.CopyTo(bytes);

                var bs = BitmapSource.Create(frame.Width, frame.Height,
                                  300, 300,
                                  PixelFormats.Rgb24,
                                  null,
                                  bytes,
                                  frame.Stride);

                var imgSrc = bs as ImageSource;

                img.Source = imgSrc;
            }));
        }

        public CaptureWindow()
        {
            //Log.ToFile(LogSeverity.Debug, "1.log");

            try
            {
                pipeline = new Pipeline();
                colorizer = new Colorizer();

                var cfg = new Config();
                cfg.EnableStream(Stream.Depth, 640, 480, Format.Z16,  30);
                cfg.EnableStream(Stream.Color, 640, 480, Format.Rgb8, 30);

                var profile = pipeline.Start(cfg);

                var software_dev = new SoftwareDevice();
                var depth_sensor = software_dev.AddSensor("Depth");
                var depth_profile = depth_sensor.AddVideoStream(new VideoStream
                {
                    type = Stream.Depth,
                    index = 0,
                    uid = 100,
                    width = 640,
                    height = 480,
                    fps = 30,
                    bpp = 2,
                    fmt = Format.Z16,
                    intrinsics = (profile.GetStream(Stream.Depth) as VideoStreamProfile).GetIntrinsics()
                });
                var color_sensor = software_dev.AddSensor("Color");
                var color_profile = color_sensor.AddVideoStream(new VideoStream
                {
                    type = Stream.Color,
                    index = 0,
                    uid = 101,
                    width = 640,
                    height = 480,
                    fps = 30,
                    bpp = 2,
                    fmt = Format.Z16,
                    intrinsics = (profile.GetStream(Stream.Color) as VideoStreamProfile).GetIntrinsics()
                });
                // Note about the Syncer: If actual FPS is significantly different from reported FPS in AddVideoStream
                // this can confuse the syncer and prevent it from producing synchronized pairs
                software_dev.SetMatcher(Matchers.Default);

                var sync = new Syncer();

                depth_sensor.Open(depth_profile);
                color_sensor.Open(color_profile);

                depth_sensor.Start(f =>
                {
                    sync.SubmitFrame(f);
                    //Debug.WriteLine("D");
                });
                color_sensor.Start(f => {
                    sync.SubmitFrame(f);
                    //Debug.WriteLine("C");
                });

                var token = tokenSource.Token;

                var t = Task.Factory.StartNew(() =>
                {
                    while (!token.IsCancellationRequested)
                    {
                        var frames = pipeline.WaitForFrames();

                        var depth_frame = frames.DepthFrame;
                        var color_frame = frames.ColorFrame;

                        var bytes = new byte[depth_frame.Stride * depth_frame.Height];
                        depth_frame.CopyTo(bytes);
                        depth_sensor.AddVideoFrame(bytes, depth_frame.Stride, 2, depth_frame.Timestamp,
                            depth_frame.TimestampDomain, (int)depth_frame.Number,
                            depth_profile);

                        bytes = new byte[color_frame.Stride * color_frame.Height];
                        color_frame.CopyTo(bytes);
                        color_sensor.AddVideoFrame(bytes, color_frame.Stride, 2, depth_frame.Timestamp,
                            color_frame.TimestampDomain, (int)depth_frame.Number,
                            color_profile);

                        depth_frame.Dispose();
                        color_frame.Dispose();
                        frames.Dispose();

                        var new_frames = sync.WaitForFrames();
                        if (new_frames.Count == 2)
                        {
                            depth_frame = new_frames.DepthFrame;
                            color_frame = new_frames.ColorFrame;

                            var colorized_depth = colorizer.Colorize(depth_frame);

                            UploadImage(imgDepth, colorized_depth);
                            UploadImage(imgColor, color_frame);

                            depth_frame.Dispose();
                            colorized_depth.Dispose();
                            color_frame.Dispose();
                        }
                        new_frames.Dispose();
                    }
                }, token);
            }
            catch (Exception ex)
            {
                MessageBox.Show(ex.Message);
                Application.Current.Shutdown();
            }

            InitializeComponent();
        }

        private void control_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            tokenSource.Cancel();
        }
    }
}
