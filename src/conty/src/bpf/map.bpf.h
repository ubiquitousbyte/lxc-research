#ifndef CONTY_MAP_BPF_H
#define CONTY_MAP_BPF_H

#include <bpf/bpf_helpers.h>
#include <asm-generic/errno.h>

/*
 * Try to find the value associated with key in the map and if not present,
 * add it and return the added value
 */
static __always_inline void *bpf_map_lookup_or_try_init(void *map, const void *key,
                                                        const void *init)
{
    void *val;
    long err;

    val = bpf_map_lookup_elem(map, key);
    if (val)
        return val;

    err = bpf_map_update_elem(map, key, init, BPF_NOEXIST);
    if (err && err != -EEXIST)
        return 0;

    return bpf_map_lookup_elem(map, key);
}

#endif //CONTY_MAP_BPF_H
