/**
 * When we fork a process, the kernel lazily allocates a copy of the
 * parent's data, heap and stack segments. What does lazily mean?
 * It means that the copy will be allocated if and only if the child
 * or parent perform a write operation on a variable located in those
 * segments. In other words, the segments have copy-on-write semantics.
 * The text segment is marked read-only and is shared between both
 * processes, i.e it need not be copied.
 *
 * The child process differs from its parent in the following points:
 * 1. The child has its own unique process identifier
 * 2. The child's parent process id is the same as the parent's process id
 * 3. The child does NOT inherit its parent's memory locks
 * 4. Process resource configurations and CPU time counters are reset to 0
 * 5. The child's set of pending signals is empty
 * 6. The child does NOT inherit semaphore adjustments from its parent
 * 7. The child does NOT inherit process-associated record locks
 *    from its parent. It does, however, inherit file-associated locks
 * 8. The child does NOT inherit timers from its parent
 * 9. The child does NOT inherit outstanding async I/O operations
 *     from its parent
 * 10. The child does NOT inherit directory change notifications
 *     from its parent
 * 11. The termination signal of the child is set to SIGCHLD. Note that
 *     this is Linux-specific.
 *
 * Further note
 * 1. Since the virtual address space of the parent is copied, the child inherits
 *    mutex states, states of condition variables and other pthread objects
 * 2. The child inherits copies of the parent's set of file descriptors.
 *    The file descriptors in both the parent and child point to the same
 *    in-kernel file description. This means that the parent and child share
 *    open file status flags, file offsets and signal-driven I/O attributes!
 * 3. The child inherits copies of the parent's set of open directory streams
 * 4. The child inherits copies of the parent's set of open message queue
 *    descriptors. Similarly to 2., the descriptors point to the same in-kernel
 *    queue descriptions.
 * 5. The child inherits the real user id and real group id from its parent
 * 6. The child also inherits the supplementary group ids from its parent
 * 7. The child inherits copies of its parent's capability sets.
 */
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/wait.h>
#include <sys/capability.h>

void show_credentials()
{
    int rc;
    pid_t pid = getpid();
    struct __user_cap_header_struct header = {
            .version = _LINUX_CAPABILITY_VERSION_3,
            .pid = pid
    };
    struct __user_cap_data_struct data = {0};

    /**
     * The execution path is as follows:
     * 1. The kernel will validate that the version in the header matches v3.
     *    If it does not, the kernel will advertise its preferred
     *    version by overriding what we've stored in the header.
     *    https://github.com/torvalds/linux/blob/master/kernel/capability.c#L82
     *
     * 2. Afterwards, the kernel will try to find the task associated with the
     *    process identifier specified in the header. It does this by first
     *    taking the pid namespace of the calling process and querying the
     *    namespace's identifier registry for the pid in the header.
     *    The identifier registry is represented as a radix tree.
     *    If the pid is part of the namespace, the kernel will look up the
     *    task structure associated with that pid
     *
     *    See:
     *    https://github.com/torvalds/linux/blob/master/kernel/capability.c#L126
     *    https://github.com/torvalds/linux/blob/master/kernel/pid.c#L420
     *    https://github.com/torvalds/linux/blob/master/kernel/pid.c#L309
     *    https://github.com/torvalds/linux/blob/master/include/linux/idr.h#L20
     *
     * 3. After finding the task structure, the kernel triggers a Linux
     *    Security Module hook that reads the credentials structure embedded in
     *    the task and copies its effective, inheritable and permitted capability
     *    sets into the user-defined memory block __user_cap_data_struct
     *
     *    See:
     *    https://github.com/torvalds/linux/blob/master/security/commoncap.c#L200
     *    https://github.com/torvalds/linux/blob/master/include/linux/cred.h#L128
     */
    rc = capget(&header, &data);
    if (rc != 0)
        return;

    /*
     * Query the effective and real user identifier of the current process.
     * The latter corresponds to the user that owns the executable currently
     * being executed. The kernel uses the effective user identifier to perform
     * access control in runtime. That is, if the current process attempts
     * to open a file, the kernel will check if the effective uid of the process
     * is resident in the access control list of that file. If it is, then
     * this process will be granted access. If it is not, errno will be set
     * accordingly.
     */
    uid_t real_uid = getuid();
    uid_t effective_uid = geteuid();

    /*
     * Also get the parent process identifier
     */
    pid_t ppid = getppid();

    printf("|%-5d|%-7d|%-7d|%-7d|0x%010x|0x%013x|0x%011x|\n", pid, ppid, real_uid,
           effective_uid, data.permitted, data.effective, data.inheritable);
}

int main(int argc, char *argv[])
{
    printf("|PID   |PPID   |RUID   |EUID   |PERM. CAPS"
           "  |EFFECT. CAPS   |INHERIT. CAPS|\n");

    /*
     * Show the current process's credentials
     */
    show_credentials();

    pid_t pid = fork();
    if (pid < 0)
        return 1;

    if (pid == 0) {
        show_credentials();
        exit(EXIT_SUCCESS);
    }

    int status;
    return waitpid(pid, &status, 0) != pid;
}