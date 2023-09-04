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
	m_module99 = CSifModuleAdapter(std::bind(&CSys147::Invoke99, this,
											 std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	
	sifMan.RegisterModule(MODULE_ID_000, &m_module000);
	sifMan.RegisterModule(MODULE_ID_001, &m_module001);
	sifMan.RegisterModule(MODULE_ID_002, &m_module002);
	sifMan.RegisterModule(MODULE_ID_003, &m_module003);
	sifMan.RegisterModule(MODULE_ID_200, &m_module200);
	sifMan.RegisterModule(MODULE_ID_201, &m_module201);
	sifMan.RegisterModule(MODULE_ID_99, &m_module99);
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

bool CSys147::Invoke99(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case 0x02000000:
		//Command 0x02000000
		//Ret 0 -> size (0x40 blocks)
		//Ret 1... -> some data (in blocks of 0x40 bytes)
		CLog::GetInstance().Warn(LOG_NAME, "LINK_02000000();\r\n");
		//Receive Responses?
		{
			if(!m_pendingReplies.empty())
			{
				auto outputPacket = reinterpret_cast<MODULE_99_PACKET*>(ret + 1);
				memcpy(outputPacket, &m_pendingReplies.front(), sizeof(MODULE_99_PACKET));
				m_pendingReplies.erase(m_pendingReplies.begin());
				reinterpret_cast<uint16*>(ret)[0] = 1;
			}
			else
			{
				reinterpret_cast<uint16*>(ret)[0] = 0;
			}
		}
		break;
	case 0x03004002:
		//Send Command?
		CLog::GetInstance().Warn(LOG_NAME, "LINK_03004002();\r\n");
		{
			assert(argsSize == 0x40);
			auto packet = reinterpret_cast<const MODULE_99_PACKET*>(args);
			CLog::GetInstance().Warn(LOG_NAME, "Recv Command 0x%02X.\r\n", packet->command);

			//0x0D -> ???
			//0x0F -> Get PCB info
			//0x18 -> Switch
			//0x38 -> SCI
			//0x39 -> ???
			//0x58 -> Mechanical Counter
			//0x48 -> Coin Sensor
			//0x68 -> Hopper

			if(packet->command == 0x0F)
			{
				MODULE_99_PACKET reply = {};

				//Stuff that is displayed
				//00:01 -> ??
				//02:03 -> Version, needs to be 0x104 (MSB first)
				//04

				reply.type = 2;
				reply.command = 0x0F;
				reply.data[2] = 0x01;
				reply.data[3] = 0x04;
				uint8 checksum = 0;
				for(const auto& value : reply.data)
				{
					checksum += value;
				}
				reply.checksum = checksum;

				m_pendingReplies.emplace_back(reply);
			}
			
			reinterpret_cast<uint16*>(ret)[0] = 1;
		}
		break;
	case 0x08000000:
		CLog::GetInstance().Warn(LOG_NAME, "LINK_08000000();\r\n");
		reinterpret_cast<uint16*>(ret)[0] = 1;
		break;
	case 0x09000002:
		CLog::GetInstance().Warn(LOG_NAME, "LINK_09000002();\r\n");
		reinterpret_cast<uint16*>(ret)[0] = 0;
		break;
	case 0x0C000002:
		CLog::GetInstance().Warn(LOG_NAME, "LINK_0C000002();\r\n");
		reinterpret_cast<uint16*>(ret)[0] = 0;
		break;
	default:
		reinterpret_cast<uint16*>(ret)[0] = 1;
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x99, method);
		break;
	}
	return true;
}
