#include "logger.hpp"
#include <spdlog/sinks/rotating_file_sink.h>
#include "rapidjson/document.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <errno.h>
#include <string.h>


namespace KCP {

// 根据实际项目中的日志大小设置
#define MAX_ONELOG_LENGTH 1024

Logger& Logger::getLogInstance() {
    static Logger s_instance;
    return s_instance;
}

Logger::Logger() : 
            log_name_("server"),  file_name_("logs/kcp_server.log"),
            max_files_(5), max_size_(1024 * 1024 * 10), log_level_(spdlog::level::info) {
    loadConfig();
    logger_ = spdlog::rotating_logger_mt(log_name_, file_name_, max_size_, max_files_);
    logger_->set_level(spdlog::level::level_enum(log_level_));   
    logger_->flush_on(spdlog::level::info); // level 级别以上的日志立即刷新到文件中
    logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e][%L] %v");
}

Logger::~Logger() {
    spdlog::drop_all();
}

void Logger::loadConfig() {
    std::ifstream ifs("config.json");
    if (!ifs) {
        std::cerr << "Failed to open config file with errno " << errno << " " << strerror(errno) << std::endl;
        return;
    }
    std::stringstream buffer;
    buffer << ifs.rdbuf();
    ifs.close();
    std::string json_str = buffer.str();

    rapidjson::Document doc;
    doc.Parse(json_str.c_str());
    if (doc.HasParseError()) {
        std::cerr << "Failed to parse JSON." << std::endl;
        return;
    }
    
    if (doc.HasMember("log") && doc["log"].IsObject()) {
        for (auto& member : doc["log"].GetObject()) {
            if (std::string("log_name") == member.name.GetString() && member.value.IsString()) {
                log_name_ = member.value.GetString();
            } else if (std::string("log_path") == member.name.GetString() && member.value.IsString()) {
                file_name_ = member.value.GetString();
            } else if (std::string("max_size") == member.name.GetString() && member.value.IsInt()) {
                max_size_ = member.value.GetInt();
            } else if (std::string("max_files") == member.name.GetString() && member.value.IsInt()) {
                max_files_ = member.value.GetInt();
            } else if (std::string("log_level") == member.name.GetString() && member.value.IsInt()) {
                log_level_ = member.value.GetInt();
            }
        }
    }

    std::cout << "log_name_: " << log_name_ << "\t" << "file_name_: " << file_name_ << "\t" << "max_size: " << max_size_ << "\t" << "max_files: " << max_files_ << "\t" << "log_level: " << log_level_ << std::endl;
    std::cout << "load config done." << std::endl;
}

void Logger::trace(const char *filename,const char *Func,const int Line,const char * Format, ... ) {
    va_list args;
    va_start(args, Format);
    char tmpbuf[MAX_ONELOG_LENGTH];
    vsnprintf(tmpbuf,MAX_ONELOG_LENGTH-1,Format,args); 
    va_end(args);
    logger_->trace("[{}:{}.{}] {}", filename ,Func, Line ,tmpbuf);
}
void Logger::debug(const char *filename,const char *Func,const int Line,const char * Format, ... ) {
    va_list args;
    va_start(args, Format);
    char tmpbuf[MAX_ONELOG_LENGTH];
    vsnprintf(tmpbuf,MAX_ONELOG_LENGTH-1,Format,args); 
    va_end(args);
    logger_->debug("[{}:{}.{}] {}", filename ,Func, Line ,tmpbuf);
}
void Logger::info(const char *filename,const char *Func,const int Line,const char * Format, ... ) {
    va_list args;
    va_start(args, Format);
    char tmpbuf[MAX_ONELOG_LENGTH];
    vsnprintf(tmpbuf,MAX_ONELOG_LENGTH-1,Format,args); 
    va_end(args);
    logger_->info("[{}:{}.{}] {}", filename ,Func, Line ,tmpbuf);
}
void Logger::warn(const char *filename,const char *Func,const int Line,const char * Format, ... ) {
    va_list args;
    va_start(args, Format);
    char tmpbuf[MAX_ONELOG_LENGTH];
    vsnprintf(tmpbuf,MAX_ONELOG_LENGTH-1,Format,args); 
    va_end(args);
    logger_->warn("[{}:{}.{}] {}", filename ,Func, Line ,tmpbuf);
}
void Logger::error(const char *filename,const char *Func,const int Line,const char * Format, ... ) {
    va_list args;
    va_start(args, Format);
    char tmpbuf[MAX_ONELOG_LENGTH];
    vsnprintf(tmpbuf,MAX_ONELOG_LENGTH-1,Format,args); 
    va_end(args);
    logger_->error("[{}:{}.{}] {}", filename ,Func, Line ,tmpbuf);
}
void Logger::critical(const char *filename,const char *Func,const int Line,const char * Format, ... ) {
    va_list args;
    va_start(args, Format);
    char tmpbuf[MAX_ONELOG_LENGTH];
    vsnprintf(tmpbuf,MAX_ONELOG_LENGTH-1,Format,args); 
    va_end(args);
    logger_->critical("[{}:{}.{}] {}", filename ,Func, Line ,tmpbuf);
}

};
