#ifndef CONTY_SAFESTRING_H
#define CONTY_SAFESTRING_H

#include <errno.h>
#include <stdio.h>

/*
 * Calls snprintf and checks if the number of bytes written potentially
 * causes an off by x error, setting the return value to -EIO if it does
 */
#define strnprintf(buf, size, ...)                                            \
    ({                                                                        \
        int __internal__ret;                                                  \
        __internal__ret = snprintf(buf, size, ##__VA_ARGS__);                 \
        if (__internal__ret < 0 || (size_t) __internal__ret >= (size_t) size) \
            __internal__ret = -EIO;                                           \
        __internal__ret;                                                      \
    })

#endif //CONTY_SAFESTRING_H
