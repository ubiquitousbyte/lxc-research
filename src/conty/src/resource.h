/*
 * Both gcc and clang implement a cleanup attribute that allows
 * function invocation whenever a variable goes out of scope
 *
 * It makes sense to utilise this feature to avoid descriptor and memory leaks
 */
#ifndef CONTY_RESOURCE_H
#define CONTY_RESOURCE_H

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

/*
 * Inspired by
 * "A good and idiomatic way to use gcc and clang attribute cleanup" question
 * on stack overflow
 *
 * This bad boy right here is what renders C++ completely unnecessary
 */
#define CONTY_CREATE_CLEANUP_FUNC(type, cleaner)         \
        static inline void cleaner##_function(type *ptr) \
        {                                                \
            if (*ptr)                                    \
                cleaner(*ptr);                           \
        }

#define CONTY_INVOKE_CLEANER(cleaner) \
	    __attribute__((__cleanup__(cleaner##_function))) __attribute__((unused))

#define CONTY_FD_CLEANER(fd)     \
        if ((fd) >= 0) {         \
            int __er_ = errno;   \
            close(fd);           \
            errno = __er_;       \
            (fd) = -EBADF;       \
        }

static inline void conty_close_fd_function(int *fd)
{
    CONTY_FD_CLEANER(*fd);
}

#define __CONTY_CLOSE CONTY_INVOKE_CLEANER(conty_close_fd)

#define CONTY_MEM_CLEANER(ptr) \
        ({                     \
            if (ptr) {         \
                free(ptr);     \
                (ptr) = NULL;  \
            }                  \
        })

static inline void conty_free_mem_function(void *ptr)
{
    CONTY_MEM_CLEANER(*(void **) ptr);
}

static inline void conty_free_strings(char **ptr)
{
    if (ptr) {
        for (int i = 0; ptr[i]; i++)
            free(ptr[i]);
        free(ptr);
    }
}

#define __CONTY_FREE CONTY_INVOKE_CLEANER(conty_free_mem)

CONTY_CREATE_CLEANUP_FUNC(char **, conty_free_strings);
#define __CONTY_FREE_STRLIST CONTY_INVOKE_CLEANER(conty_free_strings)

#define CONTY_MOVE_PTR(ptr)                     \
    ({                                          \
        __typeof(ptr) __internal_ptr__ = (ptr); \
        (ptr) = NULL;                           \
        __internal_ptr__;                       \
    })

#define CONTY_MOVE_FD(fd)                    \
    ({                                       \
        __typeof(fd) __internal_fd__ = (fd); \
        (fd) = -EBADF;                       \
        __internal_fd__;                     \
    })

#define CONTY_SNPRINTF(buf, buf_size, ...)                                        \
    ({                                                                            \
        int __ret__snprintf;                                                      \
        __ret__snprintf = snprintf(buf, buf_size, ##__VA_ARGS__);                 \
        if (__ret__snprintf < 0 || (size_t) __ret__snprintf >= (size_t) buf_size) \
            __ret__snprintf = -EIO;                                               \
        __ret__snprintf;                                                          \
    })

#endif //CONTY_RESOURCE_H
