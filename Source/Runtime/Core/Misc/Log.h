#pragma once
#pragma warning(disable : 26495 26498 26451 6285 4819 26437 26800)

#include "CoreDefines.h"
#include "CoreSystem.h"
#include "Memory/Memory.h"
#include "String/String.h"
#include "FileIO/FileHelper.h"
#include "spdlog/spdlog.h"

template<typename OStream>
OStream& operator<<(OStream& os, const PString& to_log)
{
	fmt::format_to(std::ostream_iterator<char>(os), "{0}", to_log.GetRawString());
	return os;
}

enum class ELogLevel
{
	Debug,
	Trace,
	Info,
	Warning,
	Error,
	Critical,
};

// 최근 로그 한 줄(DevConsole 로그 뷰). Text 는 "[Category]: 본문" 이다.
struct HLogLine
{
	ELogLevel Level = ELogLevel::Info;
	PString   Text;
};

class PRecentLogSink;

class GLogGlobalSystem : public GGlobalSystemInstance<GLogGlobalSystem>
{
	mutable std::shared_ptr<spdlog::logger> _logger;
	// Info 이상 최근 줄 버퍼. Log.cpp 에 정의한다.
	std::shared_ptr<PRecentLogSink> _recentSink;

public:
	GLogGlobalSystem();
	virtual ~GLogGlobalSystem();

public:
	// 최근 로그(Info 이상, 최대 1024줄). serial 은 줄이 추가될 때마다 늘어난다 → 바뀐 프레임에만 GetRecentLogs 로 복사한다.
	uint64 GetLogSerial() const;
	void   GetRecentLogs(HList<HLogLine>& OutLines) const;

public:
	template<class ...Args>
	void AddLog(const PString& category, ELogLevel loglevel, const PString& logText, Args&& ...args) const
	{
		PString log = PString::Format("[%s]: %s", category, logText);
		log = PString::Format(log, args...);

		switch (loglevel)
		{
		case ELogLevel::Debug:    _logger->debug(log.GetRawString());	 break;
		case ELogLevel::Trace:    _logger->trace(log.GetRawString());	 break;
		case ELogLevel::Info:     _logger->info(log.GetRawString());	 break;
		case ELogLevel::Warning:  _logger->warn(log.GetRawString());	 break;
		case ELogLevel::Error:	  _logger->error(log.GetRawString());	 break;
		case ELogLevel::Critical: _logger->critical(log.GetRawString()); break;
		}
	}
};


#define JG_LOG(Category, LogLevel, LogText, ...) ::GLogGlobalSystem::GetInstance().AddLog(#Category, LogLevel, LogText, __VA_ARGS__);