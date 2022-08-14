#include "user.h"

#include "log.h"

int test_id_map_put()
{
    struct conty_id_map map;

    conty_id_map_init(&map);

    if (conty_id_map_put(&map, 10, 10, 1000) != 0)
        return -1;

    return 0;
}

int main(int argc, char *argv[])
{
    if (test_id_map_put() != 0)
        return log_error_ret(EXIT_FAILURE, "test_id_map_put failed");
    return 0;
}