#include "Logger.hh"
#include <stdarg.h>

Logger::Logger(const std::string &name, int log_facility)
    : name(name)
    , log_facility(log_facility)
{
}

Logger &Logger::log(const char *format, ...)
{
    va_list myargs;
    va_start(myargs, format);
    vsyslog(LOG_MAKEPRI(log_facility, LOG_NOTICE), format, myargs);
    va_end(myargs);
    return *this;
}

Logger &Logger::log(int priority, const char *format, ...)
{
    va_list myargs;
    va_start(myargs, format);
    vsyslog(LOG_MAKEPRI(log_facility, priority), format, myargs);
    va_end(myargs);
    return *this;
}
