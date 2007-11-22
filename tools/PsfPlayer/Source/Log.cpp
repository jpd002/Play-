#include <stdarg.h>
#include <time.h>
#include "Log.h"

#define LOG_PATH "./logs/"

using namespace Framework;
using namespace std;

CLog::CLog()
{

}

CLog::~CLog()
{

}

void CLog::Print(const char* logName, const char* format, ...)
{
    CStdStream* log(GetLog(logName));
    if(log == NULL) return;
    va_list args;
    va_start(args, format);
    vfprintf(*log, format, args);
    va_end(args);
    log->Flush();
}

CStdStream* CLog::GetLog(const char* logName)
{
    LogMapType::iterator logIterator(m_logs.find(logName));
    if(logIterator != m_logs.end())
    {
        return logIterator->second;
    }
    else
    {
        CStdStream* log = new CStdStream((LOG_PATH + string(logName) + ".log").c_str(), "wb");
        m_logs[logName] = log;
        return log;
    }
}
