#include <assert.h>
#include "Iop_MtapMan.h"
#include "../Log.h"

using namespace Iop;

#define LOG_NAME "iop_mtapman"

CMtapMan::CMtapMan()
{
	m_module901 = CSifModuleAdapter(std::bind(&CMtapMan::Invoke901, this,
	                                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	m_module902 = CSifModuleAdapter(std::bind(&CMtapMan::Invoke902, this,
	                                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	m_module903 = CSifModuleAdapter(std::bind(&CMtapMan::Invoke903, this,
	                                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
}

std::string CMtapMan::GetId() const
{
	return "mtapman";
}

std::string CMtapMan::GetFunctionName(unsigned int) const
{
	return "unknown";
}

void CMtapMan::RegisterSifModules(CSifMan& sif)
{
	sif.RegisterModule(MODULE_ID_1, &m_module901);
	sif.RegisterModule(MODULE_ID_2, &m_module902);
	sif.RegisterModule(MODULE_ID_3, &m_module903);
}

void CMtapMan::Invoke(CMIPS& context, unsigned int functionId)
{
	throw std::runtime_error("Not implemented.");
}

bool CMtapMan::Invoke901(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case 1:
		ret[1] = PortOpen(args[0]);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x901, method);
		break;
	}
	return true;
}

bool CMtapMan::Invoke902(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x902, method);
		break;
	}
	return true;
}

bool CMtapMan::Invoke903(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x903, method);
		break;
	}
	return true;
}

uint32 CMtapMan::PortOpen(uint32 port)
{
	CLog::GetInstance().Print(LOG_NAME, "PortOpen(port = %d);\r\n", port);
	return 0;
}
