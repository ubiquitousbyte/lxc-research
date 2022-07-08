#ifndef CONTY_CONTY_H
#define CONTY_CONTY_H

#include <sys/stat.h>
#include <sched.h>

#include <memory>
#include <ostream>

/*
 *  A context is a wrapper around a system resource.
 *  Only processes inside the context can see and use the resource.
 */
class context {
public:
    /* The context variant represents the resource that the context wraps */
    enum variant {
        Cgroup = CLONE_NEWCGROUP,
        Ipc = CLONE_NEWIPC,
        Net = CLONE_NEWNET,
        Mount = CLONE_NEWNS,
        Pid = CLONE_NEWPID,
        User = CLONE_NEWUSER,
        Uts = CLONE_NEWUTS
    };

    /* Contexts can be uniquely identified and therefore compared */
    bool operator==(const context& o) const;

    /* Prints the context to stdout as defined in /proc */
    friend std::ostream& operator <<(std::ostream& stream,
                                     const context& ctx);

    enum variant type() const;

    ino_t inode() const;

    /* Moves the calling process into this execution context */
    void join();

    /*
     * Detaches the calling process from its current set of contexts
     * The contexts to detach from are specified in the variants.
     * The context::variant types can be ORed and used here
     */
    static void detach(int variants);

    /* Finds the context of the process identified by pid with the variant v */
    static context find(pid_t pid, enum variant v);

    /* Finds the context of the current process */
    static context current(enum variant v);

    ~context();
private:
    struct impl;
    std::unique_ptr<impl> ctx;

    explicit context(impl&& ctx);
};
#endif //CONTY_CONTY_H
