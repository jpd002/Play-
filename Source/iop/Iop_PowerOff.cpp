#include "Iop_PowerOff.h"
#include "Log.h"
#include "Iop_SifMan.h"

#define LOG_NAME ("iop_poweroff")

using namespace Iop;

CPowerOff::CPowerOff(CSifMan& sifMan)
{
	sifMan.RegisterModule(MODULE_ID, this);
}

std::string CPowerOff::GetId() const
{
	return "poweroff";
}

std::string CPowerOff::GetFunctionName(unsigned int functionId) const
{
	return "unknown";
}

void CPowerOff::Invoke(CMIPS& context, unsigned int functionId)
{
	switch(functionId)
	{
	default:
		CLog::GetInstance().Warn(LOG_NAME, "%08X: Unknown function (%d) called.\r\n", context.m_State.nPC, functionId);
		break;
	}
}
bool CPowerOff::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown RPC method invoked (0x%08X).\r\n", method);
		return true;
	}
}
