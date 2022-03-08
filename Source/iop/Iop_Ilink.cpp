#include "Iop_Ilink.h"
#include "Log.h"

#define LOG_NAME ("iop_ilink")

using namespace Iop;

uint32 CIlink::ReadRegister(uint32 address)
{
	LogRead(address);
	return 0x08;
}

void CIlink::WriteRegister(uint32 address, uint32 value)
{
	LogWrite(address, value);
}

void CIlink::LogRead(uint32 address)
{
#define LOG_GET(registerId)                                           \
	case registerId:                                                  \
		CLog::GetInstance().Print(LOG_NAME, "= " #registerId "\r\n"); \
		break;

	switch(address)
	{
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Read an unknown register 0x%08X.\r\n", address);
		break;
	}
#undef LOG_GET
}

void CIlink::LogWrite(uint32 address, uint32 value)
{
#define LOG_SET(registerId)                                                      \
	case registerId:                                                             \
		CLog::GetInstance().Print(LOG_NAME, #registerId " = 0x%08X\r\n", value); \
		break;

	switch(address)
	{
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Wrote 0x%08X to an unknown register 0x%08X.\r\n", value, address);
		break;
	}
#undef LOG_SET
}
