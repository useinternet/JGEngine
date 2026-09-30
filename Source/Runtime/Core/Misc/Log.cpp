#include "PCH/PCH.h"
#include "Log.h"

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/base_sink.h"
#include <deque>

// DevConsole 로그 뷰용 최근 줄 버퍼(Info 이상).
// 로그 호출 안에서 돌므로 엔진 풀·JG_LOG 를 쓰지 않는다(std 컨테이너만). 풀 안에서 찍는 로그도 여기로 온다.
// spdlog 1.10 의 ringbuffer_sink 는 final 이라 변경 카운터(serial)를 붙일 수 없어 따로 둔다.
class PRecentLogSink : public spdlog::sinks::base_sink<std::mutex>
{
public:
	struct HEntry
	{
		spdlog::level::level_enum Level;
		std::string               Text;
	};

	static constexpr size_t MaxLines = 1024;

	uint64 GetSerial() const
	{
		return _serial.load(std::memory_order_acquire);
	}

	void CopyTo(std::vector<HEntry>& OutEntries)
	{
		std::lock_guard<std::mutex> lock(mutex_);
		OutEntries.assign(_lines.begin(), _lines.end());
	}

protected:
	virtual void sink_it_(const spdlog::details::log_msg& InMessage) override
	{
		_lines.push_back(HEntry{ InMessage.level, std::string(InMessage.payload.data(), InMessage.payload.size()) });
		if (_lines.size() > MaxLines)
		{
			_lines.pop_front();
		}

		_serial.fetch_add(1, std::memory_order_release);
	}

	virtual void flush_() override
	{
	}

private:
	std::deque<HEntry>  _lines;
	std::atomic<uint64> _serial{ 0 };
};

namespace
{
	ELogLevel toLogLevel(spdlog::level::level_enum InLevel)
	{
		switch (InLevel)
		{
		case spdlog::level::trace:    return ELogLevel::Trace;
		case spdlog::level::debug:    return ELogLevel::Debug;
		case spdlog::level::warn:     return ELogLevel::Warning;
		case spdlog::level::err:      return ELogLevel::Error;
		case spdlog::level::critical: return ELogLevel::Critical;
		default:                      return ELogLevel::Info;
		}
	}
}

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

	// DevConsole 로그 뷰가 읽는 최근 줄 버퍼 (DevConsole_TODO 2-2)
	_recentSink = std::make_shared<PRecentLogSink>();
	_recentSink->set_level(spdlog::level::info);
	_logger->sinks().push_back(_recentSink);
}

GLogGlobalSystem::~GLogGlobalSystem()
{
	_logger.reset();
	_recentSink.reset();
	spdlog::shutdown();
}

uint64 GLogGlobalSystem::GetLogSerial() const
{
	if (_recentSink == nullptr)
	{
		return 0;
	}

	return _recentSink->GetSerial();
}

void GLogGlobalSystem::GetRecentLogs(HList<HLogLine>& OutLines) const
{
	OutLines.clear();
	if (_recentSink == nullptr)
	{
		return;
	}

	std::vector<PRecentLogSink::HEntry> entries;
	_recentSink->CopyTo(entries);

	OutLines.reserve(entries.size());
	for (const PRecentLogSink::HEntry& entry : entries)
	{
		HLogLine line;
		line.Level = toLogLevel(entry.Level);
		line.Text  = entry.Text.c_str();
		OutLines.push_back(line);
	}
}


