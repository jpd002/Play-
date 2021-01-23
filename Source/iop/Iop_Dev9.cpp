#include "Iop_Dev9.h"
#include "Log.h"

#define LOG_NAME ("iop_dev9")

using namespace Iop;

uint32 CDev9::ReadRegister(uint32 address)
{
	uint32 result = 0;
	switch(address)
	{
	case REG_REV:
		//Report expansion bay
		result = 0x30;
		break;
	}
	LogRead(address);
	return result;
}

void CDev9::WriteRegister(uint32 address, uint32 value)
{
	LogWrite(address, value);
}

void CDev9::LogRead(uint32 address)
{
	switch(address)
	{
	case REG_REV:
		CLog::GetInstance().Print(LOG_NAME, "= REG_REV\r\n");
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Read an unknown register 0x%08X.\r\n", address);
		break;
	}
}

void CDev9::LogWrite(uint32 address, uint32 value)
{
	switch(address)
	{
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Wrote 0x%08X to an unknown register 0x%08X.\r\n", value, address);
		break;
	}
}
