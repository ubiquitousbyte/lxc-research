#include <conty/conty.h>

#include <unistd.h>
#include <sys/stat.h>

/* Small utility that converts resource types to their string representations */
constexpr const char *type_to_string(enum resource_namespace::type t)
{
    switch (t) {
        case resource_namespace::type::Cgroup: return "cgroup";
        case resource_namespace::type::Ipc: return "ipc";
        case resource_namespace::type::Mount: return "mount";
        case resource_namespace::type::Net: return "net";
        case resource_namespace::type::Pid: return "pid";
        case resource_namespace::type::Time: return "time";
        case resource_namespace::type::User: return "user";
        case resource_namespace::type::Uts: return "uts";
        default: throw std::invalid_argument("unimplemented namespace type");
    }
}

resource_namespace::resource_namespace(dev_t dev, ino_t node,
                                       enum resource_namespace::type t) :
        device{dev}, node{node}, type{t} {}

bool resource_namespace::operator==(const resource_namespace& o) const
{
    return this->device == o.device && this->node == o.node;
}

std::ostream& operator<<(std::ostream& stream, const resource_namespace &ns)
{
    return stream << type_to_string(ns.type) << ':' << '[' << ns.node << ']';
}

enum resource_namespace::type resource_namespace::namespace_type() const
{
    return this->type;
}

ino_t resource_namespace::inode() const
{
    return this->node;
}

resource_namespace resource_namespace::from_proc(pid_t pid,
                                                 enum resource_namespace::type t)
{
    int rc;
    char buf[sizeof("/proc/xxxxxxxxxxxxxxxxxxxx/ns/cgroup")];
    const char *type = type_to_string(t);

    std::snprintf(buf, sizeof(buf), "/proc/%d/ns/%s", pid, type);

    struct stat metadata{};
    rc = stat(buf, &metadata);
    if (rc != 0) {
        const char *what = "namespace not found";
        throw std::system_error(errno, std::system_category(), what);
    }

    return resource_namespace{metadata.st_dev, metadata.st_ino, t};
}

resource_namespace resource_namespace::current(enum resource_namespace::type t)
{
    return resource_namespace::from_proc(getpid(), t);
}