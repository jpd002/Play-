#include "lexical_cast_ex.h"
#include "Spu2_Core.h"
#include "Log.h"

using namespace Spu2;
using namespace std;
using namespace Framework;

CCore::CCore(unsigned int coreId, uint32 baseAddress) :
m_coreId(coreId),
m_baseAddress(baseAddress)
{

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

void CCore::LogRead(uint32 address)
{
    string readValue;
    switch(address)
    {
    case STATX:
        readValue = "STATX";
        break;
    default:
        readValue = "(Unknown @ 0x" + lexical_cast_hex<string>(address, 4) + ")";
        break;
    }
    CLog::GetInstance().Print("spu2_core", "Core %i read : %s.\r\n", m_coreId, readValue.c_str());
}
