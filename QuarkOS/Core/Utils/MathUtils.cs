using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace QuarkOS.Core.Utils
{
    public static class MathUtils
    {
        public static ulong MaxValueByBitCount(int bitCount)
        {
            if (bitCount < 0 || bitCount > 64)
            {
                throw new ArgumentOutOfRangeException(nameof(bitCount), "Bit count must be between 0 and 64.");
            }

            return bitCount == 0 ? 0 : (1UL << bitCount) - 1;
        }
    }
}
