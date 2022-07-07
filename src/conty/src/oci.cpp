#include <utility>

#include <conty/oci.h>

#include <nlohmann/json.hpp>

namespace oci {
    using json = nlohmann::json;

    /* Small utility to deserialize a container that has a default initializer */
    template<class T>
    void default_value(const json& j, const char *key, T& out)
    {
        out = std::move(j.value<T>(key, T{}));
    }

    void from_json(const json& j, hook& h)
    {
        j.at("path").get_to(h.path);
        default_value(j, "args", h.args);
        default_value(j, "env", h.env);
        default_value(j, "timeout", h.timeout);
    }
   
    void from_json(const json& j, container_hooks& h)
    {
        default_value(j, "prestart", h.on_create_rt_depr);
        default_value(j, "createRuntime", h.on_create_rt);
        default_value(j, "startContainer", h.on_start_cont);
        default_value(j, "poststart", h.on_running_cont);
        default_value(j, "poststop", h.on_stopped_cont);
    }
    
    void from_json(const json& j, specification& s)
    {
        j.at("ociVersion").get_to(s.version);
        default_value(j, "hooks", s.hooks);
    }

    specification specification::from_json(const std::string& json_string)
    {
        return json::parse(json_string);
    }
};
    
