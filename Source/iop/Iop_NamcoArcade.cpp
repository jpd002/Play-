#include "Iop_NamcoArcade.h"
#include "../Log.h"
#include "Iop_Cdvdman.h"

using namespace Iop;

#define LOG_NAME "iop_namcoarcade"

CNamcoArcade::CNamcoArcade(CSifMan& sif, CCdvdman& cdvdman, uint8* iopRam)
: m_iopRam(iopRam)
, m_cdvdman(cdvdman)
{
	m_module001 = CSifModuleAdapter(std::bind(&CNamcoArcade::Invoke001, this,
											  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	m_module002 = CSifModuleAdapter(std::bind(&CNamcoArcade::Invoke002, this,
											  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	m_module003 = CSifModuleAdapter(std::bind(&CNamcoArcade::Invoke003, this,
											  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	m_module004 = CSifModuleAdapter(std::bind(&CNamcoArcade::Invoke004, this,
											  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));

	sif.RegisterModule(MODULE_ID_1, &m_module001);
	sif.RegisterModule(MODULE_ID_2, &m_module002);
	sif.RegisterModule(MODULE_ID_3, &m_module003);
	sif.RegisterModule(MODULE_ID_4, &m_module004);
}

void CNamcoArcade::SetOpticalMedia(COpticalMedia* opticalMedia)
{
	m_opticalMedia = opticalMedia;
}

std::string CNamcoArcade::GetId() const
{
	return "namcoarcade";
}

std::string CNamcoArcade::GetFunctionName(unsigned int) const
{
	return "unknown";
}

void CNamcoArcade::Invoke(CMIPS& context, unsigned int functionId)
{
	throw std::runtime_error("Not implemented.");
}

bool CNamcoArcade::Invoke001(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x001, method);
		break;
	}
	return true;
}

bool CNamcoArcade::Invoke002(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case 0x02:
		//CdStatus?
		ret[0x01] = 0; //Result?
		break;
	case 0x07:
		//Init?
		ret[0x01] = 1; //Result?
		break;
	case 0x09:
	case 0x0A:
		//Read?
		ret[0x01] = 1; //Result?
		{
			uint32 startSector = args[0];
			uint32 sectorCount = args[1];
			uint32 dstAddr = args[2];
			auto fileSystem = m_opticalMedia->GetFileSystem();
			auto dst = (method == 0x0A) ? m_iopRam : ram;
			for(unsigned int i = 0; i < sectorCount; i++)
			{
				fileSystem->ReadBlock(startSector + i, dst + (dstAddr + (i * 0x800)));
			}
		}
		break;
	case 0x0B:
		//Load backup ram?
		ret[0x01] = 1;
		break;
	case 0x0C:
		//SearchFile?
		{
			const char* path = reinterpret_cast<const char*>(ram) + args[0];
			CCdvdman::FILEINFO fileInfo = {};
			uint32 result = m_cdvdman.CdLayerSearchFileDirect(m_opticalMedia, &fileInfo, path, 0);
			ret[0x01] = result; //Result?
			ret[0x04] = fileInfo.sector;
			ret[0x05] = fileInfo.size;
			ret[0x0A] = ~0;
			ret[0x0B] = ~0;
		}
		break;
	case 0x0D:
		//Seek?
		ret[0x01] = 1; //Result?
		//args[0] = Sector Index
		//args[1] = ? 0xF
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x002, method);
		break;
	}
	return true;
}

bool CNamcoArcade::Invoke003(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	//Brake and Gaz pedals control?
	switch(method)
	{
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x003, method);
		break;
	}
	return true;
}

bool CNamcoArcade::Invoke004(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case 0x01:
		ret[0] = 3;
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x004, method);
		break;
	}
	return true;
}
