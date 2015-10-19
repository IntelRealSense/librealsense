using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace cs_config
{
    public partial class Config : Form
    {
        public Config()
        {
            InitializeComponent();
        }

        public void SetDevice(RealSense.Device d)
        {
            device = d;

            foreach (var stream in Streams) stream.Setup(d);

            Text = string.Format("C# Configuration Example ({0})", device.GetName());
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (device.IsStreaming())
            {
                try
                {
                    device.Stop();
                    label1.Text = string.Format("Stream stopped");
                    button1.Text = "Start Streaming";
                }
                catch (RealSense.Error err)
                {
                    label1.Text = string.Format("Unable to stop stream: {0}", err.Message);
                }
            }
            else
            {
                try
                {
                    foreach (var stream in Streams) stream.Apply(device);
                    device.Start();
                    label1.Text = string.Format("Stream started");
                    button1.Text = "Stop Streaming";

                    for (int i = 0; i < 4; ++i)
                    {
                        var stream = (RealSense.Stream)i;
                        if (device.IsStreamEnabled(stream))
                        {
                            var intrin = device.GetStreamIntrinsics(stream);
                            buffers[i] = new BitmapBuffer(intrin.Width, intrin.Height, device.GetStreamFormat(stream));
                        }
                        else buffers[i] = null;
                    }
                }
                catch (RealSense.Error err)
                {
                    label1.Text = string.Format("Unable to start stream: {0}", err.Message);
                }
            }

            foreach (var stream in Streams) stream.Enabled = !device.IsStreaming();
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            if (device == null) return;
            if (!device.IsStreaming()) return;

            timer1.Stop();
            device.WaitForFrames();

            var pictureBoxes = new[] { pictureBox1, pictureBox2, pictureBox3, pictureBox4 };
            for(int i=0; i<4; ++i)
            {
                if (buffers[i] != null)
                {
                    var data = device.GetFrameData((RealSense.Stream)i);
                    pictureBoxes[i].Image = buffers[i].MakeBitmap(data);
                }
                else pictureBoxes[i].Image = null;
            }

            timer1.Start();
        }

        private StreamConfig[] Streams { get { return new[] { depthConfig, colorConfig, infraredConfig, infrared2Config }; } }

        private RealSense.Device device;
        private BitmapBuffer[] buffers = new BitmapBuffer[4];

        private class BitmapBuffer
        {
            private byte[] src, dst;
            private uint[] histogram;
            private int width, height;
            private RealSense.Format format;

            private int Get16(int i) { return src[i * 2] | (src[i * 2 + 1] << 8); }

            public BitmapBuffer(int width, int height, RealSense.Format format)
            {
                this.width = width;
                this.height = height;
                this.format = format;
                switch (format)
                {
                case RealSense.Format.BGR8:
                case RealSense.Format.BGRA8:
                    // No need for buffers, return early to avoid allocating dst
                    return;
                case RealSense.Format.Y8:
                    src = new byte[width * height * 1];
                    break;
                case RealSense.Format.Z16:
                case RealSense.Format.Y16:
                case RealSense.Format.YUYV:
                    src = new byte[width * height * 2];
                    break;
                case RealSense.Format.RGB8:
                    src = new byte[width * height * 3];
                    break;
                case RealSense.Format.RGBA8:
                    src = new byte[width * height * 4];
                    break;
                }
                dst = new byte[width * height * 3];
                if (format == RealSense.Format.Z16) histogram = new uint[0x10000];
            }

            public Bitmap MakeBitmap(IntPtr data)
            {
                if (format == RealSense.Format.BGR8) return new Bitmap(width, height, width * 3, PixelFormat.Format24bppRgb, data);
                if (format == RealSense.Format.BGRA8) return new Bitmap(width, height, width * 4, PixelFormat.Format32bppRgb, data);

                Marshal.Copy(data, src, 0, src.Length);
                switch (format)
                {
                case RealSense.Format.Z16:
                    for (int i = 0; i < histogram.Length; i++) histogram[i] = 0;
                    for (int i = 0; i < width * height; i++) histogram[Get16(i)]++;
                    for (int i = 2; i < histogram.Length; i++) histogram[i] += histogram[i - 1];
                    for (int i = 0; i < width * height; ++i)
                    {
                        int d = Get16(i);
                        if (d != 0)
                        {
                            uint f = histogram[d] * 255 / histogram[0xFFFF];
                            dst[i * 3 + 0] = (byte)f;
                            dst[i * 3 + 1] = 0;
                            dst[i * 3 + 2] = (byte)(255 - f);
                        }
                        else
                        {
                            dst[i * 3 + 0] = 0;
                            dst[i * 3 + 1] = 5;
                            dst[i * 3 + 2] = 20;
                        }
                    }
                    break;
                case RealSense.Format.YUYV:
                    for (int i = 0; i < width * height; i++)
                    {
                        dst[i * 3 + 0] = src[i * 2];
                        dst[i * 3 + 1] = src[i * 2];
                        dst[i * 3 + 2] = src[i * 2];
                    }
                    break;
                case RealSense.Format.RGB8:
                    for (int i = 0; i < width * height; i++)
                    {
                        dst[i * 3 + 0] = src[i * 3 + 2];
                        dst[i * 3 + 1] = src[i * 3 + 1];
                        dst[i * 3 + 2] = src[i * 3 + 0];
                    }
                    break;
                case RealSense.Format.RGBA8:
                    for (int i = 0; i < width * height; i++)
                    {
                        dst[i * 3 + 0] = src[i * 4 + 2];
                        dst[i * 3 + 1] = src[i * 4 + 1];
                        dst[i * 3 + 2] = src[i * 4 + 0];
                    }
                    break;
                case RealSense.Format.Y8:
                    for (int i = 0; i < width * height; i++)
                    {
                        dst[i * 3 + 0] = src[i];
                        dst[i * 3 + 1] = src[i];
                        dst[i * 3 + 2] = src[i];
                    }
                    break;
                case RealSense.Format.Y16:
                    for (int i = 0; i < width * height; i++)
                    {
                        dst[i * 3 + 0] = src[i * 2 + 1];
                        dst[i * 3 + 1] = src[i * 2 + 1];
                        dst[i * 3 + 2] = src[i * 2 + 1];
                    }
                    break;
                }

                GCHandle handle = GCHandle.Alloc(dst, GCHandleType.Pinned);
                var bitmap = new Bitmap(width, height, width * 3, PixelFormat.Format24bppRgb, handle.AddrOfPinnedObject());
                handle.Free();
                return bitmap;
            }
        }
    }
}
