#include "internal.h"
#include "internal_malloc.h"
#include "utils.h"

#ifdef HMALLOC_DO_ASSERTIONS
internal void hmalloc_assert_fail(const char *msg, const char *fname, int line, const char *cond_str) {
    kprint("Assertion failed -- %s\n"
           "at  %s :: line %d\n"
           "    Condition: '%s'\n"
           , msg, fname, line, cond_str);
}
#endif
