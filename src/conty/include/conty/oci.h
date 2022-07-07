#include <string>
#include <vector>

namespace oci {

    /*
     * A hook represents a program that is run at a specific point in the 
     * lifecycle of a container. Hooks are used by container engines to 
     * configure the execution context of a container. 
     */
    struct hook {
        std::string path;
        std::vector<std::string> args;
        std::vector<std::string> env;
        int timeout;
    };

    /*
     * All hooks semantically grouped by a container's lifecycle event
     * We do not follow the naming used in the specification because 
     * it is confusing. 
     */
    struct container_hooks { 
        std::vector<hook> on_create_rt_depr;
        std::vector<hook> on_create_rt;
        std::vector<hook> on_create_cont;
        std::vector<hook> on_start_cont;
        std::vector<hook> on_running_cont;
        std::vector<hook> on_stopped_cont;
    };

    /*
     * Open Container Initiative Specification object
     */ 
    struct specification {
        std::string version;
        container_hooks hooks;

        /*
         * Creates a specification object from a json string
         */
        static specification from_json(const std::string& json_string);
    };
};

