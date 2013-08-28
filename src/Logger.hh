#pragma once

// ridiculously thin wrapper around syslog

#include <string>
#include <syslog.h>

struct Logger
{
    Logger(const std::string &name, int log_facility=LOG_LOCAL1);

    // default priority is LOG_NOTICE
    Logger &log(const char *format, ...);
    Logger &log(int priority, const char *format, ...);

private:
    std::string name;
    int log_facility;
};
