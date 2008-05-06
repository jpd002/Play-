#include "Spu.h"
#include "Log.h"

#define LOG_NAME ("spu")

CSpu::CSpu()
{
	Reset();
}

CSpu::~CSpu()
{

}

void CSpu::Reset()
{
	m_statusLow = 0;
	m_statusHigh = 0;
}

uint16 CSpu::ReadRegister(uint32 address)
{
#ifdef _DEBUG
	DisassembleRead(address);
#endif
	return 0;
}

void CSpu::WriteRegister(uint32 address, uint16 value)
{
#ifdef _DEBUG
	DisassembleWrite(address, value);
#endif
}

void CSpu::DisassembleRead(uint32 address)
{
	if(address >= SPU_GENERAL_BASE)
	{
		switch(address)
		{
		default:
			CLog::GetInstance().Print(LOG_NAME, "Read an unknown register (0x%0.8X).\r\n", address);
			break;
		}
	}
	else
	{
		unsigned int channel = (address - SPU_BEGIN) / 0x10;
		unsigned int registerId = address & 0x0F;
		CLog::GetInstance().Print(LOG_NAME, "CH%i : Read an unknown register (0x%X).\r\n", channel, registerId);
	}
}

void CSpu::DisassembleWrite(uint32 address, uint16 value)
{
	if(address >= SPU_GENERAL_BASE)
	{
		switch(address)
		{
		default:
			CLog::GetInstance().Print(LOG_NAME, "Write to an unknown register (0x%0.8X, 0x%0.4X).\r\n", address, value);
			break;
		}
	}
	else
	{
		unsigned int channel = (address - SPU_BEGIN) / 0x10;
		unsigned int registerId = address & 0x0F;
		CLog::GetInstance().Print(LOG_NAME, "CH%i : Read an unknown register (0x%X).\r\n", channel, registerId);
	}
}
