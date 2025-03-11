#include "Log.h"
#include <set>
#include <cstdarg>
#include "AppConfig.h"
#include "PathUtils.h"
#include "StdStreamUtils.h"

#define LOG_PATH "logs"

#define PREF_LOG_SHOWPRINTS "log.showprints"

#if LOGGING_ENABLED

// clang-format off
static const std::set<std::string, std::less<>> g_allowedLogs =
{
	//"iop_mcserv",
};
// clang-format on

CLog::CLog()
{
	m_logBasePath = CAppConfig::GetInstance().GetBasePath() / LOG_PATH;
	Framework::PathUtils::EnsurePathExists(m_logBasePath);
	CAppConfig::GetInstance().RegisterPreferenceBoolean(PREF_LOG_SHOWPRINTS, false);
	m_showPrints = CAppConfig::GetInstance().GetPreferenceBoolean(PREF_LOG_SHOWPRINTS);
}

void CLog::Print(const char* logName, const char* format, ...)
{
	if(!m_showPrints && !g_allowedLogs.count(logName)) return;
	auto& logStream(GetLog(logName));
	va_list args;
	va_start(args, format);
	vfprintf(logStream, format, args);
	va_end(args);
	logStream.Flush();
}

void CLog::Warn(const char* logName, const char* format, ...)
{
	auto& logStream(GetLog(logName));
	va_list args;
	va_start(args, format);
	vfprintf(logStream, format, args);
	va_end(args);
	logStream.Flush();
}

Framework::CStdStream& CLog::GetLog(const char* logName)
{
	auto logIterator(m_logs.find(logName));
	if(logIterator == std::end(m_logs))
	{
		auto logPath = m_logBasePath / (std::string(logName) + ".log");
		auto logStream = Framework::CreateOutputStdStream(logPath.native());
		m_logs[logName] = std::move(logStream);
		logIterator = m_logs.find(logName);
	}
	return logIterator->second;
}

#endif
