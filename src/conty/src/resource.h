#ifndef CONTY_BENCH_RESOURCE_H
#define CONTY_BENCH_RESOURCE_H

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>

/*
 * Generates a memory deallocation utility for type that will
 * invoke cleaner if ptr is not out of bounds
 */
#define CREATE_CLEANER(type, cleaner)               \
    static inline void cleaner##_func(type *ptr)  \
    {                                               \
        if (*ptr)                                   \
            cleaner(*ptr);                          \
    }

/*
 * Marks a variable as a resource to be cleaned once it goes out of scope
 * by the cleaner function. The cleaner must have been created beforehand via
 * CREATE_CLEANER for the variable type
 */
#define MAKE_RESOURCE(cleaner) \
        __attribute__((__cleanup__(cleaner##_func))) __attribute__((unused))

/*
 * Cleans up a file descriptor resource
 */
static inline void fd_cleaner_func(int *fd)
{
    if (*fd >= 0) {
        int err = errno;
        close(*fd);
        errno = err;
        *fd = -EBADF;
    }
}

#define FD_RESOURCE MAKE_RESOURCE(fd_cleaner)

CREATE_CLEANER(FILE *, fclose);
#define FILE_RESOURCE MAKE_RESOURCE(fclose)

/*
 * Cleans up the memory that ptr is referencing
 */
static inline void mem_cleaner_func(void *ptr)
{
    /*
     * ptr is the address of the actual pointer that is referencing
     * the memory we want to deallocate so we just promote it back up
     * and dereference in order to free up the correct value
     */
    free(*(void **) ptr);
}

#define MEM_RESOURCE MAKE_RESOURCE(mem_cleaner)

/*
 * Cleans up a null terminated array of strings
 */
static inline void stringlist_cleaner(char **ptr)
{
    if (ptr) {
        for (int i = 0; ptr[i]; i++)
            free(ptr[i]);
        free(ptr);
    }
}

CREATE_CLEANER(char **, stringlist_cleaner);
#define STRINGLIST_RESOURCE MAKE_RESOURCE(stringlist_cleaner)

/*
 * Moves ptr to a temporary variable that MUST be assigned,
 * otherwise a leak may occur
 */
#define move_ptr(ptr)                             \
    ({                                            \
        __typeof__(ptr) __internal_ptr__ = (ptr); \
        (ptr) = NULL;                             \
        __internal_ptr__;                         \
    })

/*
 * Moves a file descriptor to a temporary variable that MUST be assigned,
 * otherwise a leak will most definitely occur
 */
#define move_fd(fd)                            \
    ({                                         \
        __typeof__(fd) __internal_fd__ = (fd); \
        (fd) = -EBADF;                         \
        __internal_fd__;                       \
    })

/*
 * Clean up all elements in a linked list
 */
#define LIST_CLEAN(list, el_field, cleaner) do {     \
     __typeof((list)->slh_first) __tmp_el__ = NULL;  \
     while (!SLIST_EMPTY(list)) {                    \
         __tmp_el__ = SLIST_FIRST(list);             \
         SLIST_REMOVE_HEAD(list, el_field);          \
         cleaner(__tmp_el__);                        \
     }                                               \
} while (0)

#endif //CONTY_BENCH_RESOURCE_H
