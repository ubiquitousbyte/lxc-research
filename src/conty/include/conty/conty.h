#ifndef CONTY_CONTY_H
#define CONTY_CONTY_H

#include <cstdlib>

#include <ostream>

class resource_namespace {
public:
    /**
     * The namespace type
     */
    enum class type: char {
        Cgroup,
        Ipc,
        Net,
        Mount,
        Pid,
        Time,
        User,
        Uts
    };

    bool operator==(const resource_namespace& o) const;

    friend std::ostream& operator <<(std::ostream& stream,
                                     const resource_namespace& ns);

    /**
     * @return the type of this namespace
     */
    enum type namespace_type() const;

    /**
     * @return the inode number of this namespace
     */
    ino_t inode() const;

    /*
     * Retrieves the namespace of the given type from the proc pseudo-filesystem
     * for the process identified by pid.
     */
    static resource_namespace from_proc(pid_t pid, enum type type);

    /**
     * Retrieves the namespace of the given type for the current process
     * @param type the namespace type
     * @return the namespace to which the current process belongs to
     */
    static resource_namespace current(enum type type);

private:
    resource_namespace(dev_t device, ino_t node, enum type type);

    dev_t device;
    ino_t node;
    type  type;
};

#endif //CONTY_CONTY_H
