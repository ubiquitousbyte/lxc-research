#include <conty/conty.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <system_error>

#include <unordered_map>
#include <iostream>

/*
 * File descriptor that keeps a context alive.
 * If the file descriptor is closed and all processes inside the context
 * terminate, then the context gets deleted by the kernel.
 */
struct context_fd {
    int value;

    explicit context_fd(const char *path)
    {
        value = open(path, O_RDONLY | O_CLOEXEC);
        if (value == SENTINEL)
            throw std::system_error(errno, std::system_category());
    }

    /* Right now, context should not be copied */
    context_fd(const context_fd&) = delete;
    context_fd& operator=(const context_fd&) = delete;

    context_fd(context_fd&& other) noexcept
    {
        value = std::exchange(other.value, SENTINEL);
    }

    context_fd& operator=(context_fd&& other) noexcept
    {
        close(value);
        value = std::exchange(other.value, SENTINEL);
        return *this;
    }

    ~context_fd()
    {
        if (value != SENTINEL)
            close(value);
    }

private:
    constexpr static int SENTINEL = -1;
};

struct context::impl {
    using variant_table = typename std::unordered_map<context::variant, const char*>;

    impl(context_fd fd, ino_t ino, dev_t dev, context::variant v):
        fd{std::move(fd)}, inode{ino}, device{dev}, variant{v} {}

    context_fd                 fd;
    ino_t                      inode;
    dev_t                      device;
    context::variant           variant;
    const static variant_table variants;
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

context::context(impl&& ctx):
    ctx{std::make_unique<context::impl>(std::move(ctx))} {}

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

ino_t context::inode() const
{
    return this->ctx->inode;
}

void context::join()
{
    int rc = setns(this->ctx->fd.value, this->ctx->variant);
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

    context_fd fd{path};

    /* Fetch the device id and inode number uniquely identifying the context */
    struct stat s{};
    if (fstat(fd.value, &s) == -1)
        throw std::system_error(errno, std::system_category(), errmsg);

    return context{{std::move(fd), s.st_ino, s.st_dev, v}};
}

context context::current(enum variant v)
{
    return context::find(getpid(), v);
}

context::~context() = default;