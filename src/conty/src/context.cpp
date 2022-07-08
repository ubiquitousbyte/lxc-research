#include <conty/conty.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <system_error>

#include <unordered_map>

struct context::impl {
    using variant_table = typename std::unordered_map<context::variant, const char*>;
    /*
     * fd is a file descriptor pointing to the namespace file in the
     * proc filesystem. Keeping the file descriptor open instructs the kernel
     * to keep the namespace alive even if all processes in that namespace
     * have left it or have terminated.
     */
    int                        fd;

    ino_t                      inode;
    dev_t                      device;
    context::variant           variant;
    const static variant_table variants;

    ~impl()
    {
        /*
         * Release the file descriptor. If all processes in the context
         * have exited, the namespace will be released by the kernel
         */
        close(fd);
    }
};

/* Values must match file names in /proc/[pid]/ns */
const context::impl::variant_table context::impl::variants = {
        { context::variant::Cgroup, "cgroup" },
        { context::variant::Net, "net" },
        { context::variant::Uts, "uts" },
        { context::variant::User, "user" },
        { context::variant::Pid, "pid" },
        { context::variant::Mount, "mnt" },
        { context::variant::Ipc, "ipc" },
};

context::context(std::unique_ptr<context::impl> ctx): ctx{std::move(ctx)} {}

bool context::operator==(const context &o) const
{
    return this->ctx->inode == o.ctx->inode &&
        this->ctx->device == o.ctx->device;
}

std::ostream& operator<<(std::ostream &stream, const context &ctx)
{
    const char *variant = context::impl::variants.at(ctx.ctx->variant);
    return stream << variant << ":[" << ctx.ctx->inode << ']';
}

enum context::variant context::type() const
{
    return this->ctx->variant;
}

void context::join()
{
    int rc = setns(this->ctx->fd, this->ctx->variant);
    if (rc != 0) {
        const char *what = "could not join context";
        throw std::system_error(errno, std::system_category(), what);
    }
}

void context::detach(int variants)
{
    int rc = unshare(variants & ~(CLONE_FILES | CLONE_FS | CLONE_SYSVSEM));
    if (rc != 0) {
        const char *what = "could not detach context";
        throw std::system_error(errno, std::system_category(), what);
    }
}

context context::find(pid_t pid, enum variant v)
{
    char path[sizeof("/proc/xxxxxxxxxxxxxxxxxxxx/ns/cgroup")];
    const char *errmsg = "Could not find context";
    const char *variant;

    try {
        variant = context::impl::variants.at(v);
    } catch (const std::out_of_range& e) {
        throw std::system_error(ENOENT, std::system_category(), errmsg);
    }

    /* Construct the file path  */
    std::snprintf(path, sizeof(path), "/proc/%d/ns/%s", pid, variant);

    /*
     * We open the file in read-only mode and make sure to atomically close
     * it whenever this process or any child calls exec
     */
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd == -1)
        throw std::system_error(errno, std::system_category(), errmsg);

    /* Fetch the device id and inode number uniquely identifying the context */
    struct stat s{};
    if (fstat(fd, &s) == -1)
        throw std::system_error(errno, std::system_category(), errmsg);

    return context{
        std::make_unique<context::impl>(context::impl{fd, s.st_ino, s.st_dev, v})
    };
}

context context::current(enum variant v)
{
    return context::find(getpid(), v);
}

context::~context() = default;