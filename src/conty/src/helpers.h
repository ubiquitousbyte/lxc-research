
#ifndef CONTY_HELPERS_H
#define CONTY_HELPERS_H

#ifdef __cplusplus
extern "C" {
#endif

#define CONTAINER_OF(POINTER, STRUCT, MEMBER)                           \
        ((STRUCT *) (void *) ((char *) (POINTER) - offsetof (STRUCT, MEMBER)))

#ifdef __cplusplus
}; // extern "C"
#endif

#endif //CONTY_HELPERS_H
