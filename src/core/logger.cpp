#include "core/logger.h"

namespace Yoda {
Logger &Logger::Logger::get_singleton() {
	static Logger *instance = new Logger();
	return *instance;
}

Logger::Logger() {
	singletonLogger = spdlog::stdout_color_mt("console");
	std::shared_ptr<spdlog::sinks::windebug_sink_st> msvc_sink = std::make_shared<spdlog::sinks::windebug_sink_st>(true);
	msvc_sink->set_level(spdlog::level::debug);
	singletonLogger->sinks().push_back(msvc_sink);
}
} //namespace Yoda