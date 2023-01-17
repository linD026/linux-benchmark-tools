#ifndef __DEBUG_H__
#define __DEBUG_H__

#ifndef likely
#define likely(x) __builtin_expect(!!(x), 1)
#endif

#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif

#ifndef container_of
#define container_of(ptr, type, member)                        \
    __extension__({                                            \
        const __typeof__(((type *)0)->member) *__mptr = (ptr); \
        (type *)((char *)__mptr - offsetof(type, member));     \
    })
#endif

#ifndef __always_inline
#define __always_inline inline __attribute__((__always_inline__))
#endif

#ifndef __noinline
#define __noinline __attribute__((__noinline__))
#endif

#ifndef __allow_unused
#define __allow_unused __attribute__((unused))
#endif

#ifndef macro_var_args_count
#define macro_var_args_count(...) \
    (sizeof((void *[]){ 0, __VA_ARGS__ }) / sizeof(void *) - 1)
#endif

#ifndef ___PASTE
#define ___PASTE(a, b) a##b
#endif

#ifndef __PASTE
#define __PASTE(a, b) ___PASTE(a, b)
#endif

#ifndef __UNIQUE_ID
#define __UNIQUE_ID(prefix) __PASTE(__PASTE(__UNIQUE_ID_, prefix), __LINE__)
#endif

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#ifndef min
#define min(l, r) ((l < r) ? l : r)
#endif

#ifndef max
#define max(l, r) ((l > r) ? l : r)
#endif

#include <stdio.h>

#define debug_stream stdout
#define err_stream stderr

#define print(fmt, ...)                            \
    do {                                           \
        fprintf(debug_stream, fmt, ##__VA_ARGS__); \
    } while (0)

#define pr_info(fmt, ...)                                             \
    do {                                                              \
        print("\e[32m[INFO]\e[0m %s:%d:%s: " fmt, __FILE__, __LINE__, \
              __func__, ##__VA_ARGS__);                               \
    } while (0)

#define pr_err(fmt, ...)                                      \
    do {                                                      \
        fprintf(err_stream,                                   \
                "\e[32m[ERROR]\e[0m %s:%d:%s: "               \
                "\e[31m" fmt "\e[0m",                         \
                __FILE__, __LINE__, __func__, ##__VA_ARGS__); \
    } while (0)

#include <stdlib.h>
#include <execinfo.h>

#define STACK_BUF_SIZE 32

static inline void dump_stack(void)
{
    char **stack_info;
    int nr = 0;
    void *buf[STACK_BUF_SIZE];

    nr = backtrace(buf, STACK_BUF_SIZE);
    stack_info = backtrace_symbols(buf, nr);

    print("========== dump stack start ==========\n");
    for (int i = 0; i < nr; i++)
        print("  %s\n", stack_info[i]);
    print("========== dump stack  end  ==========\n");
}

#define BUG_ON(cond, fmt, ...)                                     \
    do {                                                           \
        if (unlikely(cond)) {                                      \
            pr_err("BUG ON: " #cond ", " fmt "\n", ##__VA_ARGS__); \
            dump_stack();                                          \
            exit(EXIT_FAILURE);                                    \
        }                                                          \
    } while (0)

#define WARN_ON(cond, fmt, ...)                                    \
    do {                                                           \
        if (unlikely(cond))                                        \
            pr_err("WARN ON:" #cond ", " fmt "\n", ##__VA_ARGS__); \
    } while (0)

#define pr_debug(fmt, ...) pr_info("DEBUG: " fmt, ##__VA_ARGS__)

#endif /* __OSC_DEBUG_H__ */

