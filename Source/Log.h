#ifndef _LOG_H_
#define _LOG_H_

#include <string>
#include <map>
#include "StdStream.h"
#include "Singleton.h"

class CLog : public CSingleton<CLog>
{
public:
    void                        Print(const char*, const char*, ...);

private:
    friend class CSingleton<CLog>;
    typedef std::map<std::string, Framework::CStdStream*> LogMapType;

                                CLog();
    virtual                     ~CLog();

    Framework::CStdStream*      GetLog(const char*);

    LogMapType                  m_logs;
};

#endif
