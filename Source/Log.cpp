#include <stdarg.h>
#include <time.h>
#include "Log.h"
#include "AppConfig.h"
#include "PathUtils.h"
#include "StdStreamUtils.h"

#define LOG_PATH "logs"

#define PREF_LOG_SHOWPRINTS "log.showprints"

CLog::CLog()
{
#ifndef DISABLE_LOGGING
	m_logBasePath = CAppConfig::GetBasePath() / LOG_PATH;
	Framework::PathUtils::EnsurePathExists(m_logBasePath);
	CAppConfig::GetInstance().RegisterPreferenceBoolean(PREF_LOG_SHOWPRINTS, false);
	m_showPrints = CAppConfig::GetInstance().GetPreferenceBoolean(PREF_LOG_SHOWPRINTS);
#endif
}

void CLog::Print(const char* logName, const char* format, ...)
{
#if defined(_DEBUG) && !defined(DISABLE_LOGGING)
	if(!m_showPrints) return;
	auto& logStream(GetLog(logName));
	va_list args;
	va_start(args, format);
	vfprintf(logStream, format, args);
	va_end(args);
	logStream.Flush();
#endif
}

void CLog::Warn(const char* logName, const char* format, ...)
{
#if defined(_DEBUG) && !defined(DISABLE_LOGGING)
	auto& logStream(GetLog(logName));
	va_list args;
	va_start(args, format);
	vfprintf(logStream, format, args);
	va_end(args);
	logStream.Flush();
#endif
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
