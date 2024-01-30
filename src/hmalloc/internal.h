#ifndef __INTERNAL_H__
#define __INTERNAL_H__

#define HMALLOC_ANSI_C
/* #define HMALLOC_DO_LOGGING */
/* #define HMALLOC_DO_ASSERTIONS */
/* #define HMALLOC_USE_SBLOCKS */

#include "common.h"
#include "kprint.h"

#define EXPAND(a) a
#define CAT2(x, y) _CAT2(x, y)
#define _CAT2(x, y) x##y

#define likely(x)   (__builtin_expect(!!(x), 1))
#define unlikely(x) (__builtin_expect(!!(x), 0))

#define internal static
#define external extern

#ifdef HMALLOC_DEBUG
#define HMALLOC_ALWAYS_INLINE
#else
#define HMALLOC_ALWAYS_INLINE __attribute__((always_inline))
#endif /* HMALLOC_DEBUG */

#define LOG(...) ;

#ifdef HMALLOC_DO_ASSERTIONS
internal void hmalloc_assert_fail(const char *msg, const char *fname, int line, const char *cond_str);
#define ASSERT(cond, msg)                                \
do { if (unlikely(!(cond))) {                            \
    hmalloc_assert_fail(msg, __FILE__, __LINE__, #cond); \
} } while (0)
#else
#define ASSERT(cond, mst) ;
#endif

#endif
