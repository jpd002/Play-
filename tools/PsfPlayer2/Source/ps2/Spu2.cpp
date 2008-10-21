#include "Spu2.h"
#include "Log.h"
#include "lexical_cast_ex.h"

#define LOG_NAME ("spu2")

using namespace PS2;
using namespace PS2::Spu2;
using namespace std;
using namespace Framework;

#define CORE_BASE (0x100000)

CSpu2::CSpu2(uint32 baseAddress) :
m_baseAddress(baseAddress)
{
    m_cores.push_back(CCore(0, CORE_BASE + 0x000));
    m_cores.push_back(CCore(1, CORE_BASE + 0x400));
}

CSpu2::~CSpu2()
{

}

uint32 CSpu2::ReadRegister(uint32 address)
{
    address -= m_baseAddress;
    if(address > CORE_BASE)
    {
        unsigned int coreId = (address - CORE_BASE) >> 10;
        if(coreId >= m_cores.size())
        {
            throw runtime_error("Invalid core identification.");
        }
        CCore& core(m_cores[coreId]);
        return core.ReadRegister(address);
    }
    else
    {
        LogRead(address);
    }
    return 0;
}

uint32 CSpu2::WriteRegister(uint32 address, uint32 value)
{
	address -= m_baseAddress;
	if(address > CORE_BASE)
	{
        unsigned int coreId = (address - CORE_BASE) >> 10;
        if(coreId >= m_cores.size())
        {
            throw runtime_error("Invalid core identification.");
        }
        CCore& core(m_cores[coreId]);
        core.WriteRegister(address, static_cast<uint16>(value));
	}
	else
	{
		LogWrite(address, value);
	}
	return 0;
}

void CSpu2::LogRead(uint32 address)
{
	switch(address)
	{
	default:
		CLog::GetInstance().Print(LOG_NAME, "Read an unknown register 0x%0.8X.\r\n", address);
		break;
	}
}

void CSpu2::LogWrite(uint32 address, uint32 value)
{
	switch(address)
	{
	default:
		CLog::GetInstance().Print(LOG_NAME, "Wrote 0x%0.8X to unknown register 0x%0.8X.\r\n", value, address);
		break;
	}
}
