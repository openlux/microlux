using System;

namespace ASCOM.microlux
{
    public class RingBuffer
    {
        private readonly byte[] buffer;
        private int writeIndex = 0, readIndex = 0, size = 0;

        public RingBuffer(int capacity)
        {
            buffer = new byte[capacity];
        }

        public int GetReadIndex()
        {
            return readIndex;
        }

        public bool Sync(byte[] marker)
        {
            var toCheck = size - (marker.Length * 2);

            for (var i = 0; i < toCheck; i++)
            {
                var start = readIndex + i;

                for (var j = 0; j < marker.Length; j++)
                {
                    if (buffer[(start + (j << 1)) % buffer.Length] != marker[j])
                    {
                        goto outer;
                    }
                }

                Consume(i);
                return true;

            outer:
                {
                    // do nothing, this is used as a marker to break inner loop then continue outer loop
                }
            }

            return false;
        }

        public void Consume(int length)
        {
            if (size >= length)
            {
                readIndex = (readIndex + length) % buffer.Length;
                size -= length;
            }
        }

        public void Write(byte[] data, int offset, int length)
        {
            if (writeIndex + length > buffer.Length)
            {
                var toWrite = buffer.Length - writeIndex;

                Write(data, 0, toWrite);
                Write(data, toWrite, length - toWrite);

                return;
            }

            if (size + length > buffer.Length)
            {
                Consume(length);
            }

            Array.Copy(data, offset, buffer, writeIndex, length);

            writeIndex = (writeIndex + length) % buffer.Length;
            size += length;
        }

        public byte[] Read(int length)
        {
            return Read(length, true);
        }

        public byte[] Read(int length, bool consume)
        {
            var data = new byte[length];

            if (readIndex + length > buffer.Length)
            {
                var toRead = buffer.Length - readIndex;

                Array.Copy(buffer, readIndex, data, 0, toRead);
                Array.Copy(buffer, 0, data, toRead, length - toRead);
            }
            else
            {
                Array.Copy(buffer, readIndex, data, 0, length);
            }

            if (consume) Consume(length);

            return data;
        }

        public int Available()
        {
            return size;
        }
    }
}
