#include <boost/lexical_cast.hpp>
#include "Spu2_Core.h"
#include "Log.h"

#define LOG_NAME_PREFIX ("spu2_core_")

using namespace PS2::Spu2;
using namespace std;
using namespace Framework;
using namespace boost;

CCore::CCore(unsigned int coreId) :
m_coreId(coreId)
{
	m_logName = LOG_NAME_PREFIX + lexical_cast<string>(m_coreId);
}

CCore::~CCore()
{
    
}

uint32 CCore::ReadRegister(uint32 address, uint32 value)
{
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

uint32 CCore::WriteRegister(uint32 address, uint32 value)
{
	LogWrite(address, value);
	return 0;
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
	case A_TSA_HI:
		CLog::GetInstance().Print(logName, "A_TSA_HI = 0x%0.4X\r\n", value);
		break;
	case A_TSA_LO:
		CLog::GetInstance().Print(logName, "A_TSA_LO = 0x%0.4X\r\n", value);
		break;
	case A_STD:
		CLog::GetInstance().Print(logName, "A_STD = 0x%0.4X\r\n", value);
		break;
    default:
		CLog::GetInstance().Print(logName, "Write 0x%0.4X to an unknown register 0x%0.4X.\r\n", value, address);
        break;
    }
}
