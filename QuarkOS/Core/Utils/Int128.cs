using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace QuarkOS.Core.Utils
{
    public struct UInt128 : IComparable, IFormattable, IConvertible
    {
        private ulong high;
        private ulong low;

        public UInt128(ulong high, ulong low)
        {
            this.high = high;
            this.low = low;
        }

        public static implicit operator UInt128(ulong value)
        {
            return new UInt128(0, value);
        }

        public static implicit operator UInt128(ulong[] values)
        {
            if (values.Length != 2)
            {
                throw new ArgumentException("Array must contain exactly two elements.");
            }
            return new UInt128(values[0], values[1]);
        }

        public static implicit operator ulong(UInt128 value)
        {
            return value.low;
        }

        public override string ToString()
        {
            return $"{high:X16}{low:X16}";
        }

        public string ToString(string format, IFormatProvider formatProvider)
        {
            return ToString();
        }

        public int CompareTo(UInt128 other)
        {
            if (this > other) return 1;
            if (this < other) return -1;
            return 0;
        }

        int IComparable.CompareTo(object obj)
        {
            if (obj is UInt128 other)
            {
                return CompareTo(other);
            }
            throw new ArgumentException("Object is not a UInt128");
        }

        public bool Equals(UInt128 other)
        {
            return this == other;
        }

        public override bool Equals(object obj)
        {
            if (obj is UInt128 other)
            {
                return Equals(other);
            }
            return false;
        }

        public override int GetHashCode()
        {
            return HashCode.Combine(high, low);
        }

        public static UInt128 operator +(UInt128 a, UInt128 b)
        {
            ulong low = a.low + b.low;
            ulong carry = (low < a.low) ? 1UL : 0UL;
            ulong high = a.high + b.high + carry;
            return new UInt128(high, low);
        }

        public static UInt128 operator -(UInt128 a, UInt128 b)
        {
            ulong borrow = (a.low < b.low) ? 1UL : 0UL;
            ulong low = a.low - b.low;
            ulong high = a.high - b.high - borrow;
            return new UInt128(high, low);
        }

        public static UInt128 operator *(UInt128 a, UInt128 b)
        {
            ulong aLow = a.low & 0xFFFFFFFF;
            ulong aHigh = a.low >> 32;
            ulong bLow = b.low & 0xFFFFFFFF;
            ulong bHigh = b.low >> 32;

            ulong lowLow = aLow * bLow;
            ulong lowHigh = aLow * bHigh;
            ulong highLow = aHigh * bLow;
            ulong highHigh = aHigh * bHigh;

            ulong mid1 = lowHigh + (lowLow >> 32);
            ulong mid2 = highLow + (mid1 & 0xFFFFFFFF);
            ulong high = highHigh + (mid1 >> 32) + (mid2 >> 32);
            ulong low = (mid2 << 32) | (lowLow & 0xFFFFFFFF);

            return new UInt128(high, low);
        }

        public static UInt128 operator /(UInt128 dividend, UInt128 divisor)
        {
            if (divisor == Zero)
                throw new DivideByZeroException();

            UInt128 quotient = Zero;
            UInt128 remainder = Zero;

            for (int i = 127; i >= 0; i--)
            {
                remainder <<= 1;
                remainder.low |= (dividend.high >> i) & 1UL;

                if (remainder >= divisor)
                {
                    remainder -= divisor;
                    quotient.low |= (1UL << i);
                }
            }

            return quotient;
        }

        public static UInt128 operator <<(UInt128 value, int shift)
        {
            if (shift >= 64)
            {
                return new UInt128(value.low << (shift - 64), 0);
            }
            else if (shift == 0)
            {
                return value;
            }
            else
            {
                return new UInt128((value.high << shift) | (value.low >> (64 - shift)), value.low << shift);
            }
        }

        public static UInt128 operator >>(UInt128 value, int shift)
        {
            if (shift >= 64)
            {
                return new UInt128(0, value.high >> (shift - 64));
            }
            else if (shift == 0)
            {
                return value;
            }
            else
            {
                return new UInt128(value.high >> shift, (value.low >> shift) | (value.high << (64 - shift)));
            }
        }

        public static bool operator <(UInt128 a, UInt128 b)
        {
            if (a.high < b.high) return true;
            if (a.high > b.high) return false;
            return a.low < b.low;
        }

        public static bool operator >(UInt128 a, UInt128 b)
        {
            if (a.high > b.high) return true;
            if (a.high < b.high) return false;
            return a.low > b.low;
        }

        public static bool operator <=(UInt128 a, UInt128 b)
        {
            return a < b || a == b;
        }

        public static bool operator >=(UInt128 a, UInt128 b)
        {
            return a > b || a == b;
        }

        public static bool operator ==(UInt128 a, UInt128 b)
        {
            return a.high == b.high && a.low == b.low;
        }

        public static bool operator !=(UInt128 a, UInt128 b)
        {
            return !(a == b);
        }

        public static UInt128 Zero => new UInt128(0, 0);

        // IConvertible Implementation
        public TypeCode GetTypeCode()
        {
            return TypeCode.Object;
        }

        public bool ToBoolean(IFormatProvider provider)
        {
            return !(this == Zero);
        }

        public byte ToByte(IFormatProvider provider)
        {
            return (byte)low;
        }

        public char ToChar(IFormatProvider provider)
        {
            return (char)low;
        }

        public DateTime ToDateTime(IFormatProvider provider)
        {
            throw new InvalidCastException("UInt128 cannot be converted to DateTime.");
        }

        public decimal ToDecimal(IFormatProvider provider)
        {
            return high * (decimal)ulong.MaxValue + low;
        }

        public double ToDouble(IFormatProvider provider)
        {
            return (double)high * ulong.MaxValue + low;
        }

        public short ToInt16(IFormatProvider provider)
        {
            return (short)low;
        }

        public int ToInt32(IFormatProvider provider)
        {
            return (int)low;
        }

        public long ToInt64(IFormatProvider provider)
        {
            return (long)low;
        }

        public sbyte ToSByte(IFormatProvider provider)
        {
            return (sbyte)low;
        }

        public float ToSingle(IFormatProvider provider)
        {
            return (float)((double)high * ulong.MaxValue + low);
        }

        public string ToString(IFormatProvider provider)
        {
            return ToString();
        }

        public object ToType(Type conversionType, IFormatProvider provider)
        {
            if (conversionType == typeof(UInt128))
                return this;
            throw new InvalidCastException($"UInt128 cannot be converted to {conversionType.Name}.");
        }

        public ushort ToUInt16(IFormatProvider provider)
        {
            return (ushort)low;
        }

        public uint ToUInt32(IFormatProvider provider)
        {
            return (uint)low;
        }

        public ulong ToUInt64(IFormatProvider provider)
        {
            return low;
        }
    }

    public struct Int128 : IComparable<Int128>, IComparable, IEquatable<Int128>, IFormattable, IConvertible
    {
        private ulong low;
        private long high;

        public Int128(long high, ulong low)
        {
            this.high = high;
            this.low = low;
        }

        public static implicit operator Int128(long value)
        {
            return new Int128(value < 0 ? -1L : 0, (ulong)value);
        }

        public static explicit operator long(Int128 value)
        {
            return (long)value.low;
        }

        public override string ToString()
        {
            return $"{high:X16}{low:X16}";
        }

        public string ToString(string format, IFormatProvider formatProvider)
        {
            return ToString();
        }

        public int CompareTo(Int128 other)
        {
            if (this > other) return 1;
            if (this < other) return -1;
            return 0;
        }

        int IComparable.CompareTo(object obj)
        {
            if (obj is Int128 other)
            {
                return CompareTo(other);
            }
            throw new ArgumentException("Object is not an Int128");
        }

        public bool Equals(Int128 other)
        {
            return this == other;
        }

        public override bool Equals(object obj)
        {
            if (obj is Int128 other)
            {
                return Equals(other);
            }
            return false;
        }

        public override int GetHashCode()
        {
            return HashCode.Combine(high, low);
        }

        public static Int128 operator +(Int128 a, Int128 b)
        {
            ulong low = a.low + b.low;
            long carry = (long)(low < a.low ? 1UL : 0UL);
            long high = a.high + b.high + carry;
            return new Int128(high, low);
        }

        public static Int128 operator -(Int128 a, Int128 b)
        {
            ulong low = a.low - b.low;
            long borrow = (long)(a.low < b.low ? 1UL : 0UL);
            long high = a.high - b.high - borrow;
            return new Int128(high, low);
        }

        public static Int128 operator *(Int128 a, Int128 b)
        {
            ulong aLow = a.low & 0xFFFFFFFF;
            ulong aHigh = a.low >> 32;
            ulong bLow = b.low & 0xFFFFFFFF;
            ulong bHigh = b.low >> 32;

            ulong lowLow = aLow * bLow;
            ulong lowHigh = aLow * bHigh;
            ulong highLow = aHigh * bLow;
            ulong highHigh = aHigh * bHigh;

            ulong mid1 = lowHigh + (lowLow >> 32);
            ulong mid2 = highLow + (mid1 & 0xFFFFFFFF);
            long high = (long)(highHigh + (mid1 >> 32) + (mid2 >> 32));
            ulong low = (mid2 << 32) | (lowLow & 0xFFFFFFFF);

            high += a.high * (long)b.low + b.high * (long)a.low;
            return new Int128(high, low);
        }

        public static Int128 operator /(Int128 dividend, Int128 divisor)
        {
            if (divisor == Zero)
                throw new DivideByZeroException();

            bool negate = false;
            if (dividend < Zero)
            {
                dividend = -dividend;
                negate = !negate;
            }
            if (divisor < Zero)
            {
                divisor = -divisor;
                negate = !negate;
            }

            Int128 quotient = Zero;
            Int128 remainder = Zero;

            for (int i = 127; i >= 0; i--)
            {
                remainder <<= 1;
                remainder.low |= (ulong)((dividend.high >> i) & 1);

                if (remainder >= divisor)
                {
                    remainder -= divisor;
                    quotient.low |= (1UL << i);
                }
            }

            return negate ? -quotient : quotient;
        }

        public static Int128 operator <<(Int128 value, int shift)
        {
            if (shift >= 64)
            {
                return new Int128((long)(value.low << (shift - 64)), 0);
            }
            else if (shift == 0)
            {
                return value;
            }
            else
            {
                return new Int128((value.high << shift) | (long)(value.low >> (64 - shift)), value.low << shift);
            }
        }

        public static Int128 operator >>(Int128 value, int shift)
        {
            if (shift >= 64)
            {
                return new Int128(value.high >> (shift - 64), (ulong)(value.high >> 63));
            }
            else if (shift == 0)
            {
                return value;
            }
            else
            {
                ulong low = (value.low >> shift) | (ulong)(value.high << (64 - shift));
                long high = value.high >> shift;
                return new Int128(high, low);
            }
        }

        public static Int128 operator &(Int128 a, Int128 b)
        {
            return new Int128(a.high & b.high, a.low & b.low);
        }

        public static Int128 operator |(Int128 a, Int128 b)
        {
            return new Int128(a.high | b.high, a.low | b.low);
        }

        public static Int128 operator ^(Int128 a, Int128 b)
        {
            return new Int128(a.high ^ b.high, a.low ^ b.low);
        }

        public static Int128 operator ~(Int128 value)
        {
            return new Int128(~value.high, ~value.low);
        }

        public static bool operator <(Int128 a, Int128 b)
        {
            if (a.high < b.high) return true;
            if (a.high > b.high) return false;
            return a.low < b.low;
        }

        public static bool operator >(Int128 a, Int128 b)
        {
            if (a.high > b.high) return true;
            if (a.high < b.high) return false;
            return a.low > b.low;
        }

        public static bool operator <=(Int128 a, Int128 b)
        {
            return a < b || a == b;
        }

        public static bool operator >=(Int128 a, Int128 b)
        {
            return a > b || a == b;
        }

        public static bool operator ==(Int128 a, Int128 b)
        {
            return a.high == b.high && a.low == b.low;
        }

        public static bool operator !=(Int128 a, Int128 b)
        {
            return !(a == b);
        }

        public static Int128 operator -(Int128 value)
        {
            if (value == Zero)
                return value;
            value = ~value + 1;
            return value;
        }

        public static Int128 Zero => new Int128(0, 0);

        // IConvertible Implementation
        public TypeCode GetTypeCode()
        {
            return TypeCode.Object;
        }

        public bool ToBoolean(IFormatProvider provider)
        {
            return !(this == Zero);
        }

        public byte ToByte(IFormatProvider provider)
        {
            return (byte)low;
        }

        public char ToChar(IFormatProvider provider)
        {
            return (char)low;
        }

        public DateTime ToDateTime(IFormatProvider provider)
        {
            throw new InvalidCastException("Int128 cannot be converted to DateTime.");
        }

        public decimal ToDecimal(IFormatProvider provider)
        {
            return high * (decimal)ulong.MaxValue + low;
        }

        public double ToDouble(IFormatProvider provider)
        {
            return (double)high * ulong.MaxValue + low;
        }

        public short ToInt16(IFormatProvider provider)
        {
            return (short)low;
        }

        public int ToInt32(IFormatProvider provider)
        {
            return (int)low;
        }

        public long ToInt64(IFormatProvider provider)
        {
            return (long)low;
        }

        public sbyte ToSByte(IFormatProvider provider)
        {
            return (sbyte)low;
        }

        public float ToSingle(IFormatProvider provider)
        {
            return (float)((double)high * ulong.MaxValue + low);
        }

        public string ToString(IFormatProvider provider)
        {
            return ToString();
        }

        public object ToType(Type conversionType, IFormatProvider provider)
        {
            if (conversionType == typeof(Int128))
                return this;
            throw new InvalidCastException($"Int128 cannot be converted to {conversionType.Name}.");
        }

        public ushort ToUInt16(IFormatProvider provider)
        {
            return (ushort)low;
        }

        public uint ToUInt32(IFormatProvider provider)
        {
            return (uint)low;
        }

        public ulong ToUInt64(IFormatProvider provider)
        {
            return low;
        }
    }
}
