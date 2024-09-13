#include <assert.h>
#include "../Log.h"
#include "../Ps2Const.h"
#include "Iop_Cdvdfsv.h"
#include "Iop_Cdvdman.h"
#include "Iop_SifManPs2.h"
#include "TimeUtils.h"

using namespace Iop;

#define LOG_NAME "iop_cdvdfsv"

#define STATE_FILENAME ("iop_cdvdfsv/state.xml")

#define STATE_PENDINGCOMMAND ("PendingCommand")
#define STATE_PENDINGREADSECTOR ("PendingReadSector")
#define STATE_PENDINGREADCOUNT ("PendingReadCount")
#define STATE_PENDINGREADADDR ("PendingReadAddr")

#define STATE_STREAMING ("Streaming")
#define STATE_STREAMPOS ("StreamPos")
#define STATE_STREAMBUFFERSIZE ("StreamBufferSize")

constexpr uint64 COMMAND_DEFAULT_DELAY = TimeUtils::UsecsToCycles(PS2::IOP_CLOCK_OVER_FREQ, 16666);

CCdvdfsv::CCdvdfsv(CSifMan& sif, CCdvdman& cdvdman, uint8* iopRam)
    : m_cdvdman(cdvdman)
    , m_iopRam(iopRam)
{
	m_module592 = CSifModuleAdapter(std::bind(&CCdvdfsv::Invoke592, this,
	                                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	m_module593 = CSifModuleAdapter(std::bind(&CCdvdfsv::Invoke593, this,
	                                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	m_module595 = CSifModuleAdapter(std::bind(&CCdvdfsv::Invoke595, this,
	                                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	m_module596 = CSifModuleAdapter(std::bind(&CCdvdfsv::Invoke596, this,
	                                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	m_module597 = CSifModuleAdapter(std::bind(&CCdvdfsv::Invoke597, this,
	                                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	m_module59A = CSifModuleAdapter(std::bind(&CCdvdfsv::Invoke59A, this,
	                                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	m_module59C = CSifModuleAdapter(std::bind(&CCdvdfsv::Invoke59C, this,
	                                          std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));

	sif.RegisterModule(MODULE_ID_1, &m_module592);
	sif.RegisterModule(MODULE_ID_2, &m_module593);
	sif.RegisterModule(MODULE_ID_4, &m_module595);
	sif.RegisterModule(MODULE_ID_5, &m_module596);
	sif.RegisterModule(MODULE_ID_6, &m_module597);
	sif.RegisterModule(MODULE_ID_7, &m_module59A);
	sif.RegisterModule(MODULE_ID_8, &m_module59C);
}

std::string CCdvdfsv::GetId() const
{
	return "cdvdfsv";
}

std::string CCdvdfsv::GetFunctionName(unsigned int) const
{
	return "unknown";
}

void CCdvdfsv::CountTicks(uint32 ticks, CSifMan* sifMan)
{
	if(m_pendingCommand != COMMAND_NONE)
	{
		m_pendingCommandDelay -= std::min<uint32>(m_pendingCommandDelay, ticks);
		if(m_pendingCommandDelay != 0)
		{
			return;
		}

		static const uint32 sectorSize = 0x800;

		uint8* eeRam = nullptr;
		if(auto sifManPs2 = dynamic_cast<CSifManPs2*>(sifMan))
		{
			eeRam = sifManPs2->GetEeRam();
		}

		if(m_pendingCommand == COMMAND_READ)
		{
			if(m_opticalMedia != nullptr)
			{
				auto fileSystem = m_opticalMedia->GetFileSystem();
				for(unsigned int i = 0; i < m_pendingReadCount; i++)
				{
					fileSystem->ReadBlock(m_pendingReadSector + i, eeRam + (m_pendingReadAddr + (i * sectorSize)));
				}
			}
		}
		else if(m_pendingCommand == COMMAND_READIOP)
		{
			if(m_opticalMedia != nullptr)
			{
				auto fileSystem = m_opticalMedia->GetFileSystem();
				for(unsigned int i = 0; i < m_pendingReadCount; i++)
				{
					fileSystem->ReadBlock(m_pendingReadSector + i, m_iopRam + (m_pendingReadAddr + (i * sectorSize)));
				}
			}
		}
		else if(m_pendingCommand == COMMAND_STREAM_READ)
		{
			if(m_opticalMedia != nullptr)
			{
				auto fileSystem = m_opticalMedia->GetFileSystem();
				for(unsigned int i = 0; i < m_pendingReadCount; i++)
				{
					fileSystem->ReadBlock(m_streamPos, eeRam + (m_pendingReadAddr + (i * sectorSize)));
					m_streamPos++;
				}
			}
		}
		else if(m_pendingCommand == COMMAND_NDISKREADY)
		{
			//Result already set, just return normally
		}
		else if(m_pendingCommand == COMMAND_READCHAIN)
		{
			//Nothing to do, everything has been read already
		}

		m_pendingCommand = COMMAND_NONE;
		sifMan->SendCallReply(MODULE_ID_4, nullptr);
	}
}

void CCdvdfsv::SetOpticalMedia(COpticalMedia* opticalMedia)
{
	m_opticalMedia = opticalMedia;
}

void CCdvdfsv::LoadState(Framework::CZipArchiveReader& archive)
{
	auto registerFile = CRegisterStateFile(*archive.BeginReadFile(STATE_FILENAME));

	m_pendingCommand = static_cast<COMMAND>(registerFile.GetRegister32(STATE_PENDINGCOMMAND));
	m_pendingReadSector = registerFile.GetRegister32(STATE_PENDINGREADSECTOR);
	m_pendingReadCount = registerFile.GetRegister32(STATE_PENDINGREADCOUNT);
	m_pendingReadAddr = registerFile.GetRegister32(STATE_PENDINGREADADDR);

	m_streaming = registerFile.GetRegister32(STATE_STREAMING) != 0;
	m_streamPos = registerFile.GetRegister32(STATE_STREAMPOS);
	m_streamBufferSize = registerFile.GetRegister32(STATE_STREAMBUFFERSIZE);
}

void CCdvdfsv::SaveState(Framework::CZipArchiveWriter& archive) const
{
	auto registerFile = std::make_unique<CRegisterStateFile>(STATE_FILENAME);

	registerFile->SetRegister32(STATE_PENDINGCOMMAND, m_pendingCommand);
	registerFile->SetRegister32(STATE_PENDINGREADSECTOR, m_pendingReadSector);
	registerFile->SetRegister32(STATE_PENDINGREADCOUNT, m_pendingReadCount);
	registerFile->SetRegister32(STATE_PENDINGREADADDR, m_pendingReadAddr);

	registerFile->SetRegister32(STATE_STREAMING, m_streaming);
	registerFile->SetRegister32(STATE_STREAMPOS, m_streamPos);
	registerFile->SetRegister32(STATE_STREAMBUFFERSIZE, m_streamBufferSize);

	archive.InsertFile(std::move(registerFile));
}

void CCdvdfsv::Invoke(CMIPS& context, unsigned int functionId)
{
	throw std::runtime_error("Not implemented.");
}

bool CCdvdfsv::Invoke592(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case 0:
	{
		//Init
		assert(argsSize >= 4);
		uint32 mode = args[0x00];
		if(retSize != 0)
		{
			assert(retSize >= 0x10);
			// ret[1]; // cdvdfsv Ver
			// ret[2]; // cdvdman Ver
			ret[0x03] = 0xFF;
		}
		CLog::GetInstance().Print(LOG_NAME, "Init(mode = %d);\r\n", mode);
	}
	break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x592, method);
		break;
	}
	return true;
}

bool CCdvdfsv::Invoke593(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case 0x01:
	{
		assert(retSize >= 0xC);
		CLog::GetInstance().Print(LOG_NAME, "ReadClock();\r\n");

		auto clockBuffer = reinterpret_cast<uint8*>(ret + 1);
		(*ret) = m_cdvdman.CdReadClockDirect(clockBuffer);
	}
	break;

	case 0x03:
		assert(retSize >= 4);
		CLog::GetInstance().Print(LOG_NAME, "GetDiskType();\r\n");
		ret[0x00] = m_cdvdman.CdGetDiskTypeDirect(m_opticalMedia);
		break;

	case 0x04:
		assert(retSize >= 4);
		CLog::GetInstance().Print(LOG_NAME, "GetError();\r\n");
		ret[0x00] = 0x00;
		break;

	case 0x05:
	{
		assert(argsSize >= 4);
		assert(retSize >= 8);
		uint32 mode = args[0x00];
		CLog::GetInstance().Print(LOG_NAME, "TrayReq(mode = %d);\r\n", mode);
		ret[0x00] = 0x01; //Result
		ret[0x01] = 0x00; //Tray check
	}
	break;

	case 0x0C:
		//Status
		assert(retSize >= 4);
		CLog::GetInstance().Print(LOG_NAME, "Status();\r\n");
		ret[0x00] = m_streaming ? CCdvdman::CDVD_STATUS_SEEK : CCdvdman::CDVD_STATUS_PAUSED;
		break;

	case 0x16:
		//Break
		{
			CLog::GetInstance().Print(LOG_NAME, "Break();\r\n");
			ret[0x00] = 1;
		}
		break;

	case 0x22:
	{
		//Set Media Mode (1 - CDROM, 2 - DVDROM)
		assert(argsSize >= 4);
		assert(retSize >= 4);
		uint32 mode = args[0x00];
		CLog::GetInstance().Print(LOG_NAME, "SetMediaMode(mode = %i);\r\n", mode);
		ret[0x00] = 1;
	}
	break;
	case 0x27:
	{
		//ReadDvdDualInfo
		assert(retSize >= 0x08);
		CLog::GetInstance().Print(LOG_NAME, "ReadDvdDualInfo();\r\n");
		ret[0] = 1;
		ret[1] = (m_opticalMedia && m_opticalMedia->GetDvdIsDualLayer()) ? 1 : 0;
		//Some games (Xenosaga) have a different version of this call which returns only 8 bytes
		if(retSize >= 0x0C)
		{
			ret[2] = m_opticalMedia ? m_opticalMedia->GetDvdSecondLayerStart() : 0;
		}
	}
	break;

	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x593, method);
		break;
	}
	return true;
}

bool CCdvdfsv::Invoke595(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case 0x01:
		Read(args, argsSize, ret, retSize, ram);
		return false;
		break;

	case 0x04:
	{
		//GetToc
		assert(argsSize >= 4);
		assert(retSize >= 4);
		uint32 nBuffer = args[0x00];
		CLog::GetInstance().Print(LOG_NAME, "GetToc(buffer = 0x%08X);\r\n", nBuffer);
		ret[0x00] = 1;
	}
	break;

	case 0x05:
	{
		assert(argsSize >= 4);
		uint32 seekSector = args[0];
		CLog::GetInstance().Print(LOG_NAME, "Seek(sector = 0x%08X);\r\n", seekSector);
	}
	break;

	case 0x09:
		return StreamCmd(args, argsSize, ret, retSize, ram);
		break;

	case 0x0D:
		ReadIopMem(args, argsSize, ret, retSize, ram);
		return false;
		break;

	case 0x0E:
		return NDiskReady(args, argsSize, ret, retSize, ram);
		break;

	case 0x0F:
		ReadChain(args, argsSize, ret, retSize, ram);
		return false;
		break;

	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x595, method);
		break;
	}
	return true;
}

bool CCdvdfsv::Invoke596(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case 0x01:
		//SetPowerOffCallback
		//Some versions of the EE CDVD library will invoke this in CdInit. It seems to be called in a no-wait mode,
		//and it's possible that the reply to this call is only sent once. Sending it always seems to cause
		//problems in some games (Hunter x Hunter: Ryuumyaku no Saidan) that use CD callbacks and call CdInit
		//many times.
		CLog::GetInstance().Print(LOG_NAME, "SetPowerOffCallback();\r\n");
		return false;
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x596, method);
		break;
	}
	return true;
}

bool CCdvdfsv::Invoke597(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case 0:
		SearchFile(args, argsSize, ret, retSize, ram);
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x597, method);
		break;
	}
	return true;
}

bool CCdvdfsv::Invoke59A(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	return Invoke59C(method, args, argsSize, ret, retSize, ram);
}

bool CCdvdfsv::Invoke59C(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case 0:
	{
		//DiskReady (returns 2 if ready, 6 if not ready)
		assert(retSize >= 4);
		assert(argsSize >= 4);
		uint32 mode = args[0x00];
		CLog::GetInstance().Print(LOG_NAME, "DiskReady(mode = %i);\r\n", mode);
		ret[0x00] = 2;
	}
	break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown method invoked (0x%08X, 0x%08X).\r\n", 0x59C, method);
		break;
	}
	return true;
}

void CCdvdfsv::Read(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	uint32 sector = args[0x00];
	uint32 count = args[0x01];
	uint32 dstAddr = args[0x02];
	uint32 mode = args[0x03];

	CLog::GetInstance().Print(LOG_NAME, "Read(sector = 0x%08X, count = 0x%08X, addr = 0x%08X, mode = 0x%08X);\r\n",
	                          sector, count, dstAddr, mode);

	//We write the result now, but ideally should be only written
	//when pending read is completed
	if(retSize >= 4)
	{
		ret[0] = 0;
	}

	assert(m_pendingCommand == COMMAND_NONE);
	m_pendingCommand = COMMAND_READ;
	m_pendingCommandDelay = CCdvdman::COMMAND_READ_BASE_DELAY + (count * CCdvdman::COMMAND_READ_SECTOR_DELAY);
	m_pendingReadSector = sector;
	m_pendingReadCount = count;
	m_pendingReadAddr = dstAddr & 0x1FFFFFFF;
}

void CCdvdfsv::ReadIopMem(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	uint32 sector = args[0x00];
	uint32 count = args[0x01];
	uint32 dstAddr = args[0x02];
	uint32 mode = args[0x03];

	CLog::GetInstance().Print(LOG_NAME, "ReadIopMem(sector = 0x%08X, count = 0x%08X, addr = 0x%08X, mode = 0x%08X);\r\n",
	                          sector, count, dstAddr, mode);

	if(retSize >= 4)
	{
		ret[0] = 0;
	}

	assert(m_pendingCommand == COMMAND_NONE);
	m_pendingCommand = COMMAND_READIOP;
	m_pendingCommandDelay = COMMAND_DEFAULT_DELAY;
	m_pendingReadSector = sector;
	m_pendingReadCount = count;
	m_pendingReadAddr = dstAddr & 0x1FFFFFFF;
}

bool CCdvdfsv::StreamCmd(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	bool immediateReply = true;

	uint32 sector = args[0x00];
	uint32 count = args[0x01];
	uint32 dstAddr = args[0x02];
	uint32 cmd = args[0x03];
	uint32 mode = args[0x04];

	CLog::GetInstance().Print(LOG_NAME, "StreamCmd(sector = 0x%08X, count = 0x%08X, addr = 0x%08X, cmd = 0x%08X, mode = 0x%08X);\r\n",
	                          sector, count, dstAddr, cmd, mode);

	assert(m_pendingCommand == COMMAND_NONE);

	switch(cmd)
	{
	case 1:
		//Start
		m_streamPos = sector;
		ret[0] = 1;
		CLog::GetInstance().Print(LOG_NAME, "StreamStart(pos = 0x%08X);\r\n", sector);
		m_streaming = true;
		break;
	case 2:
		//Read
		m_pendingCommand = COMMAND_STREAM_READ;
		m_pendingCommandDelay = COMMAND_DEFAULT_DELAY;
		m_pendingReadSector = 0;
		m_pendingReadCount = count;
		m_pendingReadAddr = dstAddr & (PS2::EE_RAM_SIZE - 1);
		ret[0] = count;
		immediateReply = false;
		CLog::GetInstance().Print(LOG_NAME, "StreamRead(count = 0x%08X, dest = 0x%08X);\r\n",
		                          count, dstAddr);
		break;
	case 3:
		//Stop
		ret[0] = 1;
		CLog::GetInstance().Print(LOG_NAME, "StreamStop();\r\n");
		m_streaming = false;
		break;
	case 5:
		//Init
		ret[0] = 1;
		CLog::GetInstance().Print(LOG_NAME, "StreamInit(bufsize = 0x%08X, numbuf = 0x%08X, buf = 0x%08X);\r\n",
		                          sector, count, dstAddr);
		m_streamBufferSize = sector;
		break;
	case 6:
		//Status
		ret[0] = m_streamBufferSize;
		CLog::GetInstance().Print(LOG_NAME, "StreamStat();\r\n");
		break;
	case 4:
	case 9:
		//Seek
		m_streamPos = sector;
		ret[0] = 1;
		CLog::GetInstance().Print(LOG_NAME, "StreamSeek(pos = 0x%08X);\r\n", sector);
		break;
	default:
		CLog::GetInstance().Warn(LOG_NAME, "Unknown stream command used.\r\n");
		break;
	}

	return immediateReply;
}

bool CCdvdfsv::NDiskReady(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	//DiskReady (returns 2 if ready, 6 if not ready)
	assert(retSize >= 4);
	CLog::GetInstance().Print(LOG_NAME, "NDiskReady();\r\n");
	if(m_pendingCommand != COMMAND_NONE)
	{
		ret[0x00] = 6;
		return true;
	}
	else
	{
		//Delay command (required by Downhill Domination)
		m_pendingCommand = COMMAND_NDISKREADY;
		m_pendingCommandDelay = COMMAND_DEFAULT_DELAY;
		ret[0x00] = 2;
		return false;
	}
}

void CCdvdfsv::ReadChain(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	assert(argsSize >= 788);
	assert(retSize == 0);

	CLog::GetInstance().Print(LOG_NAME, "ReadChain(...);\r\n");

	auto fileSystem = m_opticalMedia->GetFileSystem();
	static const uint32 sectorSize = 0x800;

	static const uint32 maxTupleCount = 64;
	for(uint32 tuple = 0; tuple < maxTupleCount; tuple++)
	{
		uint32 tupleBase = (tuple * 3);
		uint32 sectorPos = args[tupleBase + 0];
		uint32 sectorCount = args[tupleBase + 1];
		uint32 dstAddress = args[tupleBase + 2];
		if((sectorPos == ~0U) || (sectorCount == ~0U) || (dstAddress == ~0U))
		{
			break;
		}
		assert((dstAddress & 1) == 0);
		for(unsigned int sector = 0; sector < sectorCount; sector++)
		{
			fileSystem->ReadBlock(sectorPos + sector, ram + (dstAddress + (sector * sectorSize)));
		}
	}

	//DBZ: Budokai Tenkaichi hangs in its loading screen if this command's result is not delayed.
	m_pendingCommand = COMMAND_READCHAIN;
	m_pendingCommandDelay = COMMAND_DEFAULT_DELAY;
}

void CCdvdfsv::SearchFile(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	uint32 layer = 0;
	uint32 pathOffset = 0x24;
	if(argsSize == 0x128)
	{
		pathOffset = 0x24;
	}
	else if(argsSize == 0x124)
	{
		pathOffset = 0x20;
	}
	else if(argsSize == 0x12C)
	{
		//Used by:
		//- Xenosaga (dual layer)
		pathOffset = 0x24;
		layer = args[0x128 / 4];
	}
	else
	{
		CLog::GetInstance().Warn(LOG_NAME, "Warning: Using unknown structure size (%d bytes);\r\n", argsSize);
	}

	assert(retSize == 4);

	if(!m_opticalMedia)
	{
		ret[0] = 0;
		return;
	}

	//0x12C structure
	//00 - Block Num
	//04 - Size
	//08
	//0C
	//10
	//14
	//18
	//1C
	//20 - Unknown
	//24 - Path

	const char* path = reinterpret_cast<const char*>(args) + pathOffset;
	CLog::GetInstance().Print(LOG_NAME, "SearchFile(layer = %d, path = '%s');\r\n", layer, path);

	CCdvdman::FILEINFO fileInfo = {};
	uint32 result = m_cdvdman.CdLayerSearchFileDirect(m_opticalMedia, &fileInfo, path, layer);
	if(result)
	{
		args[0x00] = fileInfo.sector;
		args[0x01] = fileInfo.size;
	}
	ret[0] = result;
}
