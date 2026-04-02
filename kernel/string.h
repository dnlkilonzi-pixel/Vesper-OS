#ifndef STRING_H
#define STRING_H

#include <stdint.h>

/*
 * VESPER OS – Minimal string/memory utilities
 *
 * Provided as inline functions so no .c file is needed.  Only the subset
 * actually used by the kernel is implemented here.
 */

static inline void *memset(void *s, int c, uint32_t n)
{
    uint8_t *p = (uint8_t *)s;
    while (n--) {
        *p++ = (uint8_t)c;
    }
    return s;
}

static inline void *memcpy(void *dst, const void *src, uint32_t n)
{
    uint8_t       *d = (uint8_t *)dst;
    const uint8_t *s = (const uint8_t *)src;
    while (n--) {
        *d++ = *s++;
    }
    return dst;
}

static inline int memcmp(const void *a, const void *b, uint32_t n)
{
    const uint8_t *pa = (const uint8_t *)a;
    const uint8_t *pb = (const uint8_t *)b;
    while (n--) {
        if (*pa != *pb) {
            return (int)(unsigned int)*pa - (int)(unsigned int)*pb;
        }
        pa++; pb++;
    }
    return 0;
}

static inline uint32_t strlen(const char *s)
{
    uint32_t n = 0;
    while (s[n]) {
        n++;
    }
    return n;
}

static inline int strcmp(const char *a, const char *b)
{
    while (*a && *b && *a == *b) {
        a++; b++;
    }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

static inline int strncmp(const char *a, const char *b, uint32_t n)
{
    while (n > 0 && *a && *b && *a == *b) {
        a++; b++; n--;
    }
    if (n == 0) {
        return 0;
    }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

static inline char *strncpy(char *dst, const char *src, uint32_t n)
{
    uint32_t i;
    for (i = 0; i < n && src[i]; i++) {
        dst[i] = src[i];
    }
    for (; i < n; i++) {
        dst[i] = '\0';
    }
    return dst;
}

#endif /* STRING_H */
