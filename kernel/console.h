#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include "types.h"

/* Console I/O functions */
void con_init(void);
void con_putc(char c);
void con_getc(char *buf);
void con_gets(char *buf, size_t len);
void con_printf(const char *fmt, ...);

#endif /* __CONSOLE_H__ */
