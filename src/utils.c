#include "utils.h"
#include "kmalloc.h"
#include "kprint.h"

u64 strlen(const char *s) {
    u64 len;

    len = 0;

    while (*s != 0) { s += 1; len += 1; }

    return len;
}

s64 strcmp(const char *a, const char *b) {
    s64 diff;

    while ((diff = *a - *b) == 0 && *a != 0) { a += 1; b += 1; }

    return diff;
}

void strcpy(char *dst, const char *src) {
    memcpy(dst, src, strlen(src) + 1);
}

void strcat(char *dst, const char *src) {
    u64 offset;

    offset = strlen(dst);

    strcpy(dst + offset, src);
}

char *strdup(const char *s) {
    u64   len;
    char *result;

    len    = strlen(s);
    result = kmalloc(len + 1);
    memcpy(result, s, len + 1);

    return result;
}

void memset(void *dst, int c, u64 n) {
    const u8  word[8] = { c, c, c, c, c, c, c, c };
    void     *p;

    for (p = dst; !IS_ALIGNED(p, 8); p += 1) {
        *(u8*)p = c;
    }

    for (; p < ALIGN(dst + n, 8); p += 8) {
        *(u64*)p = *(u64*)word;
    }

    for (; p < dst + n; p += 1) {
        *(u8*)p = c;
    }
}

void memcpy(void *dst, const void *src, u64 n) {
    u64 i;
    for (i = 0; i < n; i += 1) {
        *((u8*)dst + i) = *((u8*)src +i);
    }
}

void memmove(void *dst, const void *src, u64 n) {
    s64 i;

    if (dst <= src) {
        memcpy(dst, src, n);
        return;
    }

    for (i = n - 1; i >= 0; i -= 1) {
        *((u8*)dst + i) = *((u8*)src +i);
    }
}

u64 next_power_of_2(u64 x) {
    if (x == 0) { return 2; }

    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    x |= x >> 32;
    x++;

    return x;
}

s32 stoi(const char *s) {
    u32 is_neg;
    s32 r;

    is_neg = *s == '-';
    if (is_neg) { s += 1; }

    r = 0;
    while (*s) {
        r *= 10;
        r += *s - '0';
        s += 1;
    }

    if (is_neg) { r = -r; }

    return r;
}

char * itos(char *p, s32 d) {
    int neg;

    neg = d < 0;

    if (neg) { d = -d; }

    p += 3 * sizeof(s32);
    *--p = 0;

    do {
        *--p = '0' + d % 10;
        d /= 10;
    } while (d);

    if (neg) { *--p = '-'; }

    return p;
}

array_t sh_split(const char *s) {
    array_t  r;
    char    *copy,
            *sub,
            *sub_p;
    char     c, prev;
    int      len,
             start,
             end,
             q,
             sub_len,
             i;

    r     = array_make(char*);
    copy  = strdup(s);
    len   = strlen(copy);
    start = 0;
    end   = 0;
    prev  = 0;

    while (start < len && is_space(copy[start])) { start += 1; }

    while (start < len) {
        c   = copy[start];
        q   = 0;
        end = start;

        if (c == '#' && prev != '\\') {
            break;
        } else if (c == '"') {
            start += 1;
            prev   = copy[end];
            while (end + 1 < len
            &&    (copy[end + 1] != '"' || prev == '\\')) {
                end += 1;
                prev = copy[end];
            }
            q = 1;
        } else if (c == '\'') {
            start += 1;
            prev   = copy[end];
            while (end + 1 < len
            &&    (copy[end + 1] != '\'' || prev == '\\')) {
                end += 1;
                prev = copy[end];
            }
            q = 1;
        } else {
            while (end + 1 < len
            &&     !is_space(copy[end + 1])) {
                end += 1;
            }
        }

        sub_len = end - start + 1;
        if (q && sub_len == 0 && start == len) {
            sub    = kmalloc(2);
            sub[0] = copy[end];
            sub[1] = 0;
        } else {
            sub   = kmalloc(sub_len + 1);
            sub_p = sub;
            for (i = 0; i < sub_len; i += 1) {
                c = copy[start + i];
                if (c == '\\'
                &&  i < sub_len - 1
                &&  (copy[start + i + 1] == '"'
                || copy[start + i + 1] == '\''
                || copy[start + i + 1] == '#')) {
                    continue;
                }
                *sub_p = c;
                sub_p += 1;
            }
            *sub_p = 0;
        }

        array_push(r, sub);

        end  += q;
        start = end + 1;

        while (start < len && is_space(copy[start])) { start += 1; }
    }

    kfree(copy);

    return r;
}

void free_string_array(array_t array) {
    char **it;

    array_traverse(array, it) {
        kfree(*it);
    }

    array_free(array);
}

void hexdump(const u8 *bytes, u32 n_bytes) {
    u32  i;
    u32  j;
    char c;
    char s[17];

    i = 0;

    while (n_bytes >= 16) {
        for (j = 0; j < 16; j += 1) {
            c = bytes[i + j];
            if (is_print(c) && (c == ' ' || !is_space(c))) {
                s[j] = c;
            } else {
                s[j] = '.';
            }
        }
        s[16] = 0;

        kprint("%m%08X%_:  ", i);

        for (j = 0; j < 16; j += 1) {
            kprint("%b%02x%_", bytes[i]);
            i       += 1;
            n_bytes -= 1;

            if (j & 1) { kprint(" "); }
        }

        kprint("    ");


        kprint("%g%-16s%_\n", s);

    }

    if (n_bytes > 0) {
        kprint("%m%08X%_:  ", i);

        for (j = 0; j < n_bytes; j += 1) {
            c = bytes[i + j];
            if (is_print(c) && !is_space(c)) {
                s[j] = c;
            } else {
                s[j] = '.';
            }
        }
        s[j] = 0;

        j = 3;
        while (n_bytes > 0) {
            kprint("%b%02x%_", bytes[i]);
            i       += 1;
            j       += 1;
            n_bytes -= 1;

            if (j & 1) { kprint(" "); }
        }

        while (i & 15) {
            kprint("  ");
            if (i & 1) { kprint(" "); }
            i += 1;
        }

        kprint("    %g%-16s%_\n", s);
    }
}
