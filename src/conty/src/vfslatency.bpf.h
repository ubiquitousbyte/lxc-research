#ifndef CONTY_VFSLATENCY_BPF_H
#define CONTY_VFSLATENCY_BPF_H

enum fs_file_ops {
    OPEN,
    READ,
    WRITE,
    FSYNC,
    MAX_OP
};

#endif //CONTY_VFSLATENCY_BPF_H
