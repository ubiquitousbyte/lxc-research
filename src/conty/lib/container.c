#include "container.h"

#include <fcntl.h>
#include <sys/wait.h>

#include "resource.h"
#include "log.h"
#include "sync.h"
#include "clone.h"
#include "user.h"
#include "mount.h"

static int init_namespaces(struct conty_container *cc);
static int ns_sharer(void *arg);
static int container_entrypoint(void *arg);

struct conty_container *conty_container_create(const char *id, const char *bundle)
{
    MAKE_RESOURCE(conty_container_delete) struct conty_container *cc = NULL;

    cc = malloc(sizeof(struct conty_container));
    if (!cc)
        return log_fatal_ret(NULL, "out of memory");

    if (conty_container_init(cc, id, bundle) != 0)
        return NULL;

    if (conty_container_spawn(cc) != 0)
        return NULL;

    conty_sync_init_runtime(cc->cc_syncfds);

    /*
     * The container process is running, it's first job is to tell us that
     * we need to set up the runtime environment on the host, hence, we
     * wait for that event first
     */
    if (conty_sync_await_container(cc->cc_syncfds, EVENT_RT_CREATE) != 0)
        goto reap_and_exit;

    /*
     * TODO: Run runtime hooks here
     */

    /*
     * Great, the runtime environment is set up, now instruct the container
     * that it needs to proceed by pivoting into the new environment
     * and acknowledging that all went well
     */
    if (conty_sync_container(cc->cc_syncfds, EVENT_CONT_CREATE) != 0)
        goto reap_and_exit;

    return move_ptr(cc);

reap_and_exit:
    waitpid(cc->cc_pid, NULL, 0);
    return NULL;
}

int conty_container_start(struct conty_container *container)
{
    /*
     * The container must be waiting to be started, so simply instruct
     * it to execute and wait for a status report
     */
    if (conty_sync_container(container->cc_syncfds, EVENT_CONT_START) != 0)
        goto reap_and_exit;

    return 0;

reap_and_exit:
    waitpid(container->cc_pid, NULL, 0);
    return -1;
}

int conty_container_kill(struct conty_container *container, int sig)
{
    if (pidfd_send_signal(container->cc_pollfd, sig, NULL, 0) != 0)
        return log_error_ret(-errno, "cannot kill container %s", container->cc_id);
    return 0;
}

int conty_container_delete(struct conty_container *container)
{
    if (container) {
        if (container->cc_conf)
            oci_conf_free(container->cc_conf);


    }
}

int conty_container_init(struct conty_container *cc, const char *id, const char *bundle)
{
    int err;
    struct oci_conf *conf = NULL;

    if (!(conf = oci_conf_deser_file(bundle)))
        return -EINVAL;

    cc->cc_conf   = move_ptr(conf);
    cc->cc_id     = id;
    cc->cc_pollfd = -EBADF;

    if ((err = conty_sync_init(cc->cc_syncfds)) != 0)
        return err;

    return init_namespaces(cc);
}

int conty_container_spawn(struct conty_container *cc)
{
    if (cc->cc_ns_has_fds) {
        /*
         * If the container needs to join a set of existing namespaces,
         * we need to spawn an intermediate process that moves itself
         * in them and then forks off the actual container process.
         *
         * Because this process will be short-lived, and a simple configuration
         * necessity, we optimise with CLONE_VFORK and CLONE_VM.
         * The latter flag configures the intermediate process to share
         * the virtual memory pages with the runtime to avoid copy on write
         * semantics of a typical fork. The former flag makes sure that the
         * parent is suspended until the intermediate process calls _exit.
         * This avoids memory corruption
         */
        int istatus;
        pid_t ipid;
        int flags = CLONE_VFORK | CLONE_VM | CLONE_FILES;

        ipid = clone_old(ns_sharer, (void *) cc, flags, NULL);
        if (ipid < 0)
            return log_error_ret(-errno, "cannot spawn container");

        if (waitpid(ipid, &istatus, 0) != ipid)
            return log_error_ret(-errno, "cannot await namespace sharer");

        if (!WIFEXITED(istatus))
            return log_error_ret(-errno, "namespace sharer died");
        else if (WEXITSTATUS(istatus) != 0)
            return log_error_ret(-errno, "namespace sharer failed");
    } else {
        /*
         * Container has no namespaces to join ergo directly
         * fork off the container process
         */
        cc->cc_pid = clone3_cb(container_entrypoint, (void *) cc,
                               cc->cc_ns_new | CLONE_PIDFD, &cc->cc_pollfd);
        if (cc->cc_pid < 0)
            return log_error_ret(-errno, "cannot spawn container");
    }
    return 0;
}

static int container_entrypoint(void *arg)
{
    CONTAINER_RESOURCE struct conty_container *cc = (struct conty_container *) arg;
    struct oci_conf *conf = cc->cc_conf;
    struct oci_process *proc = &conf->oc_proc;
    struct conty_rootfs rootfs;
    int err;

    conty_sync_init_container(cc->cc_syncfds);

    /*
     * First, the child will wake the parent and instruct it to run
     * the runtime hooks.
     * In parallel, we'll set up the security context and the root filesystem
     */
    if (conty_sync_wake_runtime(cc->cc_syncfds, EVENT_RT_CREATE) != 0)
        goto err_out;

    if (cc->cc_ns_new & CLONE_NEWUSER) {
        /*
         * The caller has requested the creation of a new user namespace,
         * so we set up the uid/gid mappings between the host and the container
         *
         * The kernel will then be able to do proper authorization
         * It is very important we set up the user namespace first, because
         * it is superordinate to all subsequently created namespaces
         */
        if (conty_id_map_write_oci_uids(&conf->oc_uids) != 0)
            goto err_notify_runtime;

        if (conty_id_disable_setgroups() != 0)
            goto err_notify_runtime;

        if (conty_id_map_write_oci_gids(&conf->oc_gids) != 0)
            goto err_notify_runtime;
    }

    if (cc->cc_ns_new & CLONE_NEWNS) {
        /*
         * Alright, so we need to create a new root filesystem,
         * we start by bind mounting the OCI path onto itself in order
         * to create a mount point
         */
        struct oci_rootfs *oci_root = &conf->oc_rootfs;

        if (conty_rootfs_init(&rootfs, oci_root->orfs_path, oci_root->orfs_readonly) != 0)
            goto err_notify_runtime;

        if (conty_rootfs_mount(&rootfs) != 0)
            goto err_notify_runtime;

        /*
         * Next, we create the device mount points under the new root
         * This includes /dev/shm and /dev/mqueue to ensure that
         * container applications can use the POSIX IPC APIs
         */
        if (conty_rootfs_mount_dev(&rootfs) != 0)
            goto err_notify_runtime;

        if (conty_rootfs_mount_shm(&rootfs) != 0)
            goto err_notify_runtime;

        if (conty_rootfs_mount_mqueue(&rootfs) != 0)
            goto err_notify_runtime;

        /*
         * We need to create a multitude of device nodes that are used
         * by almost all programming language runtimes for various reasons
         *
         * /dev/urandom and /dev/random are particularly important
         *
         * If we can't create isolated device nodes, we'll fall back to
         * bind mounting the nodes resident on the host system
         */
        if (conty_rootfs_mkdev(&rootfs) != 0)
            goto err_notify_runtime;

        if (cc->cc_ns_new & CLONE_NEWPID) {
            /*
             * We need to mount procfs to avoid leaking process information
             * from the host. This can only be done (and needs to be done)
             * if the caller has requested a new pid namespace.
             * Since access control to /proc is regulated by the
             * superordinate user namespace of the pid namespace that mounted
             * proc, we would not be able to mount proc if the user hadn't
             * requested a new pid namespace
             */
            if (conty_rootfs_mount_proc(&rootfs) != 0)
                goto err_notify_runtime;
        }

        if (cc->cc_ns_new & CLONE_NEWNET) {
            /*
             * Same thing as proc but applies to the network namespace and sysfs
             */
            if (conty_rootfs_mount_sys(&rootfs) != 0)
                goto err_notify_runtime;
        }
    }

    if (cc->cc_ns_new & CLONE_NEWUTS) {
        /*
         * Caller has requested a new UTS namespace, i,e they probably
         * want to change the hostname inside the container
         */
        if (conf->oc_hostname) {
            if (sethostname(conf->oc_hostname, strlen(conf->oc_hostname)) != 0)
                goto err_notify_runtime;
        }
    }

    /*
     * At this point, we've successfully created the container environment,
     * we just need the runtime to confirm that
     * we should continue operating by sending us a creation event.
     *
     * We block indefinitely until we receive that event or an error
     * signifying that we should stop everything
     */
    if (conty_sync_await_runtime(cc->cc_syncfds, EVENT_CONT_CREATE) != 0)
        goto err_out;

    if (cc->cc_ns_new & CLONE_NEWNS) {
        /*
         * The runtime told us to move forward, so we replace the old
         * root filesystem image with the new one
         */
        if (conty_rootfs_pivot(&rootfs) != 0)
            goto err_notify_runtime;
    }

    /*
     * Alright, we've pivoted into the new environment.
     * What's left is for the runtime to instructs us to
     * actually execute the user-defined binary via an EVENT_START
     */
    if (conty_sync_runtime(cc->cc_syncfds, EVENT_CONT_CREATED) != 0)
        goto err_out;

    /*
     * We received the start request, so chdir into the current working
     * directory of the user application and call execvp;
     */
    if (chdir(proc->oproc_cwd) != 0)
        goto err_notify_runtime;

    /*
     * Finally, run that bad boy
     */
    execve(proc->oproc_argv[0], proc->oproc_argv, proc->oproc_envp);

err_notify_runtime:
    conty_sync_wake_runtime(cc->cc_syncfds, EVENT_ERROR);
err_out:
    return -1;
}

static int ns_sharer(void *arg)
{
    struct conty_container *cc = (struct conty_container *) arg;

    for (conty_ns_t ns = 0; ns < CONTY_NS_LEN; ns++) {
        FD_RESOURCE int nsfd = -EBADF;

        nsfd = cc->cc_ns_fds[ns];
        if (nsfd >= 0 && (conty_ns_set(nsfd, 0) < 0))
            return log_error_ret(-1, "cannot join namespace");
    }

    /*
     * We set the CLONE_PARENT flag to ensure that the runtime becomes
     * the parent process of the container, and not this intermediate process
     * which will exit.
     */
    unsigned long flags = cc->cc_ns_new | CLONE_PARENT | CLONE_PIDFD;
    cc->cc_pid = clone3_cb(container_entrypoint, arg, flags, &cc->cc_pollfd);

    if (cc->cc_pid < 0)
        return log_error_ret(-1, "cannot spawn container");

    return 0;
}

static int init_namespaces(struct conty_container *cc)
{
    int ns, fd;
    unsigned long ns_set = 0, flag;
    struct oci_namespace *ons_cur, *ons_tmp;
    struct oci_namespaces *namespaces = &cc->cc_conf->oc_namespaces;

    cc->cc_ns_has_fds = 0;
    memset(cc->cc_ns_fds, -EBADF, CONTY_NS_LEN * sizeof(int));

    SLIST_FOREACH_SAFE(ons_cur, namespaces, ons_next, ons_tmp) {
        ns = conty_ns_from_str(ons_cur->ons_type);
        if (ns < 0)
            return log_error_ret(-EINVAL, "unsupported namespace");

        flag = conty_ns_flags[ns];
        if (ns_set & flag) {
            LOG_WARN("ignoring duplicate %s namespace", ons_cur->ons_type);
            continue;
        }

        if (!ons_cur->ons_path)
            cc->cc_ns_new |= flag;
        else {
            fd = open(ons_cur->ons_path, O_RDONLY | O_CLOEXEC);
            if (fd < 0)
                return log_error_ret(-errno, "cannot open namespace");

            cc->cc_ns_fds[ns] = fd;
            cc->cc_ns_has_fds = 1;
        }

        ns_set |= flag;
    }

    return 0;
}