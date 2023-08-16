#include "Iop_NamcoSys147.h"
#include "../../Log.h"

using namespace Iop;
using namespace Iop::Namco;

#define LOG_NAME ("iop_namco_sys147")

CSys147::CSys147(CSifMan& sifMan)
{
	m_module000 = CSifModuleAdapter(std::bind(&CSys147::Invoke000, this,
											  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	m_module001 = CSifModuleAdapter(std::bind(&CSys147::Invoke001, this,
											  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	m_module002 = CSifModuleAdapter(std::bind(&CSys147::Invoke002, this,
											  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	m_module003 = CSifModuleAdapter(std::bind(&CSys147::Invoke003, this,
											  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	m_module200 = CSifModuleAdapter(std::bind(&CSys147::Invoke200, this,
											  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	m_module201 = CSifModuleAdapter(std::bind(&CSys147::Invoke201, this,
											  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));

	sifMan.RegisterModule(MODULE_ID_000, &m_module000);
	sifMan.RegisterModule(MODULE_ID_001, &m_module001);
	sifMan.RegisterModule(MODULE_ID_002, &m_module002);
	sifMan.RegisterModule(MODULE_ID_003, &m_module003);
	sifMan.RegisterModule(MODULE_ID_200, &m_module200);
	sifMan.RegisterModule(MODULE_ID_201, &m_module201);
}

std::string CSys147::GetId() const
{
	return "sys147";
}

std::string CSys147::GetFunctionName(unsigned int functionId) const
{
	return "unknown";
}

void CSys147::Invoke(CMIPS& context, unsigned int functionId)
{
	throw std::runtime_error("Not implemented.");
}

bool CSys147::Invoke000(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x000, method);
		break;
	}
	return true;
}

bool CSys147::Invoke001(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x001, method);
		break;
	}
	return true;
}

bool CSys147::Invoke002(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x002, method);
		break;
	}
	return true;
}

bool CSys147::Invoke003(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x003, method);
		break;
	}
	return true;
}

bool CSys147::Invoke200(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x200, method);
		break;
	}
	return true;
}

bool CSys147::Invoke201(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x201, method);
		break;
	}
	return true;
}
