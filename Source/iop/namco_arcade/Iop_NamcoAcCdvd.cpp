#include "Iop_NamcoAcCdvd.h"
#include "../Iop_Cdvdman.h"
#include "../../Ps2Const.h"
#include "../../Log.h"

using namespace Iop;
using namespace Iop::Namco;

#define LOG_NAME ("iop_namco_accdvd")

CAcCdvd::CAcCdvd(CSifMan& sif, CCdvdman& cdvdman, uint8* iopRam)
: m_cdvdman(cdvdman)
, m_iopRam(iopRam)
{
	sif.RegisterModule(MODULE_ID, this);
}

void CAcCdvd::SetOpticalMedia(COpticalMedia* opticalMedia)
{
	m_opticalMedia = opticalMedia;
}

std::string CAcCdvd::GetId() const
{
	return "accdvd";
}

std::string CAcCdvd::GetFunctionName(unsigned int) const
{
	return "unknown";
}

void CAcCdvd::Invoke(CMIPS& context, unsigned int functionId)
{
	throw std::runtime_error("Not implemented.");
}

bool CAcCdvd::Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case 0x02:
		//CdStatus? or DiskReady?
		//Game seems to want this to be 2 before proceeding
		ret[0x01] = 2;
		break;
	case 0x03:
		//CdType
		CLog::GetInstance().Print(LOG_NAME, "CdType();\r\n");
		ret[0x01] = m_cdvdman.CdGetDiskTypeDirect(m_opticalMedia);
		break;
	case 0x05:
		//Unknown, used by Ridge Racer
		CLog::GetInstance().Warn(LOG_NAME, "Cmd5();\r\n");
		ret[0x01] = 1; //Result?
		break;
	case 0x07:
		//Init?
		CLog::GetInstance().Warn(LOG_NAME, "Cmd7();\r\n");
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
			CLog::GetInstance().Warn(LOG_NAME, "Read%d(start = 0x%08X, count = %d, dstAddr = 0x%08X);\r\n",
									 method, startSector, sectorCount, dstAddr);
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
		//Ridge Racer V uses this
		//Time Crisis 3 uses this, fails to proceed if this doesn't return 0
		ret[0x01] = 0;
		CLog::GetInstance().Warn(LOG_NAME, "Cmd11();\r\n");
		break;
	case 0x0C:
		//SearchFile?
		{
			const char* path = reinterpret_cast<const char*>(ram) + args[0];
			CCdvdman::FILEINFO fileInfo = {};
			uint32 result = m_cdvdman.CdLayerSearchFileDirect(m_opticalMedia, &fileInfo, path, 0);
			CLog::GetInstance().Warn(LOG_NAME, "SearchFile(path = '%s');\r\n", path);
			ret[0x01] = result; //Result?
			ret[0x04] = fileInfo.sector;
			ret[0x05] = fileInfo.size;
			ret[0x0A] = ~0; //Some date/time info
			ret[0x0B] = ~0;
		}
		break;
	case 0x0D:
		//Seek?
		ret[0x01] = 1; //Result?
		CLog::GetInstance().Warn(LOG_NAME, "Seek(sector = 0x%08X);\r\n", args[0]);
		//args[0] = Sector Index?
		//args[1] = ? 0xF
		break;
	case 0x0F:
		ret[0x01] = 2; //Result? (needs to be not 1 or 0x20)
		CLog::GetInstance().Warn(LOG_NAME, "Cmd15();\r\n");
		break;
	case 0x13:
		{
			uint32 sectorCount = args[0];
			uint32 mode = args[1];
			uint32 dstAddr = args[2] & (PS2::EE_RAM_SIZE - 1);
			uint32 errAddr = args[3];
			CLog::GetInstance().Warn(LOG_NAME, "StRead(count = %d, mode = %d, dstAddr = 0x%08X, errAddr = 0x%08X);\r\n",
									 sectorCount, mode, dstAddr, errAddr);
			auto fileSystem = m_opticalMedia->GetFileSystem();
			for(unsigned int i = 0; i < sectorCount; i++)
			{
				fileSystem->ReadBlock(m_streamPos + i, ram + (dstAddr + (i * 0x800)));
				m_streamPos++;
			}
			if(errAddr != 0)
			{
				auto err = reinterpret_cast<uint32*>(ram + errAddr);
				(*err) = 0; //No error
			}
		}
		break;
	case 0x15:
		{
			//Stream Seek?
			CLog::GetInstance().Warn(LOG_NAME, "StSeek(sector = 0x%08X);\r\n", args[0]);
			m_streamPos = args[0];
			ret[0x01] = 1;
		}
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x002, method);
		break;
	}
	return true;
}
