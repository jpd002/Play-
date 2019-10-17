#pragma once

#include <string>
#include <map>
#include "filesystem_def.h"
#include "StdStream.h"
#include "Singleton.h"

class CLog : public CSingleton<CLog>
{
public:
	CLog();
	virtual ~CLog() = default;

	void Print(const char*, const char*, ...);
	void Warn(const char*, const char*, ...);

private:
	typedef std::map<std::string, Framework::CStdStream> LogMapType;

	Framework::CStdStream& GetLog(const char*);

	fs::path m_logBasePath;
	LogMapType m_logs;
	bool m_showPrints = false;
};
