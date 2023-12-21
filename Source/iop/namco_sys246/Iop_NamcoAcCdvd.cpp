#include "Iop_NamcoAcCdvd.h"
#include "../Iop_Cdvdman.h"
#include "../../Ps2Const.h"
#include "../../Log.h"

using namespace Iop;
using namespace Iop::Namco;

#define LOG_NAME ("iop_namco_accdvd")

#define FUNCTION_CDSEARCHFILE "IopCdSearchFile"
#define FUNCTION_CDREAD "IopCdRead"
#define FUNCTION_CDSYNC "IopCdSync"
#define FUNCTION_CDUNKNOWN_17 "IopCdUnknown_17"
#define FUNCTION_CDUNKNOWN_19 "IopCdUnknown_19"
#define FUNCTION_CDGETERROR "IopCdGetError"
#define FUNCTION_CDGETDISKTYPE "IopCdGetDiskType"
#define FUNCTION_CDUNKNOWN_37 "IopCdUnknown_37"

CAcCdvd::CAcCdvd(CSifMan& sif, CCdvdman& cdvdman, uint8* iopRam, CAcRam& acRam)
    : m_cdvdman(cdvdman)
    , m_iopRam(iopRam)
    , m_acRam(acRam)
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

std::string CAcCdvd::GetFunctionName(unsigned int functionId) const
{
	switch(functionId)
	{
	case 10:
		return FUNCTION_CDSEARCHFILE;
	case 11:
		return FUNCTION_CDREAD;
	case 16:
		return FUNCTION_CDSYNC;
	case 17:
		return FUNCTION_CDUNKNOWN_17;
	case 19:
		return FUNCTION_CDUNKNOWN_19;
	case 20:
		return FUNCTION_CDGETERROR;
	case 21:
		return FUNCTION_CDGETDISKTYPE;
	case 37:
		return FUNCTION_CDUNKNOWN_37;
	default:
		return "unknown";
	}
}

void CAcCdvd::Invoke(CMIPS& context, unsigned int functionId)
{
	switch(functionId)
	{
	case 10:
	{
		uint32 fileInfoPtr = context.m_State.nGPR[CMIPS::A0].nV0;
		uint32 pathPtr = context.m_State.nGPR[CMIPS::A1].nV0;
		CLog::GetInstance().Warn(LOG_NAME, FUNCTION_CDSEARCHFILE "(fileInfoPtr = 0x%08X, pathPtr = %s);\r\n",
		                         fileInfoPtr,
		                         PrintStringParameter(m_iopRam, pathPtr).c_str());
		auto path = reinterpret_cast<const char*>(m_iopRam + pathPtr);
		auto fileInfo = reinterpret_cast<CCdvdman::FILEINFO*>(m_iopRam + fileInfoPtr);
		uint32 result = m_cdvdman.CdLayerSearchFileDirect(m_opticalMedia, fileInfo, path, 0);
		context.m_State.nGPR[CMIPS::V0].nV0 = result;
	}
	break;
	case 11:
	{
		uint32 startSector = context.m_State.nGPR[CMIPS::A0].nV0;
		uint32 sectorCount = context.m_State.nGPR[CMIPS::A1].nV0;
		uint32 bufferPtr = context.m_State.nGPR[CMIPS::A2].nV0;
		uint32 modePtr = context.m_State.nGPR[CMIPS::A3].nV0;
		CLog::GetInstance().Warn(LOG_NAME, FUNCTION_CDREAD "(startSector = 0x%X, sectorCount = 0x%X, bufferPtr = 0x%08X, modePtr = 0x%08X);\r\n",
		                         startSector, sectorCount, bufferPtr, modePtr);
		context.m_State.nGPR[CMIPS::V0].nV0 = m_cdvdman.CdRead(startSector, sectorCount, bufferPtr, modePtr);
	}
	break;
	case 16:
		//CdSync
		{
			uint32 mode = context.m_State.nGPR[CMIPS::A0].nV0;
			CLog::GetInstance().Warn(LOG_NAME, FUNCTION_CDSYNC "(mode = %d);\r\n",
			                         mode);
			context.m_State.nGPR[CMIPS::V0].nV0 = m_cdvdman.CdSync(mode);
		}
		break;
	case 17:
		CLog::GetInstance().Warn(LOG_NAME, FUNCTION_CDUNKNOWN_17 "(param0 = %d);\r\n",
		                         context.m_State.nGPR[CMIPS::A0].nV0);
		context.m_State.nGPR[CMIPS::V0].nV0 = 1;
		break;
	case 19:
		//CdDiskReady
		CLog::GetInstance().Warn(LOG_NAME, FUNCTION_CDUNKNOWN_19 "(param0 = %d);\r\n",
		                         context.m_State.nGPR[CMIPS::A0].nV0);
		context.m_State.nGPR[CMIPS::V0].nV0 = 2;
		break;
	case 20:
		CLog::GetInstance().Warn(LOG_NAME, FUNCTION_CDGETERROR "();\r\n");
		context.m_State.nGPR[CMIPS::V0].nV0 = 0;
		break;
	case 21:
		CLog::GetInstance().Warn(LOG_NAME, FUNCTION_CDGETDISKTYPE "();\r\n");
		context.m_State.nGPR[CMIPS::V0].nV0 = m_cdvdman.CdGetDiskTypeDirect(m_opticalMedia);
		break;
	case 37:
		CLog::GetInstance().Warn(LOG_NAME, FUNCTION_CDUNKNOWN_37 "(param0 = %d);\r\n",
		                         context.m_State.nGPR[CMIPS::A0].nV0);
		context.m_State.nGPR[CMIPS::V0].nV0 = 1;
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown IOP method invoked (0x%08X).\r\n", functionId);
		break;
	}
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
			static const size_t sectorSize = 0x800;
			CLog::GetInstance().Warn(LOG_NAME, "Read%d(start = 0x%08X, count = %d, dstAddr = 0x%08X);\r\n",
			                         method, startSector, sectorCount, dstAddr);
			auto fileSystem = m_opticalMedia->GetFileSystem();
			auto dst = (method == 0x0A) ? m_iopRam : ram;
			for(unsigned int i = 0; i < sectorCount; i++)
			{
				uint32 dstOffset = (dstAddr + (i * sectorSize));
				uint32 sectorIndex = startSector + i;
				if((method == 0x0A) && (dstAddr >= 0x40000000))
				{
					uint8 sectorData[sectorSize];
					fileSystem->ReadBlock(sectorIndex, sectorData);
					m_acRam.Write(dstOffset - 0x40000000, sectorData, sectorSize);
				}
				else
				{
					fileSystem->ReadBlock(sectorIndex, dst + dstOffset);
				}
			}
		}
		break;
	case 0x0B:
		//Ridge Racer V uses this
		//Time Crisis 3 uses this, fails to proceed if this doesn't return 0
		ret[0x01] = 0;
		CLog::GetInstance().Warn(LOG_NAME, "CdSync();\r\n");
		break;
	case 0x0C:
		//SearchFile?
		{
			const char* path = reinterpret_cast<const char*>(ram) + args[0];
			CCdvdman::FILEINFO fileInfo = {};
			uint32 result = m_cdvdman.CdLayerSearchFileDirect(m_opticalMedia, &fileInfo, path, 0);
			CLog::GetInstance().Warn(LOG_NAME, "SearchFile(path = '%s');\r\n", path);
			ret[0x00] = 0;      //Soul Calibur 2 requires this to be 0
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
	case 0x1A:
	{
		assert(retSize >= 0x10);
		CLog::GetInstance().Print(LOG_NAME, "ReadRtc();\r\n");

		auto clockBuffer = reinterpret_cast<uint8*>(ret + 2);
		ret[0x00] = 0;
		ret[0x01] = m_cdvdman.CdReadClockDirect(clockBuffer);
	}
	break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown SIF method invoked (0x%08X).\r\n", method);
		break;
	}
	return true;
}
