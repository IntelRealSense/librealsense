using System;
using System.Linq;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

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
        }

        private void button1_Click(object sender, EventArgs e)
        {
            if (device.IsStreaming)
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
                }
                catch (RealSense.Error err)
                {
                    label1.Text = string.Format("Unable to start stream: {0}", err.Message);
                }
            }

            foreach (var stream in Streams) stream.Enabled = !device.IsStreaming;
        }

        private void Reserve(ref byte[] buffer, int size)
        {
            if (buffer == null || buffer.Length < size) buffer = new byte[size];
        }

        private void LoadSource(int index, IntPtr data, int size)
        {
            Reserve(ref src[index], size);
            Marshal.Copy(data, src[index], 0, size);
        }

        private Bitmap GetDest(int index, int width, int height)
        {
            GCHandle handle = GCHandle.Alloc(dst[index], GCHandleType.Pinned);
            var bitmap = new Bitmap(width, height, width * 3, PixelFormat.Format24bppRgb, handle.AddrOfPinnedObject());
            handle.Free();
            return bitmap;
        }

        private Bitmap MakeZ16Bitmap(int index, int width, int height, IntPtr frame)
        {
            LoadSource(index, frame, width * height * 2);
            Reserve(ref dst[index], width * height * 3);

            // Build a cumulative histogram for of depth values in [1,0xFFFF]      
            for (int i = 0; i < depthHistogram.Length; i++) depthHistogram[i] = 0;
            for (int i = 0; i < width * height; i++) depthHistogram[src[index][i * 2] | (src[index][i * 2 + 1] << 8)]++;
            for (int i = 2; i < depthHistogram.Length; i++) depthHistogram[i] += depthHistogram[i - 1];

            // Produce an image in BGR ordered byte array
            for (int i = 0; i < width*height; ++i)
            {
                int d = src[index][i * 2] | (src[index][i * 2 + 1] << 8);
                if (d != 0)
                {
                    uint f = depthHistogram[d] * 255 / depthHistogram[0xFFFF]; // 0-255 based on histogram location
                    dst[index][i * 3 + 0] = (byte)f;
                    dst[index][i * 3 + 1] = 0;
                    dst[index][i * 3 + 2] = (byte)(255 - f);
                }
                else
                {
                    dst[index][i * 3 + 0] = 0;
                    dst[index][i * 3 + 1] = 5;
                    dst[index][i * 3 + 2] = 20;
                }
            }

            return GetDest(index, width, height);
        }

        private Bitmap MakeYUYVBitmap(int index, int width, int height, IntPtr frame)
        {
            LoadSource(index, frame, width * height * 2);
            Reserve(ref dst[index], width * height * 3);
            for (int i = 0; i < width * height; i++)
            {
                dst[index][i * 3 + 0] = src[index][i * 2];
                dst[index][i * 3 + 1] = src[index][i * 2];
                dst[index][i * 3 + 2] = src[index][i * 2];
            }
            return GetDest(index, width, height);
        }

        private Bitmap MakeY8Bitmap(int index, int width, int height, IntPtr frame)
        {
            LoadSource(index, frame, width * height);
            Reserve(ref dst[index], width * height * 3);
            for (int i = 0; i < width * height; i++)
            {
                dst[index][i * 3 + 0] = src[index][i];
                dst[index][i * 3 + 1] = src[index][i];
                dst[index][i * 3 + 2] = src[index][i];
            }
            return GetDest(index, width, height);
        }

        private Bitmap MakeY16Bitmap(int index, int width, int height, IntPtr frame)
        {
            LoadSource(index, frame, width * height * 2);
            Reserve(ref dst[index], width * height * 3);
            for (int i = 0; i < width * height; i++)
            {
                dst[index][i * 3 + 0] = src[index][i * 2 + 1];
                dst[index][i * 3 + 1] = src[index][i * 2 + 1];
                dst[index][i * 3 + 2] = src[index][i * 2 + 1];
            }
            return GetDest(index, width, height);
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            if (device == null) return;
            if (!device.IsStreaming) return;

            timer1.Stop();
            device.WaitForFrames();

            var pictureBoxes = new[] { pictureBox1, pictureBox2, pictureBox3, pictureBox4 };
            foreach(RealSense.Stream stream in Enum.GetValues(typeof(RealSense.Stream)))
            {
                if (device.StreamIsEnabled(stream))
                {
                    var intrin = device.GetStreamIntrinsics(stream);
                    switch (device.GetStreamFormat(stream))
                    {
                    case RealSense.Format.Z16:
                        pictureBoxes[(int)stream].Image = MakeZ16Bitmap((int)stream, intrin.Width, intrin.Height, device.GetFrameData(stream));
                        break;
                    case RealSense.Format.YUYV:
                        pictureBoxes[(int)stream].Image = MakeYUYVBitmap((int)stream, intrin.Width, intrin.Height, device.GetFrameData(stream));
                        break;
                    case RealSense.Format.RGB8:
                    case RealSense.Format.BGR8:
                        pictureBoxes[(int)stream].Image = new Bitmap(intrin.Width, intrin.Height, intrin.Width * 3, PixelFormat.Format24bppRgb, device.GetFrameData(stream));
                        break;
                    case RealSense.Format.RGBA8:
                    case RealSense.Format.BGRA8:
                        pictureBoxes[(int)stream].Image = new Bitmap(intrin.Width, intrin.Height, intrin.Width * 4, PixelFormat.Format32bppRgb, device.GetFrameData(stream));
                        break;
                    case RealSense.Format.Y8:
                        pictureBoxes[(int)stream].Image = MakeY8Bitmap((int)stream, intrin.Width, intrin.Height, device.GetFrameData(stream));
                        break;
                    case RealSense.Format.Y16:
                        pictureBoxes[(int)stream].Image = MakeY16Bitmap((int)stream, intrin.Width, intrin.Height, device.GetFrameData(stream));
                        break;
                    }
                }
                else pictureBoxes[(int)stream].Image = null;
            }

            timer1.Start();
        }

        private StreamConfig[] Streams { get { return new[] { depthConfig, colorConfig, infraredConfig, infrared2Config }; } }

        private RealSense.Device device;
        private byte[][] src = new byte[4][], dst = new byte[4][];
        private uint[] depthHistogram = new uint[0x10000];
    }
}
