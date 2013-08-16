#include <assert.h>
#include "../Log.h"
#include "../Ps2Const.h"
#include "IOP_Cdvdfsv.h"

using namespace Iop;

#define LOG_NAME "iop_cdvdfsv"

CCdvdfsv::CCdvdfsv(CSifMan& sif, uint8* iopRam)
: m_nStreamPos(0)
, m_iso(NULL)
, m_iopRam(iopRam)
, m_delayReadSuccess(false)
, m_lastReadSector(0)
, m_lastReadCount(0)
, m_lastReadAddr(0)
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

void CCdvdfsv::SetIsoImage(CISO9660* iso)
{
	m_iso = iso;
}

void CCdvdfsv::SetReadToEeRamHandler(const ReadToEeRamHandler& readToEeRamHandler)
{
	m_readToEeRamHandler = readToEeRamHandler;
}

void CCdvdfsv::Invoke(CMIPS& context, unsigned int functionId)
{
	throw std::runtime_error("Not implemented.");
}

/*
void CCdvdfsv::SaveState(CStream* pStream)
{
	if(m_nID != MODULE_ID_1) return;

	pStream->Write(&m_nStreamPos, 4);
}

void CCdvdfsv::LoadState(CStream* pStream)
{
	if(m_nID != MODULE_ID_1) return;

	pStream->Read(&m_nStreamPos, 4);
}
*/
void CCdvdfsv::Invoke592(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case 0:
		{
			//Init
			assert(argsSize >= 4);
			uint32 nMode = args[0x00];
			if(retSize != 0)
			{
				assert(retSize >= 0x10);
				ret[0x03] = 0xFF;
			}
			CLog::GetInstance().Print(LOG_NAME, "Init(mode = %d);\r\n", nMode);
		}
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown method invoked (0x%0.8X, 0x%0.8X).\r\n", 0x592, method);
		break;
	}
}

void CCdvdfsv::Invoke593(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
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

	case 0x22:
		{
			//Set Media Mode (1 - CDROM, 2 - DVDROM)
			assert(argsSize >= 4);
			assert(retSize >= 4);
			uint32 nMode = args[0x00];
			CLog::GetInstance().Print(LOG_NAME, "SetMediaMode(mode = %i);\r\n", nMode); 
			ret[0x00] = 1;
		}
		break;

	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown method invoked (0x%0.8X, 0x%0.8X).\r\n", 0x593, method);
		break;
	}
}

void CCdvdfsv::Invoke595(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case 0x01:
		Read(args, argsSize, ret, retSize, ram);
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
		break;

	case 0x0E:
		//DiskReady (returns 2 if ready, 6 if not ready)
		assert(retSize >= 4);
		CLog::GetInstance().Print(LOG_NAME, "NDiskReady();\r\n");
		if(m_delayReadSuccess)
		{
			ret[0x00] = 6;
			m_lastReadSector = 0;
			m_lastReadCount = 0;
			m_lastReadAddr = 0;
			m_delayReadSuccess = false;
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
}

void CCdvdfsv::Invoke597(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
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
}

void CCdvdfsv::Invoke59A(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	Invoke59C(method, args, argsSize, ret, retSize, ram);
}

void CCdvdfsv::Invoke59C(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	switch(method)
	{
	case 0:
		{
			//DiskReady (returns 2 if ready, 6 if not ready)
			assert(retSize >= 4);
			assert(argsSize >= 4);
			uint32 nMode = args[0x00];
			CLog::GetInstance().Print(LOG_NAME, "DiskReady(mode = %i);\r\n", nMode);
			ret[0x00] = 2;
		}
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown method invoked (0x%0.8X, 0x%0.8X).\r\n", 0x59C, method);
		break;
	}
}

void CCdvdfsv::Read(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	uint32 nSector		= args[0x00];
	uint32 nCount		= args[0x01];
	uint32 nDstAddr		= args[0x02];
	uint32 nMode		= args[0x03];

	CLog::GetInstance().Print(LOG_NAME, "Read(sector = 0x%0.8X, count = 0x%0.8X, addr = 0x%0.8X, mode = 0x%0.8X);\r\n", \
		nSector,
		nCount,
		nDstAddr,
		nMode);

	static const uint32 sectorSize = 0x800;

	if(m_iso != NULL)
	{
		for(unsigned int i = 0; i < nCount; i++)
		{
			m_iso->ReadBlock(nSector + i, ram + (nDstAddr + (i * sectorSize)));
		}
	}

	if(m_readToEeRamHandler)
	{
		m_readToEeRamHandler(nDstAddr, sectorSize * nCount);
	}

	if(retSize >= 4)
	{
		ret[0] = 0;
	}

	if((nSector == m_lastReadSector) && (nCount == m_lastReadCount) && (nDstAddr == m_lastReadAddr))
	{
		//Induce a fake read delay because Wild Arms: Alter Code F relies on that to determine read success.
		CLog::GetInstance().Print(LOG_NAME, "Read: Faking delay because we're trying to read the same way twice.\r\n");
		m_delayReadSuccess = true;
	}

	m_lastReadSector = nSector;
	m_lastReadCount = nCount;
	m_lastReadAddr = nDstAddr;
}

void CCdvdfsv::ReadIopMem(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	uint32 nSector		= args[0x00];
	uint32 nCount		= args[0x01];
	uint32 nDstAddr		= args[0x02];
	uint32 nMode		= args[0x03];

	CLog::GetInstance().Print(LOG_NAME, "ReadIopMem(sector = 0x%0.8X, count = 0x%0.8X, addr = 0x%0.8X, mode = 0x%0.8X);\r\n", \
		nSector,
		nCount,
		nDstAddr,
		nMode);

	if(m_iso != NULL)
	{
		for(unsigned int i = 0; i < nCount; i++)
		{
			m_iso->ReadBlock(nSector + i, m_iopRam + nDstAddr);
			nDstAddr += 0x800;
		}
	}

	if(retSize >= 4)
	{
		ret[0] = 0;
	}

	if((nSector == m_lastReadSector) && (nCount == m_lastReadCount) && (nDstAddr == m_lastReadAddr))
	{
		//Induce a fake read delay because Wild Arms: Alter Code F relies on that to determine read success.
		CLog::GetInstance().Print(LOG_NAME, "Read: Faking delay because we're trying to read the same way twice.\r\n");
		m_delayReadSuccess = true;
	}

	m_lastReadSector = nSector;
	m_lastReadCount = nCount;
	m_lastReadAddr = nDstAddr;
}

void CCdvdfsv::StreamCmd(uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram)
{
	uint32 nSector, nCount, nDstAddr, nCmd, nMode;

	nSector		= args[0x00];
	nCount		= args[0x01];
	nDstAddr	= args[0x02];
	nCmd		= args[0x03];
	nMode		= args[0x04];

	CLog::GetInstance().Print(LOG_NAME, "StreamCmd(sector = 0x%0.8X, count = 0x%0.8X, addr = 0x%0.8X, cmd = 0x%0.8X);\r\n", \
		nSector,
		nCount,
		nDstAddr,
		nMode);

	switch(nCmd)
	{
	case 1:
		//Start
		m_nStreamPos = nSector;
		ret[0] = 1;
		
		CLog::GetInstance().Print(LOG_NAME, "StreamStart(pos = 0x%0.8X);\r\n", \
			nSector);
		break;
	case 2:
		//Read
		nDstAddr &= (PS2::EE_RAM_SIZE - 1);

		if(m_iso != NULL)
		{
			for(unsigned int i = 0; i < nCount; i++)
			{
				m_iso->ReadBlock(m_nStreamPos, ram + (nDstAddr + (i * 0x800)));
				m_nStreamPos++;
			}
		}

		ret[0] = nCount;

		CLog::GetInstance().Print(LOG_NAME, "StreamRead(count = 0x%0.8X, dest = 0x%0.8X);\r\n", \
			nCount, \
			nDstAddr);
		break;
	case 5:
		//Init
		ret[0] = 1;

		CLog::GetInstance().Print(LOG_NAME, "StreamInit(bufsize = 0x%0.8X, numbuf = 0x%0.8X, buf = 0x%0.8X);\r\n", \
			nSector, \
			nCount, \
			nDstAddr);
		break;
	case 4:
	case 9:
		//Seek
		m_nStreamPos = nSector;
		ret[0] = 1;

		CLog::GetInstance().Print(LOG_NAME, "StreamSeek(pos = 0x%0.8X);\r\n", \
			nSector);
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

	char* sPath = reinterpret_cast<char*>(args) + pathOffset;

	//Fix all slashes
	std::string fixedPath(sPath);
	{
		std::string::size_type slashPos = fixedPath.find('\\');
		while(slashPos != std::string::npos)
		{
			fixedPath[slashPos] = '/';
			slashPos = fixedPath.find('\\', slashPos + 1);
		}
	}

	CLog::GetInstance().Print(LOG_NAME, "SearchFile(path = %s);\r\n", fixedPath.c_str());

	ISO9660::CDirectoryRecord Record;
	if(!m_iso->GetFileRecord(&Record, fixedPath.c_str()))
	{
		ret[0] = 0;
		return;
	}

	args[0x00] = Record.GetPosition();
	args[0x01] = Record.GetDataLength();

	ret[0] = 1;
}
