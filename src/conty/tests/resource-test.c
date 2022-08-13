#include "resource.h"
#include "log.h"

#include <stdlib.h>
#include <fcntl.h>

static int test_mem_cleaner()
{
    MEM_RESOURCE void *data = NULL;

    data = malloc(64);
    if (!data)
        return -1;

    return 0;
}

static int test_file_cleaner()
{
    FD_RESOURCE int fd = -EBADF;

    fd = open("/dev/null", O_RDONLY);
    if (fd < 0)
        return -1;

    return 0;
}

static int test_move_ptr()
{
    MEM_RESOURCE void *tmp = NULL;
    MEM_RESOURCE void *data = NULL;

    tmp = malloc(64);
    if (!tmp)
        return -1;

    data = move_ptr(tmp);
    if (tmp)
        return -1;
    if (!data)
        return -1;

    return 0;
}

static int test_move_fd()
{
    FD_RESOURCE int tmp = -EBADF;
    FD_RESOURCE int fd = -EBADF;

    tmp = open("/dev/null", O_RDONLY);
    if (tmp < 0)
        return -1;

    fd = move_fd(tmp);
    if (tmp >= 0)
        return -1;
    if (fd < 0)
        return -1;

    return 0;
}

static int test_stringlist_cleaner()
{
    STRINGLIST_RESOURCE char **stringlist = NULL;
    int i;

    stringlist = malloc(5 * sizeof(char*));
    if (!stringlist)
        return -1;

    for (i = 0; i < 4; i++)
        stringlist[i] = malloc(20);

    stringlist[i] = NULL;

    return 0;
}

int main(int argc, char *argv[])
{
   if (test_mem_cleaner() != 0) {
       LOG_ERROR("test_mem_cleaner failed");
        goto err;
   }

   if (test_file_cleaner() != 0) {
       LOG_ERROR("test_file_cleaner failed");
       goto err;
   }

   if (test_move_fd() != 0) {
       LOG_ERROR("test_move_fd failed");
       goto err;
   }

   if (test_move_ptr() != 0) {
       LOG_ERROR("test_move_ptr failed");
       goto err;
   }

   if (test_stringlist_cleaner() != 0) {
       LOG_ERROR("test_stringlist_cleaner failed");
       goto err;
   }

   return EXIT_SUCCESS;

err:
    return EXIT_FAILURE;
}