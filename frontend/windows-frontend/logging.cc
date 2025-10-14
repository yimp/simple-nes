#include "logging.h"
#include <iostream>
#include <sstream>
#include <thread>

#define _USING_MSVC
#if defined(_USING_MSVC)
#include <windows.h>
#elif defined(_USING_MINGW)
#include <sys/time.h>
#endif

Timestamp Timestamp::now()
{
#define _USING_MSVC
#if defined(_USING_MSVC)
    FILETIME ft;
    ::GetSystemTimeAsFileTime(&ft);
    int64_t ts = ((int64_t)ft.dwHighDateTime << 32 | ft.dwLowDateTime);
    return Timestamp((ts - 116444736000000000LL) / 10);
#elif defined(_USING_MINGW)
    timeval tv;
    ::gettimeofday(&tv, nullptr);
    return Timestamp(tv.tv_sec * 1000000ull + tv.tv_usec);
#endif
}

std::string Timestamp::YYYY_MM_DD_HH_MM_SS_UUUUUU() const
{
    tm tm_time;
    time_t seconds = _us_since_epoch / 1000000ull;
    int64_t us = _us_since_epoch % 1000000ull;

    ::gmtime_s(&tm_time, &seconds);

    char buf[64];
    std::snprintf(buf, 64, "%04d-%02d-%02d %02d:%02d:%02d.%06d"
        , tm_time.tm_year + 1900, tm_time.tm_mon, tm_time.tm_mday, tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec, us
    );
    return buf; 
}

namespace logger_ctx
{
int pid = 0;
std::string pid_string;
thread_local int tid;
thread_local std::string tid_string;
thread_local std::string logger_string;
const std::string& get_ctx_string()
{
    if (!logger_string.empty())
        return logger_string;
    {
        std::ostringstream oss;
        oss << std::this_thread::get_id();
        tid_string = oss.str();
        logger_string = "[" + pid_string + ":" + tid_string + "]";
    }
    return logger_string;
}
};

Logger __logger;

Logger::Logger()
{
    logger_ctx::pid        = ::getpid();
    logger_ctx::pid_string = std::to_string(logger_ctx::pid);
}

static const char* __level_label[] = {
    "[TRACE]",
    "[DEBUG]",
    "[INFO]",
    "[WARN]",
    "[ERROR]",
    "[FATAL]",
};

// YYYY-MM-DD HH:MM:SS.UUUUUU [pid:tid][level] msg. FILE:LINE:FUNC
void Logger::log(int level, const std::string& msg, const LogEntry& entry)
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::cout << Timestamp::now().YYYY_MM_DD_HH_MM_SS_UUUUUU() << " " << logger_ctx::get_ctx_string() << __level_label[level]
        << " " <<  msg << " " << entry.filename << ":" << entry.line << ":" << entry.function << std::endl;
}

void Logger::log(int level, const std::string& msg)
{
    std::lock_guard<std::mutex> lock(_mutex);
    std::cout << Timestamp::now().YYYY_MM_DD_HH_MM_SS_UUUUUU() << " " << logger_ctx::get_ctx_string() << __level_label[level]
        << " " <<  msg << " " << std::endl;
}