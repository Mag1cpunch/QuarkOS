#include <stddef.h>
#include <stdint.h>

typedef struct {
    char*  data;
    size_t length;
    size_t capacity;
} String;

#define STR(s) ((s).data)

String string_empty(void);
int string_reserve(String* s, size_t min_capacity);
int string_from_cstr(String* out, const char* cstr);
void string_free(String* s);
int string_append_cstr(String* s, const char* suffix);
int string_append(String* s, const String* suffix);
int string_copy(String* dst, const String* src);
void string_move(String* dst, String* src);

void *memcpy(void *restrict dest, const void *restrict src, size_t n);
void *memset(void *s, int c, size_t n);
void *memmove(void *dest, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);

size_t strlen(const char* str);
char *strcat(char *dst, const char *src);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);