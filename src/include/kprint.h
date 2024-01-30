#ifndef __KPRINT_H__
#define __KPRINT_H__

#include "common.h"

#include <stdarg.h>

#define PR_RESET       "\e[0m"
#define PR_CLS         "\e[2J"
#define PR_CURSOR_HOME "\e[H"
#define PR_FG_BLACK    "\e[30m"
#define PR_FG_BLUE     "\e[34m"
#define PR_FG_GREEN    "\e[32m"
#define PR_FG_CYAN     "\e[36m"
#define PR_FG_RED      "\e[31m"
#define PR_FG_YELLOW   "\e[33m"
#define PR_FG_MAGENTA  "\e[35m"
#define PR_BG_BLACK    "\e[40m"
#define PR_BG_BLUE     "\e[44m"
#define PR_BG_GREEN    "\e[42m"
#define PR_BG_CYAN     "\e[46m"
#define PR_BG_RED      "\e[41m"
#define PR_BG_YELLOW   "\e[43m"
#define PR_BG_MAGENTA  "\e[45m"

void kputc(u8 c);
u8   kgetc(void);
void vkprint(const char *fmt, va_list args);
void kprint(const char *fmt, ...);
void kprint_set_putc(void (*putc)(u8));
void kprint_set_getc(u8 (*getc)(void));

#endif
