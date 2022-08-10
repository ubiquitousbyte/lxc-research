#ifndef CONTY_LOG_H
#define CONTY_LOG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdarg.h>
#include <time.h>

/*
 * Log event object
 */
struct conty_log_event {
    va_list     varargs;
    const char *fmt;
    const char *file;
    void       *udata;
    int         line;
    int         lvl;
};

typedef void (*conty_log_cb)(struct conty_log_event *event);

void conty_log_set_quiet(int quiet);
void conty_log_set_level(int level);

enum {
    CONTY_LOG_TRACE,
    CONTY_LOG_DEBUG,
    CONTY_LOG_INFO,
    CONTY_LOG_WARN,
    CONTY_LOG_ERROR,
    CONTY_LOG_FATAL
};

#define LOG_TRACE(...) conty_log(CONTY_LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_DEBUG(...) conty_log(CONTY_LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...) conty_log(CONTY_LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...) conty_log(CONTY_LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) conty_log(CONTY_LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_FATAL(...) conty_log(CONTY_LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

#define LOG_ERROR_RET(__ret__, fmt, ...)            \
        ({                                                \
            __typeof(__ret__) __internal_ret__ = (__ret__); \
            LOG_ERROR(fmt, ##__VA_ARGS__);          \
            __internal_ret__;                             \
        })

#define LOG_ERROR_RET_ERRNO(__ret__, __errno__, fmt, ...) \
    ({                                                          \
        __typeof(__ret__) __int_ret__ = (__ret__);                \
        errno = __errno__;                                      \
        LOG_ERROR(fmt, ##__VA_ARGS__);                    \
        __int_ret__;                                            \
    })

#define LOG_FATAL_RET(__ret__, fmt, ...)            \
        ({                                                \
            __typeof(__ret__) __internal_ret__ = (__ret__); \
            LOG_FATAL(fmt, ##__VA_ARGS__);          \
            __internal_ret__;                             \
        })

#define LOG_FATAL_RET_ERRNO(__ret__, __errno__, fmt, ...) \
    ({                                                          \
        typeof(__ret__) __int_ret__ = (__ret__);                \
        errno = __errno__;                                      \
        LOG_FATAL(fmt, ##__VA_ARGS__);                    \
        __int_ret__;                                            \
    })

void conty_log(int level, const char *file, int line, const char *fmt, ...);

#ifdef __cplusplus
}; // extern "C"
#endif
#endif //CONTY_LOG_H
