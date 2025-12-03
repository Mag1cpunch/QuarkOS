#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

#include <system/term.h>

typedef __typeof__( (size_t)0 - 1 ) ssize_t;

static void print_uint(unsigned long long v,
                       unsigned base, int upper,
                       void (*out)(char))
{
    char buf[32];  int len = 0;
    const char *dig = upper ? "0123456789ABCDEF"
                            : "0123456789abcdef";
    if (!v) buf[len++] = '0';
    while (v) { buf[len++] = dig[v % base]; v /= base; }
    while (len) out(buf[--len]);
}

static void print_int(long long v, void (*out)(char))
{
    if (v < 0) { out('-'); v = -v; }
    print_uint((unsigned long long)v, 10, 0, out);
}

enum { NONE, HH, H, L, LL, Z, T, J };

static unsigned long long fetch_u(int mod, va_list ap)
{
    switch (mod) {
        case HH: return (unsigned char)  va_arg(ap, unsigned int);
        case H : return (unsigned short) va_arg(ap, unsigned int);
        case L : return                   va_arg(ap, unsigned long);
        case LL: return                   va_arg(ap, unsigned long long);
        case Z : return                   va_arg(ap, size_t);      /* uintptr_t */
        case T : return (unsigned long long)va_arg(ap, ptrdiff_t); /* intptr_t  */
        case J : return                   va_arg(ap, uintmax_t);
        default: return                   va_arg(ap, unsigned int);
    }
}

static long long fetch_s(int mod, va_list ap)
{
    switch (mod) {
        case HH: return (signed char)      va_arg(ap, int);
        case H : return (short)            va_arg(ap, int);
        case L : return                    va_arg(ap, long);
        case LL: return                    va_arg(ap, long long);
        case Z : return                    va_arg(ap, ssize_t);
        case T : return                    va_arg(ap, ptrdiff_t);
        case J : return                    va_arg(ap, intmax_t);
        default: return                    va_arg(ap, int);
    }
}

int printf(const char *fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    int written = 0;

    for (; *fmt; ++fmt) {
        if (*fmt != '%') { kputchar(*fmt); ++written; continue; }

        // Flags
        int alt = 0, zero = 0, left = 0, plus = 0, space = 0;
        int width = 0, precision = -1;
        for (++fmt; ; ++fmt) {
            if (*fmt == '#') { alt = 1; continue; }
            if (*fmt == '0') { zero = 1; continue; }
            if (*fmt == '-') { left = 1; continue; }
            if (*fmt == '+') { plus = 1; continue; }
            if (*fmt == ' ') { space = 1; continue; }
            break;
        }

        // Width
        if (*fmt >= '0' && *fmt <= '9') {
            width = 0;
            while (*fmt >= '0' && *fmt <= '9')
                width = width * 10 + (*fmt++ - '0');
        }
        // Precision
        if (*fmt == '.') {
            ++fmt;
            precision = 0;
            while (*fmt >= '0' && *fmt <= '9')
                precision = precision * 10 + (*fmt++ - '0');
        }

        // Length modifier
        int mod = NONE;
        if (*fmt == 'h' && fmt[1]=='h') { mod = HH; fmt += 2; }
        else if (*fmt == 'h')           { mod = H;  ++fmt;  }
        else if (*fmt == 'l' && fmt[1]=='l') { mod = LL; fmt += 2; }
        else if (*fmt == 'l')           { mod = L;  ++fmt;  }
        else if (*fmt == 'z')           { mod = Z;  ++fmt;  }
        else if (*fmt == 't')           { mod = T;  ++fmt;  }
        else if (*fmt == 'j')           { mod = J;  ++fmt;  }

        char pad = zero && !left ? '0' : ' ';

        switch (*fmt) {
        case 'c': {
            char c = (char)va_arg(ap,int);
            int padlen = width > 1 ? width - 1 : 0;
            if (!left) while (padlen--) { kputchar(pad); ++written; }
            kputchar(c); ++written;
            if (left) while (padlen--) { kputchar(' '); ++written; }
        } break;

        case 's': {
            const char *s = va_arg(ap,const char*);
            int slen = 0; const char *t = s;
            while (*t && (precision < 0 || slen < precision)) { ++slen; ++t; }
            int padlen = width > slen ? width - slen : 0;
            if (!left) while (padlen--) { kputchar(pad); ++written; }
            for (int i = 0; i < slen; ++i) { kputchar(*s++); ++written; }
            if (left) while (padlen--) { kputchar(' '); ++written; }
        } break;

        case 'd': case 'i': {
            long long v = fetch_s(mod, ap);
            char buf[32]; int len = 0;
            int neg = v < 0;
            unsigned long long uv = neg ? -v : v;
            if (!uv) buf[len++] = '0';
            while (uv) { buf[len++] = '0' + (uv % 10); uv /= 10; }
            if (neg) buf[len++] = '-';
            else if (plus) buf[len++] = '+';
            else if (space) buf[len++] = ' ';
            int padlen = width > len ? width - len : 0;
            if (!left) while (padlen-- > 0) { kputchar(pad); ++written; }
            while (len) { kputchar(buf[--len]); ++written; }
            if (left) while (padlen-- > 0) { kputchar(' '); ++written; }
        } break;

        case 'u': case 'o': case 'x': case 'X': {
            unsigned long long v = fetch_u(mod, ap);
            int base = (*fmt == 'u') ? 10 : (*fmt == 'o') ? 8 : 16;
            char buf[32]; int len = 0;
            const char *dig = (*fmt == 'X') ? "0123456789ABCDEF" : "0123456789abcdef";
            if (!v) buf[len++] = '0';
            while (v) { buf[len++] = dig[v % base]; v /= base; }
            if (alt && *fmt == 'o' && buf[len-1] != '0') buf[len++] = '0';
            if (alt && *fmt == 'x') { buf[len++] = 'x'; buf[len++] = '0'; }
            if (alt && *fmt == 'X') { buf[len++] = 'X'; buf[len++] = '0'; }
            int padlen = width > len ? width - len : 0;
            if (!left) while (padlen-- > 0) { kputchar(pad); ++written; }
            while (len) { kputchar(buf[--len]); ++written; }
            if (left) while (padlen-- > 0) { kputchar(' '); ++written; }
        } break;

        case 'p': {
            uintptr_t v = (uintptr_t)va_arg(ap, void*);
            char buf[2 + sizeof(uintptr_t) * 2];
            int len = 0;
            buf[len++] = '0'; buf[len++] = 'x';
            for (int i = (sizeof(uintptr_t) * 2) - 1; i >= 0; --i)
                buf[len++] = "0123456789abcdef"[(v >> (i * 4)) & 0xf];
            for (int i = 0; i < len; ++i) { kputchar(buf[i]); ++written; }
        } break;

        case 'n': {
            int *ptr = va_arg(ap, int*);
            *ptr = written;
        } break;

        case '%': kputchar('%'); ++written; break;

        default : kputchar('?'); ++written; break;
        }
    }

    va_end(ap);
    return written;
}