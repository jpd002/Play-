#include "Spu2.h"
#include "Log.h"
#include "lexical_cast_ex.h"

using namespace Spu2;
using namespace std;
using namespace Framework;

CSpu2::CSpu2(uint32 baseAddress) :
m_baseAddress(baseAddress)
{
    m_cores.push_back(CCore(0, 0x100000));
    m_cores.push_back(CCore(1, 0x100400));
}

CSpu2::~CSpu2()
{

}

uint32 CSpu2::ReadRegister(uint32 address)
{
    address -= m_baseAddress;
    if(address > 0x100000)
    {
        unsigned int coreId = (address - 0x100000) >> 10;
        if(coreId >= m_cores.size())
        {
            throw runtime_error("Invalid core identification.");
        }
        CCore& core(m_cores[coreId]);
        return core.ReadRegister(address) | (core.ReadRegister(address + 2) << 16);
    }
    else
    {
        LogRead(address);
    }
    return 0;
}

void CSpu2::LogRead(uint32 address)
{
    string readValue;
    switch(address)
    {
    default:
        readValue = "(Unknown @ 0x" + lexical_cast_hex<string>(address, 4) + ")";
        break;
    }
    CLog::GetInstance().Print("spu2", "Read : %s.\r\n", readValue.c_str());
}
