#include "log.h"

#include <pthread.h>

static const char *conty_log_levels[CONTY_LOG_FATAL+1] = {
        [CONTY_LOG_TRACE] = "TRACE",
        [CONTY_LOG_DEBUG] = "DEBUG",
        [CONTY_LOG_INFO]  = "INFO",
        [CONTY_LOG_WARN]  = "WARN",
        [CONTY_LOG_ERROR] = "ERROR",
        [CONTY_LOG_FATAL] = "FATAL",
};

#define CONTY_LOG_MAX_CALLBACKS 16

static struct {
    void        *udata;
    int          level;
    int          quiet;
} CONTY_LOGGER;


void conty_log_set_level(int level)
{
    CONTY_LOGGER.level = level;
}

void conty_log_set_quiet(int quiet)
{
    CONTY_LOGGER.quiet = quiet;
}

static void conty_log_stdout_cb(struct conty_log_event *event)
{
    fprintf(event->udata, "%-5s %s:%d: ",
            conty_log_levels[event->lvl], event->file, event->line);
    vfprintf(event->udata, event->fmt, event->varargs);
    fprintf(event->udata, "\n");
    fflush(event->udata);
}

static void conty_log_file_cb(struct conty_log_event *event)
{
    fprintf(event->udata, "%-5s %s:%d: ",
            conty_log_levels[event->lvl], event->file, event->line);
    vfprintf(event->udata, event->fmt, event->varargs);
    fprintf(event->udata, "\n");
    fflush(event->udata);
}

static void conty_log_event_init(struct conty_log_event *event, void *data)
{
    event->udata = data;
}

void conty_log(int level, const char *file, int line, const char *fmt, ...)
{
    struct conty_log_event event = {
            .fmt  = fmt,
            .file = file,
            .line = line,
            .lvl  = level
    };

    if (CONTY_LOGGER.quiet == 0 && level >= CONTY_LOGGER.level) {
        conty_log_event_init(&event, stderr);
        va_start(event.varargs, fmt);
        conty_log_stdout_cb(&event);
        va_end(event.varargs);
    }
}