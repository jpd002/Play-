#include <assert.h>
#include "../Log.h"
#include "../Ps2Const.h"
#include "Iop_Cdvdfsv.h"
#include "Iop_SifManPs2.h"

using namespace Iop;

#define LOG_NAME "iop_cdvdfsv"

CCdvdfsv::CCdvdfsv(CSifMan& sif, uint8* iopRam)
: m_iopRam(iopRam)
{
	m_module592 = CSifModuleAdapter(std::bind(&CCdvdfsv::Invoke592, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	m_module593 = CSifModuleAdapter(std::bind(&CCdvdfsv::Invoke593, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4, std::placeholders::_5, std::placeholders::_6));
	m_module595 = CSifModuleAdapter(std::bind(&CCdvdfsv::Invoke595, this,
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
	sif.RegisterModule(MODULE_ID_6, &m_module597);
	sif.RegisterModule(MODULE_ID_7, &m_module59A);
	sif.RegisterModule(MODULE_ID_8, &m_module59C);
}

CCdvdfsv::~CCdvdfsv()
{

}

std::string CCdvdfsv::GetId() const
{
	return "cdvdfsv";
}

std::string CCdvdfsv::GetFunctionName(unsigned int) const
{
	return "unknown";
}

void CCdvdfsv::ProcessCommands(CSifMan* sifMan)
{
	if(m_pendingCommand != COMMAND_NONE)
	{
		static const uint32 sectorSize = 0x800;

		if(m_pendingCommand == COMMAND_READ)
		{
			auto sifManPs2 = dynamic_cast<CSifManPs2*>(sifMan);
			assert(sifManPs2 != nullptr);

			uint8* eeRam = sifManPs2->GetEeRam();

			if(m_iso != nullptr)
			{
				for(unsigned int i = 0; i < m_pendingReadCount; i++)
				{
					m_iso->ReadBlock(m_pendingReadSector + i, eeRam + (m_pendingReadAddr + (i * sectorSize)));
				}
			}
		}
		else if(m_pendingCommand == COMMAND_READIOP)
		{
			if(m_iso != nullptr)
			{
				for(unsigned int i = 0; i < m_pendingReadCount; i++)
				{
					m_iso->ReadBlock(m_pendingReadSector + i, m_iopRam + (m_pendingReadAddr + (i * sectorSize)));
				}
			}
		}

		m_pendingCommand = COMMAND_NONE;
		sifMan->SendCallReply(MODULE_ID_4, nullptr);
	}
}

void CCdvdfsv::SetIsoImage(CISO9660* iso)
{
	m_iso = iso;
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
				ret[0x03] = 0xFF;
			}
			CLog::GetInstance().Print(LOG_NAME, "Init(mode = %d);\r\n", mode);
		}
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown method invoked (0x%0.8X, 0x%0.8X).\r\n", 0x592, method);
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

			time_t currentTime = time(0);
			tm* localTime = localtime(&currentTime);
			uint8* clockResult = reinterpret_cast<uint8*>(ret + 1);
			clockResult[0] = 0x01;											//Status ?
			clockResult[1] = static_cast<uint8>(localTime->tm_sec);			//Seconds
			clockResult[2] = static_cast<uint8>(localTime->tm_min);			//Minutes
			clockResult[3] = static_cast<uint8>(localTime->tm_hour);		//Hour
			clockResult[4] = 0;												//Padding
			clockResult[5] = static_cast<uint8>(localTime->tm_mday);		//Day
			clockResult[6] = static_cast<uint8>(localTime->tm_mon);			//Month
			clockResult[7] = static_cast<uint8>(localTime->tm_year % 100);	//Year

			ret[0x00] = 0x00;
		}
		break;

	case 0x03:
		assert(retSize >= 4);
		CLog::GetInstance().Print(LOG_NAME, "GetDiskType();\r\n");
		//Returns PS2DVD for now.
		ret[0x00] = 0x14;
		break;

	case 0x04:
		assert(retSize >= 4);
		CLog::GetInstance().Print(LOG_NAME, "GetError();\r\n");
		ret[0x00] = 0x00;
		break;

	case 0x0C:
		//Status
		//Returns
		//0 - Stopped
		//1 - Open
		//2 - Spin
		//3 - Read
		//and more...
		assert(retSize >= 4);
		CLog::GetInstance().Print(LOG_NAME, "Status();\r\n");
		ret[0x00] = 0;
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

	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown method invoked (0x%0.8X, 0x%0.8X).\r\n", 0x593, method);
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
			CLog::GetInstance().Print(LOG_NAME, "GetToc(buffer = 0x%0.8X);\r\n", nBuffer);
			ret[0x00] = 1;
		}
		break;

	case 0x05:
		{
			assert(argsSize >= 4);
			uint32 seekSector = args[0];
			CLog::GetInstance().Print(LOG_NAME, "Seek(sector = 0x%0.8X);\r\n", seekSector);
		}
		break;

	case 0x09:
		StreamCmd(args, argsSize, ret, retSize, ram);
		break;

	case 0x0D:
		ReadIopMem(args, argsSize, ret, retSize, ram);
		return false;
		break;

	case 0x0E:
		//DiskReady (returns 2 if ready, 6 if not ready)
		assert(retSize >= 4);
		CLog::GetInstance().Print(LOG_NAME, "NDiskReady();\r\n");
		if(m_pendingCommand != COMMAND_NONE)
		{
			ret[0x00] = 6;
		}
		else
		{
			ret[0x00] = 2;
		}
		break;

	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown method invoked (0x%0.8X, 0x%0.8X).\r\n", 0x595, method);
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
		CLog::GetInstance().Print(LOG_NAME, "Unknown method invoked (0x%0.8X, 0x%0.8X).\r\n", 0x597, method);
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
		CLog::GetInstance().Print(LOG_NAME, "Unknown method invoked (0x%0.8X, 0x%0.8X).\r\n", 0x59C, method);
		break;
	}
	return true;
}

void CCdvdfsv::Read(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	uint32 sector	= args[0x00];
	uint32 count	= args[0x01];
	uint32 dstAddr	= args[0x02];
	uint32 mode		= args[0x03];

	CLog::GetInstance().Print(LOG_NAME, "Read(sector = 0x%0.8X, count = 0x%0.8X, addr = 0x%0.8X, mode = 0x%0.8X);\r\n",
		sector, count, dstAddr, mode);

	//We write the result now, but ideally should be only written
	//when pending read is completed
	if(retSize >= 4)
	{
		ret[0] = 0;
	}

	assert(m_pendingCommand == COMMAND_NONE);
	m_pendingCommand = COMMAND_READ;
	m_pendingReadSector = sector;
	m_pendingReadCount = count;
	m_pendingReadAddr = dstAddr & 0x1FFFFFFF;
}

void CCdvdfsv::ReadIopMem(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	uint32 sector	= args[0x00];
	uint32 count	= args[0x01];
	uint32 dstAddr	= args[0x02];
	uint32 mode		= args[0x03];

	CLog::GetInstance().Print(LOG_NAME, "ReadIopMem(sector = 0x%0.8X, count = 0x%0.8X, addr = 0x%0.8X, mode = 0x%0.8X);\r\n",
		sector, count, dstAddr, mode);

	if(retSize >= 4)
	{
		ret[0] = 0;
	}

	assert(m_pendingCommand == COMMAND_NONE);
	m_pendingCommand = COMMAND_READIOP;
	m_pendingReadSector = sector;
	m_pendingReadCount = count;
	m_pendingReadAddr = dstAddr & 0x1FFFFFFF;
}

void CCdvdfsv::StreamCmd(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	uint32 sector	= args[0x00];
	uint32 count	= args[0x01];
	uint32 dstAddr	= args[0x02];
	uint32 cmd		= args[0x03];
	uint32 mode		= args[0x04];

	CLog::GetInstance().Print(LOG_NAME, "StreamCmd(sector = 0x%0.8X, count = 0x%0.8X, addr = 0x%0.8X, cmd = 0x%0.8X, mode = 0x%0.8X);\r\n",
		sector, count, dstAddr, cmd, mode);

	switch(cmd)
	{
	case 1:
		//Start
		m_streamPos = sector;
		ret[0] = 1;
		CLog::GetInstance().Print(LOG_NAME, "StreamStart(pos = 0x%0.8X);\r\n", sector);
		break;
	case 2:
		//Read
		dstAddr &= (PS2::EE_RAM_SIZE - 1);

		if(m_iso != NULL)
		{
			for(unsigned int i = 0; i < count; i++)
			{
				m_iso->ReadBlock(m_streamPos, ram + (dstAddr + (i * 0x800)));
				m_streamPos++;
			}
		}

		ret[0] = count;
		CLog::GetInstance().Print(LOG_NAME, "StreamRead(count = 0x%0.8X, dest = 0x%0.8X);\r\n",
			count, dstAddr);
		break;
	case 3:
		//Stop
		ret[0] = 1;
		CLog::GetInstance().Print(LOG_NAME, "StreamStop();\r\n");
		break;
	case 5:
		//Init
		ret[0] = 1;
		CLog::GetInstance().Print(LOG_NAME, "StreamInit(bufsize = 0x%0.8X, numbuf = 0x%0.8X, buf = 0x%0.8X);\r\n",
			sector, count, dstAddr);
		break;
	case 4:
	case 9:
		//Seek
		m_streamPos = sector;
		ret[0] = 1;
		CLog::GetInstance().Print(LOG_NAME, "StreamSeek(pos = 0x%0.8X);\r\n", sector);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown stream command used.\r\n");
		break;
	}
}

void CCdvdfsv::SearchFile(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	uint32 pathOffset = 0x24;
	if(argsSize == 0x128)
	{
		pathOffset = 0x24;
	}
	else if(argsSize == 0x124)
	{
		pathOffset = 0x20;
	}
	else
	{
		CLog::GetInstance().Print(LOG_NAME, "Warning: Using unknown structure size (%d bytes);\r\n", argsSize);
	}

	assert(retSize == 4);

	if(m_iso == NULL)
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
	CLog::GetInstance().Print(LOG_NAME, "SearchFile(path = %s);\r\n", path);

	//Fix all slashes
	std::string fixedPath(path);
	{
		auto slashPos = fixedPath.find('\\');
		while(slashPos != std::string::npos)
		{
			fixedPath[slashPos] = '/';
			slashPos = fixedPath.find('\\', slashPos + 1);
		}
	}

	//Hack to remove any superfluous version extensions (ie.: ;1) that might be present in the path
	//Don't know if this is valid behavior but shouldn't hurt compatibility. This was done for Sengoku Musou 2.
	while(1)
	{
		auto semColCount = std::count(fixedPath.begin(), fixedPath.end(), ';');
		if(semColCount <= 1) break;
		auto semColPos = fixedPath.rfind(';');
		assert(semColPos != std::string::npos);
		fixedPath = std::string(fixedPath.begin(), fixedPath.begin() + semColPos);
	}

	ISO9660::CDirectoryRecord record;
	if(!m_iso->GetFileRecord(&record, fixedPath.c_str()))
	{
		ret[0] = 0;
		return;
	}

	args[0x00] = record.GetPosition();
	args[0x01] = record.GetDataLength();

	ret[0] = 1;
}
