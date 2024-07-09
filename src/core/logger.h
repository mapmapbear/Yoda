#pragma once
#include "spdlog/spdlog.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/msvc_sink.h>

#define LOG_ERROR Logger::get_singleton().singletonLogger->error
#define LOG_DEBUG Logger::get_singleton().singletonLogger->debug
#define LOG_INFO Logger::get_singleton().singletonLogger->info

namespace Yoda {
    class Logger {
    public:
        static Logger& get_singleton();
        
        std::shared_ptr<spdlog::logger> singletonLogger;
    private:
        Logger();
    };
}