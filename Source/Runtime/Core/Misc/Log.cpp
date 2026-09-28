#include "PCH/PCH.h"
#include "Log.h"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"

GLogGlobalSystem::GLogGlobalSystem()
{
	HFileHelper::WriteAllText("jg_log.txt", "");

	spdlog::set_pattern("%^[:%t:][%l]%v%$");
	_logger = spdlog::basic_logger_mt("JGLog", "jg_log.txt");


	_logger->set_level(spdlog::level::trace);
	// 경고 이상에서만 즉시 flush 한다. (Memory_TODO 1-1. 이전에는 trace 이상 = 모든 줄을 flush 해서 풀 할당마다 파일 I/O가 붙었다)
	// Info 이하는 spdlog 버퍼가 차거나 종료 시 쓰인다. 크래시 직전 Info 줄이 잘릴 수 있으므로 원인 추적 시에는 이 값을 trace 로 되돌려 빌드한다.
	_logger->flush_on(spdlog::level::warn);

	auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
	consoleSink->set_pattern("%^[:%t:][%l]%v%$");
	_logger->sinks().push_back(consoleSink);
}

GLogGlobalSystem::~GLogGlobalSystem()
{
	_logger.reset();
	spdlog::shutdown();
}


