#include "Iop_NamcoPadMan.h"
#include "Log.h"

using namespace Iop;
using namespace Iop::Namco;

#define LOG_NAME ("iop_namco_padman")

std::string CPadMan::GetId() const
{
	return "padman";
}

std::string CPadMan::GetFunctionName(unsigned int) const
{
	return "unknown";
}

void CPadMan::RegisterSifModules(CSifMan& sif)
{
	sif.RegisterModule(MODULE_ID_1, this);
	sif.RegisterModule(MODULE_ID_2, this);
	sif.RegisterModule(MODULE_ID_3, this);
	sif.RegisterModule(MODULE_ID_4, this);
}

void CPadMan::Invoke(CMIPS& context, unsigned int functionId)
{
	throw std::runtime_error("Not implemented.");
}

bool CPadMan::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(method == 1);
	method = args[0];
	switch(method)
	{
	case 0x00000010:
		Init(args, argsSize, ret, retSize, ram);
		break;
	case 0x00000012:
		GetModuleVersion(args, argsSize, ret, retSize, ram);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown method invoked (0x%08X).\r\n", method);
		break;
	}
	return true;
}

void CPadMan::Init(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(retSize >= 0x10);

	CLog::GetInstance().Print(LOG_NAME, "Init();\r\n");

	ret[3] = 1;
}

void CPadMan::GetModuleVersion(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(retSize >= 0x10);

	CLog::GetInstance().Print(LOG_NAME, "GetModuleVersion();\r\n");

	ret[3] = 0x00000400;
}
