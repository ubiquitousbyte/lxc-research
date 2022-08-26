#include "tcplatency.h"
#include "vfslatency.h"
#include "rqlatency.h"

#include <unistd.h>

int main(int argc, char *argv[])
{
    struct bench_tcplatency_conf tcpconf = {
            .tcpl_duration = 120,
            .tcpl_milliseconds = 1,
            .tcpl_interval = 1,
            .tcpl_sink = "/home/nas/tcp-trial.csv"
    };

    struct bench_rqlatency_conf rqconf = {
            .rql_interval = 1,
            .rql_duration = 10,
            .rql_pid = 32526,
            .rql_sink = "/home/nas/rq-trial.csv"
    };

    struct bench_vfslatency_conf vfsconf = {
            .vfsl_interval = 1,
            .vfsl_duration = 120,
            .vfsl_pid = 0,
            .vfsl_sink = "/home/nas/vfs-trial.csv"
    };

    return bench_rqlatency_trace(&rqconf);
}