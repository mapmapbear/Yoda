#pragma once
#include "spdlog/spdlog.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/msvc_sink.h>

namespace Yoda {
    class Logger {
    public:
        static Logger& get_singleton();
        
        std::shared_ptr<spdlog::logger> singletonLogger;
    private:
        Logger();
    };
}