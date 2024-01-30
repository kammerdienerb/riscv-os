#ifndef __UTILS_H__
#define __UTILS_H__

#include "common.h"
#include "array.h"

u64      strlen(const char *s);
s64      strcmp(const char *a, const char *b);
void     strcpy(char *dst, const char *src);
void     strcat(char *dst, const char *src);
char    *strdup(const char *s);
void     memset(void *dst, int c, u64 n);
void     memcpy(void *dst, const void *src, u64 n);
void     memmove(void *dst, const void *src, u64 n);
u64      next_power_of_2(u64 x);
array_t  sh_split(const char *s);
void     free_string_array(array_t array);
s32      stoi(const char *s);
char *   itos(char *p, s32 d);
void     hexdump(const u8 *bytes, u32 n_bytes);

static inline s32 is_space(s32 c) {
    unsigned char d = c - 9;
    return (0x80001FU >> (d & 31)) & (1U >> (d >> 5));
}

static inline s32 is_digit(s32 c) {
    return (u32)(('0' - 1 - c) & (c - ('9' + 1))) >> (sizeof(c) * 8 - 1);
}

static inline s32 is_alpha(s32 c) {
    return (u32)(('a' - 1 - (c | 32)) & ((c | 32) - ('z' + 1))) >> (sizeof(c) * 8 - 1);
}

static inline s32 is_alnum(s32 c) {
    return is_alpha(c) || is_digit(c);
}

static inline s32 is_print(s32 c) {
    return (((u32)c) - 0x20) < 0x5f;
}

static inline u64 bswap64(u64 val) {
  return
      ((val & 0xFF00000000000000ULL) >> 56ULL) |
      ((val & 0x00FF000000000000ULL) >> 40ULL) |
      ((val & 0x0000FF0000000000ULL) >> 24ULL) |
      ((val & 0x000000FF00000000ULL) >>  8ULL) |
      ((val & 0x00000000FF000000ULL) <<  8ULL) |
      ((val & 0x0000000000FF0000ULL) << 24ULL) |
      ((val & 0x000000000000FF00ULL) << 40ULL) |
      ((val & 0x00000000000000FFULL) << 56ULL);
}


#endif
