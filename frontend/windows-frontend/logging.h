#ifndef __LOGGING_H__
#define __LOGGING_H__
#include <string>
#include <vector>
#include <cstdint>
#include <mutex>

class Timestamp
{
public:
    Timestamp() = default;
    Timestamp(int64_t us) : _us_since_epoch(us) { }
public:
    static Timestamp now();
    std::string YYYY_MM_DD_HH_MM_SS_UUUUUU() const;
private:
    int64_t _us_since_epoch = 0;
};

class Logger
{
public:
    struct LogEntry {
        int line;
        const std::string& filename;
        const std::string& function;
    };

    enum {
        LOG_LEVEL_TRACE,
        LOG_LEVEL_DEBUG,
        LOG_LEVEL_INFO,
        LOG_LEVEL_WARN,
        LOG_LEVEL_ERROR,
        LOG_LEVEL_FATAL,
    };
public:
    Logger();
public:
    template<typename... Args>
    void trace(const LogEntry& entry, Args&&... args)
    {
        if (_level <= LOG_LEVEL_TRACE)
            log(LOG_LEVEL_TRACE, formatString(std::forward<Args>(args)...), entry);
    } 

    template<typename... Args>
    void debug(Args&&... args)
    {
        if (_level <= LOG_LEVEL_DEBUG)
            log(LOG_LEVEL_DEBUG, formatString(std::forward<Args>(args)...));
    } 

    template<typename... Args>
    void info(Args&&... args)
    {
        if (_level <= LOG_LEVEL_INFO)
            log(LOG_LEVEL_INFO, formatString(std::forward<Args>(args)...));
    } 

    template<typename... Args>
    void warn(const LogEntry& entry, Args&&... args)
    {
        if (_level <= LOG_LEVEL_WARN)
            log(LOG_LEVEL_WARN, formatString(std::forward<Args>(args)...), entry);
    } 

    template<typename... Args>
    void error(const LogEntry& entry, Args&&... args)
    {
        if (_level <= LOG_LEVEL_ERROR)
            log(LOG_LEVEL_ERROR, formatString(std::forward<Args>(args)...), entry);
    } 

    template<typename... Args>
    void fatal(const LogEntry& entry, Args&&... args)
    {
        if (_level <= LOG_LEVEL_FATAL)
        {
            log(LOG_LEVEL_FATAL, formatString(std::forward<Args>(args)...), entry); 
            std::abort();
        }
    } 
private:
    void log(int level, const std::string& msg, const LogEntry& entry);
    void log(int level, const std::string& msg);

    template<typename... Args>
    std::string formatString(const char* fmt, Args&&... args)
    {
        thread_local static char buf[1024]; // stack
        int len = std::snprintf(nullptr, 0, fmt, args...);
        if (len + 1 < sizeof(buf))
        {
            std::snprintf(buf, 1024, fmt, args...);
            return buf;
        }
        else 
        {
            std::vector<char> vec(len); // heap
            std::snprintf(vec.data(), len, fmt, args...);
            return std::string(vec.begin(), vec.end());
        }
        return "";
    }
private:
    int _level = LOG_LEVEL_TRACE;
    std::mutex _mutex;
};


extern Logger __logger;

#define LOG_TRACE(...) \
    __logger.trace(Logger::LogEntry{__LINE__, __FILE__, __func__}, __VA_ARGS__)

#define LOG_DEBUG(...) \
    __logger.debug(__VA_ARGS__)

#define LOG_INFO(...) \
    __logger.info(__VA_ARGS__)

#define LOG_WARN(...) \
    __logger.warn(Logger::LogEntry{__LINE__, __FILE__, __func__}, __VA_ARGS__)

#define LOG_ERROR(...) \
    __logger.error(Logger::LogEntry{__LINE__, __FILE__, __func__}, __VA_ARGS__)

#define LOG_FATAL(...) \
    __logger.fatal(Logger::LogEntry{__LINE__, __FILE__, __func__}, __VA_ARGS__)

#endif