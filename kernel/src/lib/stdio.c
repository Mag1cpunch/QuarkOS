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

        int alt = 0;
        for (++fmt; ; ++fmt) {
            if (*fmt == '#') { alt = 1; continue; }
            if (*fmt == '0' || *fmt == '-' ||
                *fmt == '+' || *fmt == ' ')  continue;
            break;
        }

        int mod = NONE;
        if (*fmt == 'h' && fmt[1]=='h') { mod = HH; fmt += 2; }
        else if (*fmt == 'h')           { mod = H;  ++fmt;  }
        else if (*fmt == 'l' && fmt[1]=='l') { mod = LL; fmt += 2; }
        else if (*fmt == 'l')           { mod = L;  ++fmt;  }
        else if (*fmt == 'z')           { mod = Z;  ++fmt;  }
        else if (*fmt == 't')           { mod = T;  ++fmt;  }
        else if (*fmt == 'j')           { mod = J;  ++fmt;  }

        switch (*fmt) {
        case 'c':
            kputchar((char)va_arg(ap,int));
            ++written;
            break;

        case 's': {
            const char *s = va_arg(ap,const char*);
            while (*s) { kputchar(*s++); ++written; }
        } break;

        case 'd': case 'i':
            print_int(fetch_s(mod, ap), kputchar);
            break;

        case 'u':
            print_uint(fetch_u(mod, ap), 10, 0, kputchar);
            break;

        case 'o': {
            unsigned long long v = fetch_u(mod, ap);
            if (alt && v) kputchar('0');
            print_uint(v, 8, 0, kputchar);
        } break;

        case 'x': case 'X': {
            unsigned long long v = fetch_u(mod, ap);
            if (alt) { kputchar('0'); kputchar((*fmt=='x')?'x':'X'); }
            print_uint(v, 16, *fmt=='X', kputchar);
        } break;

        case 'p':
            kputchar('0'); kputchar('x');
            print_uint((uintptr_t)va_arg(ap, void*), 16, 0, kputchar);
            break;

        case '%': kputchar('%'); ++written; break;

        default : kputchar('?'); ++written; break;
        }
    }

    va_end(ap);
    return written;
}