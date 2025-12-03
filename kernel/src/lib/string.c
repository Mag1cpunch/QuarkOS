#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <hardware/memory/heap.h>

void *memcpy(void *restrict dest, const void *restrict src, size_t n) {
    uint8_t *restrict pdest = (uint8_t *restrict)dest;
    const uint8_t *restrict psrc = (const uint8_t *restrict)src;

    for (size_t i = 0; i < n; i++) {
        pdest[i] = psrc[i];
    }

    return dest;
}

void *memset(void *s, int c, size_t n) {
    uint8_t *p = (uint8_t *)s;

    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }

    return s;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *pdest = (uint8_t *)dest;
    const uint8_t *psrc = (const uint8_t *)src;

    if (src > dest) {
        for (size_t i = 0; i < n; i++) {
            pdest[i] = psrc[i];
        }
    } else if (src < dest) {
        for (size_t i = n; i > 0; i--) {
            pdest[i-1] = psrc[i-1];
        }
    }

    return dest;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *p1 = (const uint8_t *)s1;
    const uint8_t *p2 = (const uint8_t *)s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] < p2[i] ? -1 : 1;
        }
    }

    return 0;
}

size_t strlen(const char* str) 
{
	size_t len = 0;
	while (str[len])
		len++;
	return len;
}

char *strcat(char *dst, const char *src)
{
    char *d = dst;

    while (*d)
        ++d;

    while ((*d++ = *src++)) ;

    return dst;
}

int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        if (s1[i] != s2[i]) {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
        if (s1[i] == '\0') {
            return 0;
        }
    }
    return 0;
}

String string_empty(void) {
    String s;
    s.data = NULL;
    s.length = 0;
    s.capacity = 0;
    return s;
}

int string_reserve(String* s, size_t min_capacity) {
    if (s->capacity >= min_capacity)
        return 1;

    size_t new_capacity = s->capacity ? s->capacity : 16;
    while (new_capacity < min_capacity) {
        new_capacity *= 2;
    }

    char* new_data;
    if (s->data == NULL) {
        new_data = (char*)kmalloc(new_capacity);
        if (!new_data) return 0;
        new_data[0] = '\0';
    } else {
        new_data = (char*)krealloc(s->data, new_capacity);
        if (!new_data) return 0;
    }

    s->data = new_data;
    s->capacity = new_capacity;

    return 1;
}

int string_from_cstr(String* out, const char* cstr) {
    size_t len = strlen(cstr);
    String s = string_empty();

    if (!string_reserve(&s, len + 1)) {
        return 0;
    }

    for (size_t i = 0; i < len; ++i) {
        s.data[i] = cstr[i];
    }
    s.data[len] = '\0';
    s.length = len;

    *out = s;
    return 1;
}

void string_free(String* s) {
    if (s->data) {
        kfree(s->data);
        s->data = NULL;
    }
    s->length = 0;
    s->capacity = 0;
}

int string_append_cstr(String* s, const char* suffix) {
    size_t suff_len = strlen(suffix);
    size_t new_len = s->length + suff_len;

    if (!string_reserve(s, new_len + 1)) {
        return 0;
    }

    for (size_t i = 0; i < suff_len; ++i) {
        s->data[s->length + i] = suffix[i];
    }

    s->length = new_len;
    s->data[s->length] = '\0';

    return 1;
}

int string_copy(String* dst, const String* src) {
    if (!string_reserve(dst, src->length + 1)) {
        return 0;
    }

    for (size_t i = 0; i < src->length; ++i) {
        dst->data[i] = src->data[i];
    }

    dst->data[src->length] = '\0';
    dst->length = src->length;
    return 1;
}

void string_move(String* dst, String* src) {
    string_free(dst);

    dst->data = src->data;
    dst->length = src->length;
    dst->capacity = src->capacity;

    src->data = NULL;
    src->length = 0;
    src->capacity = 0;
}
