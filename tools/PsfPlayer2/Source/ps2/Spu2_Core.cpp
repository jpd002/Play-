#include <boost/lexical_cast.hpp>
#include "Spu2_Core.h"
#include "Log.h"

#define LOG_NAME_PREFIX ("spu2_core_")

using namespace PS2::Spu2;
using namespace std;
using namespace Framework;
using namespace boost;

CCore::CCore(unsigned int coreId, uint32 baseAddress) :
m_coreId(coreId),
m_baseAddress(baseAddress)
{
	m_logName = LOG_NAME_PREFIX + lexical_cast<string>(m_coreId);
}

CCore::~CCore()
{
    
}

uint16 CCore::ReadRegister(uint32 address)
{
    address -= m_baseAddress;
    uint16 result = 0;
    switch(address)
    {
    case STATX:
        result = 0x0000;
        break;
    default:
        break;
    }
#ifdef _DEBUG
    LogRead(address);
#endif
    return result;
}

void CCore::WriteRegister(uint32 address, uint16 value)
{
    address -= m_baseAddress;
	LogWrite(address, value);
}

void CCore::LogRead(uint32 address)
{
	const char* logName = m_logName.c_str();
    switch(address)
    {
    case STATX:
		CLog::GetInstance().Print(logName, "= STATX\r\n");
        break;
    default:
		CLog::GetInstance().Print(logName, "Read an unknown register 0x%0.4X.\r\n", address);
        break;
    }
}

void CCore::LogWrite(uint32 address, uint16 value)
{
	const char* logName = m_logName.c_str();
    switch(address)
    {
    default:
		CLog::GetInstance().Print(logName, "Write 0x%0.4X to an unknown register 0x%0.4X.\r\n", value, address);
        break;
    }
}
