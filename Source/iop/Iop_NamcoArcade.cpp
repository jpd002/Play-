#include "Iop_NamcoArcade.h"
#include "../Log.h"
#include "../Ps2Const.h"
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

static uint32 registerPtr = 0;

void CNamcoArcade::SetButtonState(unsigned int padNumber, PS2::CControllerInfo::BUTTON button, bool pressed, uint8* ram)
{
	if(pressed && (registerPtr != 0))
	{
		*reinterpret_cast<uint16*>(ram + registerPtr + 0xC0) += 1;
	}
}

void CNamcoArcade::SetAxisState(unsigned int padNumber, PS2::CControllerInfo::BUTTON button, uint8 axisValue, uint8* ram)
{
}

bool CNamcoArcade::Invoke001(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	//JVIO stuff?
	static uint16 valueToHold = 0;
	static uint16 otherImportantValue = 0;
	switch(method)
	{
	case 0x02:
		{
			//Ridge Racer 5 have argsSize = 0x18 (module name seems different)
			if(argsSize == 0x18)
			{
				//loadbackupram?
				//args[3] = ram ptr?
				//args[4] = src
				//args[5] = size
				uint32 infoPtr = args[4];
				uint32* info = reinterpret_cast<uint32*>(ram + infoPtr);
				int i = 0;
				i++;
			}
			//Sengoku Basara & Time Crisis have argsSize == 0x10
			else if(argsSize == 0x10)
			{
				assert(argsSize >= 0x10);
				uint32 infoPtr = args[2];
				//CLog::GetInstance().Warn(LOG_NAME, "cmd2[_0 = 0x%08X, _1 = 0x%08X, _2 = 0x%08X, _3 = 0x%08X];\r\n",
				//						 args[0], args[1], args[2], args[3]);
				uint32* info = reinterpret_cast<uint32*>(ram + infoPtr);
				CLog::GetInstance().Warn(LOG_NAME, "jvioboot?(infoPtr = 0x%08X);\r\n", infoPtr);
				if(info[2] == 0x60002300)
				{
					//Send?
					uint32 dataPtr = info[1];
					uint16* data = reinterpret_cast<uint16*>(ram + dataPtr);
					valueToHold = data[0];
					otherImportantValue = data[3];
					CLog::GetInstance().Warn(LOG_NAME, "sending (0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X, 0x%04X);\r\n",
											 data[0], data[1], data[2], data[3],
											 data[4], data[5], data[6], data[7]);
				}
				else if(info[1] == 0x60002000)
				{
					//Recv?
					uint32 dataPtr = info[2];
					uint16* data = reinterpret_cast<uint16*>(ram + dataPtr);
					CLog::GetInstance().Warn(LOG_NAME, "recving (dataPtr = 0x%08X);\r\n", dataPtr);
					data[0] = valueToHold;
					if(valueToHold == 0x3E6F)
					{
						data[1] = 0x225; //firmware version?
						//data[0x18] = 0x01; //Some size?
					}
				}
				else
				{
					CLog::GetInstance().Warn(LOG_NAME, "Unknown Packet\r\n");
					CLog::GetInstance().Warn(LOG_NAME, "info[_0 = 0x%08X, _1 = 0x%08X, _2 = 0x%08X, _3 = 0x%08X];\r\n",
											 info[0], info[1], info[2], info[3]);
					CLog::GetInstance().Warn(LOG_NAME, "------------------------------------------\r\n");
				}
			}
			else
			{
				CLog::GetInstance().Warn(LOG_NAME, "Unknown args size for method 2: %d.\r\n", argsSize);
			}
		}
		break;
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

bool CNamcoArcade::Invoke003(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	//method 0x02 -> jvsif_starts
	//method 0x04 -> jvsif_registers
	//Brake and Gaz pedals control?
	switch(method)
	{
	case 0x04:
		{
			CLog::GetInstance().Warn(LOG_NAME, "jvsif_registers(0x%08X, 0x%08X);\r\n", args[0], args[1]);
			uint32* params = reinterpret_cast<uint32*>(ram + args[0]);
			registerPtr = params[3];
			//Set break and gaz values?
			//*reinterpret_cast<uint16*>(ram + registerPtr + 0x0E) = 0x0800;
			//*reinterpret_cast<uint16*>(ram + registerPtr + 0xC0) = 0x22;
		}
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x003, method);
		break;
	}
	return true;
}

bool CNamcoArcade::Invoke004(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	//ac194 - Ridge Racer V & Wangan, force feedback
	switch(method)
	{
	case 0x01:
		CLog::GetInstance().Print(LOG_NAME, "ac194_status();\r\n");
		ret[0x00] = 3;
		reinterpret_cast<uint8*>(ret)[0x0C] = 'C';
		reinterpret_cast<uint8*>(ret)[0x0D] = '0';
		reinterpret_cast<uint8*>(ret)[0x0E] = '1';
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x004, method);
		break;
	}
	return true;
}
