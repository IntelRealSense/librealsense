﻿using System;
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
        private Align align = new Align(Stream.Color);
        private CustomProcessingBlock alignBlock;
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
                pipeline = new Pipeline();
                colorizer = new Colorizer();

                var depthWidth = 640;
                var depthHeight = 480;
                var depthFrames = 30;
                var depthFormat = Format.Z16;

                var colorWidth = 640;
                var colorHeight = 480;
                var colorFrames = 30;

                // To find appropriate depth and color profiles
                FindAppropriateProfiles(
                                        ref depthWidth,
                                        ref depthHeight,
                                        ref depthFrames,
                                        ref depthFormat,

                                        ref colorWidth,
                                        ref colorHeight,
                                        ref colorFrames
                                        );

                var cfg = new Config();
                cfg.EnableStream(Stream.Depth, depthWidth, depthHeight, depthFormat, depthFrames);
                cfg.EnableStream(Stream.Color, colorWidth, colorHeight, Format.Rgb8, colorFrames);

                var profile = pipeline.Start(cfg);

                // Setup the SW device and sensors
                var software_dev = new SoftwareDevice();
                var depth_sensor = software_dev.AddSensor("Depth");
                var depth_profile = depth_sensor.AddVideoStream(new SoftwareVideoStream
                {
                    type = Stream.Depth,
                    index = 0,
                    uid = 100,
                    width = depthWidth,
                    height = depthHeight,
                    fps = depthFrames,
                    bpp = 2,
                    format = depthFormat,
                    intrinsics = profile.GetStream(Stream.Depth).As<VideoStreamProfile>().GetIntrinsics()
                });
                depth_sensor.AddReadOnlyOption(Option.DepthUnits, 1.0f / 5000);

                var color_sensor = software_dev.AddSensor("Color");
                var color_profile = color_sensor.AddVideoStream(new SoftwareVideoStream
                {
                    type = Stream.Color,
                    index = 0,
                    uid = 101,
                    width = colorWidth,
                    height = colorHeight,
                    fps = colorFrames,
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

                // Init custom (align) processing block
                InitAlignProcessingBlock(profile);

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
                                // Invoke custom (align) processing block
                                alignBlock.Process(new_frames);
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

        /// <summary>
        /// To find appropriate depth and color profiles
        /// </summary>
        private void FindAppropriateProfiles(ref int depthWidth, ref int depthHeight, ref int depthFrames, ref Format depthFormat, ref int colorWidth, ref int colorHeight, ref int colorFrames)
        {
            using (var ctx = new Context())
            {
                var devices = ctx.QueryDevices();
                var dev = devices[0];

                Console.WriteLine("\nUsing device 0, an {0}", dev.Info[CameraInfo.Name]);
                Console.WriteLine("    Serial number: {0}", dev.Info[CameraInfo.SerialNumber]);
                Console.WriteLine("    Firmware version: {0}", dev.Info[CameraInfo.FirmwareVersion]);

                var sensors = dev.QuerySensors();
                var depthSensor = sensors[0];
                var colorSensor = sensors[1];

                var depthProfiles = depthSensor.StreamProfiles
                                    .Where(p => p.Stream == Stream.Depth)
                                    .OrderBy(p => p.Framerate)
                                    .Select(p => p.As<VideoStreamProfile>());
                VideoStreamProfile colorProfile = null;

                // select color profile to have frameset equal or closer to depth frameset to syncer work smooth
                foreach (var depthProfile in depthProfiles)
                {
                    depthWidth = depthProfile.Width;
                    depthHeight = depthProfile.Height;
                    var df = depthFrames = depthProfile.Framerate;
                    depthFormat = depthProfile.Format;
                    colorProfile = colorSensor.StreamProfiles
                                        .Where(p => p.Stream == Stream.Color)
                                        .OrderByDescending(p => p.Framerate)
                                        .Select(p => p.As<VideoStreamProfile>())
                                        .FirstOrDefault(p => p.Framerate == df);
                    if (colorProfile != null)
                    {
                        colorWidth = colorProfile.Width;
                        colorHeight = colorProfile.Height;
                        colorFrames = colorProfile.Framerate;
                        break;
                    }
                }
                if (colorProfile == null)
                {
                    // if no profile with the same framerate found, takes the first
                    colorProfile = colorSensor.StreamProfiles
                                    .Where(p => p.Stream == Stream.Color)
                                    .OrderByDescending(p => p.Framerate)
                                    .Select(p => p.As<VideoStreamProfile>()).FirstOrDefault();
                    if (colorProfile == null)
                    {
                        throw new InvalidOperationException($"Error while finding appropriate depth and color profiles");
                    }
                    colorWidth = colorProfile.Width;
                    colorHeight = colorProfile.Height;
                    colorFrames = colorProfile.Framerate;

                }

            }
        }

        /// <summary>
        /// Initialize align processing block
        /// </summary>
        private void InitAlignProcessingBlock(PipelineProfile pipelineProfile)
        {
            // Get the recommended processing blocks for the depth sensor
            var sensor = pipelineProfile.Device.QuerySensors().First(s => s.Is(Extension.DepthSensor));
            var blocks = sensor.ProcessingBlocks.ToList();

            // Get color and depth updaters 
            Action<VideoFrame> updateDepth, updateColor;
            SetupWindow(pipelineProfile, out updateDepth, out updateColor);

            // Create custom processing block
            // For demonstration purposes it will:
            // a. Get a frameset
            // b. Run post-processing on the depth frame
            // c. Combine the result back into a frameset
            // Processing blocks are inherently thread-safe and play well with
            // other API primitives such as frame-queues, 
            // and can be used to encapsulate advanced operations.
            // All invocations are, however, synchronious so the high-level threading model
            // is up to the developer
            alignBlock = new CustomProcessingBlock((f, src) =>
            {
                // We create a FrameReleaser object that would track
                // all newly allocated .NET frames, and ensure deterministic finalization
                // at the end of scope. 
                using (var releaser = new FramesReleaser())
                {
                    foreach (ProcessingBlock p in blocks)
                        f = p.Process(f).DisposeWith(releaser);

                    f = f.ApplyFilter(align).DisposeWith(releaser);
                    f = f.ApplyFilter(colorizer).DisposeWith(releaser);

                    var frames = f.As<FrameSet>().DisposeWith(releaser);

                    var colorFrame = frames[Stream.Color, Format.Rgb8].DisposeWith(releaser);
                    var colorizedDepth = frames[Stream.Depth, Format.Rgb8].DisposeWith(releaser);

                    // Combine the frames into a single result
                    var res = src.AllocateCompositeFrame(colorizedDepth, colorFrame).DisposeWith(releaser);
                    // Send it to the next processing stage
                    src.FrameReady(res);
                }
            });

            // Register to results of processing via a callback:
            alignBlock.Start(f =>
            {
                using (var frames = f.As<FrameSet>())
                {
                    var colorFrame = frames.ColorFrame.DisposeWith(frames);
                    var colorizedDepth = frames.First<VideoFrame>(Stream.Depth, Format.Rgb8).DisposeWith(frames);

                    Dispatcher.Invoke(DispatcherPriority.Render, updateDepth, colorizedDepth);
                    Dispatcher.Invoke(DispatcherPriority.Render, updateColor, colorFrame);
                }
            });
        }

        private void control_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            tokenSource.Cancel();
        }

        private void SetupWindow(PipelineProfile pipelineProfile, out Action<VideoFrame> depth, out Action<VideoFrame> color)
        {
            using (var p = pipelineProfile.GetStream(Stream.Color).As<VideoStreamProfile>())
            {
                imgDepth.Source = new WriteableBitmap(p.Width, p.Height, 96d, 96d, PixelFormats.Rgb24, null);
                imgColor.Source = new WriteableBitmap(p.Width, p.Height, 96d, 96d, PixelFormats.Rgb24, null);
            }
            depth = UpdateImage(imgDepth);
            color = UpdateImage(imgColor);
        }
    }
}
