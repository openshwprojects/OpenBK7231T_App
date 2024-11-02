#ifndef _PANIC_H_
#define _PANIC_H_

int backtrace_now(int (*print_func)(const char *fmt, ...), uintptr_t *regs);
int backtrace_now_app(int (*print_func)(const char *fmt, ...));

#endif
