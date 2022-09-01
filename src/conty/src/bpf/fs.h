#ifndef CONTY_FS_H
#define CONTY_FS_H

enum fs_file_ops {
    OPEN,
    READ,
    WRITE,
    FSYNC,
    MAX_OP
};

static const char *file_op_names[] = {
        [READ]  = "read",
        [WRITE] = "write",
        [OPEN]  = "open",
        [FSYNC] = "fsync",
};

#endif //CONTY_FS_H
