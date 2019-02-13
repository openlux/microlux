using MadWizard.WinUSBNet;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Windows.Forms;

namespace ASCOM.microlux
{
    class Microlux
    {
        private static readonly Guid MICROLUX_GUID = new Guid("{6D4AA3C7-8867-40BA-97A2-EC1DD808C787}");

        private static readonly byte[] SYNC_MARKER = new byte[] { 0x50, 0x55, 0x0C, 0xA5, 0x00, 0x5A, 0x24, 0x5A, 0x40 };

        private const int FRAME_WIDTH = 1284;
        private const int FRAME_HEIGHT = 968;

        private const int FRAME_SIZE = FRAME_WIDTH * FRAME_HEIGHT * 2;
        private const int FRAME_BUFFER_SIZE = FRAME_SIZE * 4;

        private const int QUEUE_DEPTH = 256;
        private const int BUFFER_SIZE = 65536;

        private readonly object _lock = new object();

        private readonly string serialNumber;

        private readonly BlockingCollection<Buffer> bufferQueue    = new BlockingCollection<Buffer>(new ConcurrentQueue<Buffer>(), QUEUE_DEPTH);
        private readonly BlockingCollection<byte[]> frameDataQueue = new BlockingCollection<byte[]>(new ConcurrentQueue<byte[]>(), 4);
        private readonly BlockingCollection<int[,]> frameQueue     = new BlockingCollection<int[,]>(new ConcurrentQueue<int[,]>(), 1);

        private USBDevice device;
        private TransferQueue transferQueue;
        private bool abort = false;

        public Microlux(string serialNumber)
        {
            this.serialNumber = serialNumber;

            Init();
        }

        public void Init()
        {
            new Thread(DecodeThread).Start();
            new Thread(BufferThread).Start();
        }

        public void Connect()
        {
            device = Open(serialNumber);

            if (device == null)
            {
                throw new ASCOM.NotConnectedException("Failed to open USB device");
            }

            device.ControlPipeTimeout = 0;

            var fifoInterface = device.Interfaces[2];
            var fifoPipe = fifoInterface.InPipe;

            fifoPipe.Policy.RawIO = true;

            lock (_lock)
            {
                abort = false;
                transferQueue = new TransferQueue(fifoPipe, QUEUE_DEPTH, BUFFER_SIZE);
            }
            
            new Thread(TransferThread).Start();
        }

        public void WriteExposureMessage(int startX, int endX, int startY, int endY, int gain, int offset, int exposureCoarse, int exposureFine, int lineLength)
        {
            byte[] data = new byte[16];
            var wrap = new BinaryWriter(new MemoryStream(data));

            startX = 0;
            startY = 2;
            endX = 1283;
            endY = 965;

            wrap.Write((short)startX);
            wrap.Write((short)endX);
            wrap.Write((short)startY);
            wrap.Write((short)endY);

            wrap.Write((byte)gain);
            wrap.Write((byte)offset);

            wrap.Write((short)exposureCoarse);
            wrap.Write((short)exposureFine);
            wrap.Write((short)lineLength);

            device.ControlOut(0x41, 0x80, 0, 2, data);
        }

        public int[,] ReadFrame()
        {
            return frameQueue.Take();
        }

        public void DecodeThread()
        {
            while (true)
            {
                byte[] data = frameDataQueue.Take();

                var frame = new int[FRAME_WIDTH, FRAME_HEIGHT];

                for (var y = 0; y < FRAME_HEIGHT; y++)
                {
                    var lineStart = y * FRAME_WIDTH * 2;

                    for (var x = 0; x < FRAME_WIDTH; x++)
                    {
                        var pb = data[(lineStart + (x << 1))] & 0xFF;
                        var pd = data[(lineStart + (x << 1)) | 1] & 0x0F;
                        var p = (R8(pb) << 4) | ((R8(pd) >> 4) & 0x0F);

                        frame[x, y] = (R8(pb) << 8) | (R8(pd) & 0xF0);
                    }
                }

                frameQueue.TryAdd(frame);
            }
        }

        public void BufferThread()
        {
            var ringBuffer = new RingBuffer(FRAME_BUFFER_SIZE);
            var syncing = true;

            while (true)
            {
                try
                {
                    var buffer = bufferQueue.Take();
                    ringBuffer.Write(buffer.Data, 0, buffer.Length);

                    if (syncing)
                    {
                        if (ringBuffer.Sync(SYNC_MARKER))
                        {
                            syncing = false;
                        }
                    }
                    else
                    {
                        if (ringBuffer.Available() >= FRAME_SIZE)
                        {
                            byte[] frameData = ringBuffer.Read(FRAME_SIZE);
                            frameDataQueue.TryAdd(frameData);

                            syncing = true;
                        }
                    }
                } catch (Exception) { }
            }
        }

        public void TransferThread()
        {
            Thread.CurrentThread.Priority = ThreadPriority.Highest;

            try
            {
                while (true)
                {
                    lock (_lock)
                    {
                        if (abort)
                        {
                            return;
                        }
                    }

                    bufferQueue.TryAdd(transferQueue.Read());
                }
            } catch (Exception) { }
        }

        public void Disconnect()
        {
            lock (_lock)
            {
                abort = true;
            }

            device.Dispose();
        }

        public static int R8(int i)
        {
            return table[i];
        }

        public static readonly int[] table = {
            0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
            0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
            0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
            0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
            0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
            0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
            0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
            0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
            0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
            0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
            0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
            0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
            0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
            0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
            0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
            0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
            0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
            0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
            0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
            0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
            0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
            0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
            0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
            0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
            0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
            0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
            0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
            0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
            0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
            0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
            0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
            0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff,
        };

        public static USBDevice Open(string serialNumber)
        {
            var list = List();

            if (list.Count == 0) return null;

            if (string.IsNullOrEmpty(serialNumber))
            {
                return new USBDevice(list.First().Info);
            }

            foreach (var device in list)
            {
                if (device.SerialNumber.Equals(serialNumber))
                {
                    return new USBDevice(device.Info);
                }
            }

            return null;
        }

        public static List<MicroluxDevice> List()
        {
            var list = new List<MicroluxDevice>();

            var devices = USBDevice.GetDevices(MICROLUX_GUID);

            var regex = new Regex(@"usb#vid_[a-f0-9]{4}&pid_[a-f0-9]{4}#([a-f0-9]{8})#", RegexOptions.IgnoreCase);

            foreach (var device in devices)
            {
                var match = regex.Match(device.DevicePath);

                if (match.Success)
                {
                    list.Add(new MicroluxDevice(match.Groups[1].Value.ToUpper(), device));
                }
                else
                {
                    list.Add(new MicroluxDevice(string.Empty, device));
                }
            }

            return list;
        }
    }

    class MicroluxDevice
    {
        public string SerialNumber {
            get;
        }

        public USBDeviceInfo Info
        {
            get;
        }

        public MicroluxDevice(string name, USBDeviceInfo info)
        {
            SerialNumber = name;
            Info = info;
        }

        public override string ToString()
        {
            return string.IsNullOrEmpty(SerialNumber) ? "microlux" : ("microlux [" + SerialNumber + "]");
        }
    }
}
