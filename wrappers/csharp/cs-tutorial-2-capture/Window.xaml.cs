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
                var rect = new Int32Rect(0, 0, frame.Width, frame.Height);
                wbmp.WritePixels(rect, frame.Data, frame.Stride * frame.Height, frame.Stride);
            });
        }

        public CaptureWindow()
        {
            InitializeComponent();

            const int f450_width = 1920;
            const int f450_height = 1088;
            try
            {
                Action<VideoFrame> updateInfraredFirst;
                Action<VideoFrame> updateInfraredSecond;

                // The colorizer processing block will be used to visualize the irFirst frames.
                colorizer = new Colorizer();

                // Create and config the pipeline to strem irSecond and irFirst frames.
                pipeline = new Pipeline();

                var cfg = new Config();
                cfg.EnableStream(Stream.Infrared, 1, f450_width, f450_height, Format.Yuyv, 5);
                cfg.EnableStream(Stream.Infrared, 2, f450_width, f450_height, Format.Yuyv, 5);

                var pp = pipeline.Start(cfg);

                SetupWindow(pp, out updateInfraredFirst, out updateInfraredSecond);

                Task.Factory.StartNew(() =>
                {
                    while (!tokenSource.Token.IsCancellationRequested)
                    {
                        // We wait for the next available FrameSet and using it as a releaser object that would track
                        // all newly allocated .NET frames, and ensure deterministic finalization
                        // at the end of scope. 
                        using (var frames = pipeline.WaitForFrames())
                        {
                            var irFirstFrame = frames.InfraredFrame.DisposeWith(frames);
                            var irSecondFrame = frames.InfraredFrame.DisposeWith(frames);

                            // We colorize the ir frames for visualization purposes
                            //var colorizedIrFirstFrame = colorizer.Process<VideoFrame>(irFirstFrame).DisposeWith(frames);
                            //var colorizedIrSecondFrame = colorizer.Process<VideoFrame>(irSecondFrame).DisposeWith(frames);

                            // Render the frames.
                            //Dispatcher.Invoke(DispatcherPriority.Render, updateInfraredFirst, colorizedDepth);
                            Dispatcher.Invoke(DispatcherPriority.Render, updateInfraredFirst, irFirstFrame);
                            Dispatcher.Invoke(DispatcherPriority.Render, updateInfraredSecond, irSecondFrame);
                            //Dispatcher.Invoke(DispatcherPriority.Render, updateInfraredFirst, colorizedIrFirstFrame);
                            //Dispatcher.Invoke(DispatcherPriority.Render, updateInfraredSecond, colorizedIrSecondFrame);

                            /*Dispatcher.Invoke(new Action(() =>
                            {
                                String irFirst_dev_sn = irFirstFrame.Sensor.Info[CameraInfo.SerialNumber];
                                txtTimeStamp.Text = irFirst_dev_sn + " : " + String.Format("{0,-20:0.00}", 
                                    irFirstFrame.Timestamp) + "(" + irFirstFrame.TimestampDomain.ToString() + ")";
                            }));
                            Dispatcher.Invoke(new Action(() =>
                            {
                                String irSecond_dev_sn = irSecondFrame.Sensor.Info[CameraInfo.SerialNumber];
                                txtTimeStamp.Text = irSecond_dev_sn + " : " + String.Format("{0,-20:0.00}",
                                    irSecondFrame.Timestamp) + "(" + irSecondFrame.TimestampDomain.ToString() + ")";
                            }));*/
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

        private void SetupWindow(PipelineProfile pipelineProfile, out Action<VideoFrame> irFirst, out Action<VideoFrame> irSecond)
        {
            using (var p = pipelineProfile.GetStream(Stream.Infrared).As<VideoStreamProfile>())
                imgIrFirst.Source = new WriteableBitmap(p.Width, p.Height, 96d, 96d, PixelFormats.Rgb24, null);
            irFirst = UpdateImage(imgIrFirst);

            using (var p = pipelineProfile.GetStream(Stream.Infrared).As<VideoStreamProfile>())
                imgIrSecond.Source = new WriteableBitmap(p.Width, p.Height, 96d, 96d, PixelFormats.Rgb24, null);
            irSecond = UpdateImage(imgIrSecond);
        }
    }
}
