#pragma once

#include <memory>
#include <string>
#include <cstdarg>
#include <iostream>
#include <spdlog/spdlog.h>

namespace KCP {
class Logger {
public:
    static Logger& getLogInstance();
public:
    std::shared_ptr<spdlog::logger> log() { return logger_; }

    void trace(const char *filename,const char *Func,const int Line,const char * Format, ... );
    void debug(const char *filename,const char *Func,const int Line,const char * Format, ... );
    void info(const char *filename,const char *Func,const int Line,const char * Format, ... );
    void warn(const char *filename,const char *Func,const int Line,const char * Format, ... );
    void error(const char *filename,const char *Func,const int Line,const char * Format, ... );
    void critical(const char *filename,const char *Func,const int Line,const char * Format, ... );
private:
    // 创建日志实例
    Logger();
    ~Logger();

    // 从配置文件中加载日志配置
    void loadConfig();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
private:
    std::shared_ptr<spdlog::logger> logger_;    // spdlog::logger的实例
    std::string log_name_;    // 日志名称
    std::string file_name_;    // 日志文件名
    int max_files_;    // 日志文件最大个数 
    int max_size_;     // 日志文件最大大小 5MB
    int log_level_;    // 日志级别
};


#define Log_Instance KCP::Logger::getLogInstance()

#define  __FILENAME__ (strrchr(__FILE__, '/')?(strrchr(__FILE__, '/') + 1 ):__FILE__)
#define log_trace(...) Log_Instance.trace(__FILENAME__, __func__, __LINE__, __VA_ARGS__)
#define log_debug(...) Log_Instance.debug(__FILENAME__, __func__, __LINE__, __VA_ARGS__)
#define log_info(...) Log_Instance.info(__FILENAME__, __func__, __LINE__, __VA_ARGS__)
#define log_warn(...) Log_Instance.warn(__FILENAME__, __func__, __LINE__, __VA_ARGS__)
#define log_error(...) Log_Instance.error(__FILENAME__, __func__, __LINE__, __VA_ARGS__)
#define log_critical(...) Log_Instance.critical(__FILENAME__, __func__, __LINE__, __VA_ARGS__)

};