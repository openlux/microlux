using MadWizard.WinUSBNet;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace ASCOM.microlux
{
    class TransferQueue
    {
        private readonly USBPipe pipe;
        private readonly int depth;
        private readonly int bufferSize;

        private readonly IAsyncResult[] queue;
        private readonly byte[][] buffers;

        private bool first = true;
        private int index = 0;

        public TransferQueue(USBPipe pipe, int depth, int bufferSize)
        {
            this.pipe = pipe;
            this.depth = depth;
            this.bufferSize = bufferSize;

            this.queue = new IAsyncResult[depth];
            this.buffers = new byte[depth][];
        }

        public Buffer Read()
        {
            if (first)
            {
                for (var i = 0; i < depth; i++)
                {
                    buffers[i] = new byte[bufferSize];
                }
                
                for (var i = 0; i < depth; i++)
                {
                    queue[i] = pipe.BeginRead(buffers[i], 0, bufferSize, null, null);
                }

                first = false;
            }

            var r = queue[index];
            var b = buffers[index];

            var nb = new byte[bufferSize];
            queue[index] = pipe.BeginRead(nb, 0, bufferSize, null, null);
            buffers[index] = nb;

            var length = pipe.EndRead(r);
            index = (index + 1) % depth;

            return new Buffer(b, length);
        }
    }

    public class Buffer
    {
        public byte[] Data { get; set; }
        public int Length { get; set; }

        public Buffer(byte[] data, int length)
        {
            Data = data;
            Length = length;
        }
    }
}
