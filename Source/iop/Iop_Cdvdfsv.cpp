#include <assert.h>
#include "../Log.h"
#include "../Ps2Const.h"
#include "IOP_Cdvdfsv.h"
#include "placeholder_def.h"

using namespace Iop;
using namespace Framework;
using namespace std;
using namespace std::tr1;

#define LOG_NAME "iop_cdvdfsv"

CCdvdfsv::CCdvdfsv(CSifMan& sif) :
m_nStreamPos(0),
m_iso(NULL)
{
    m_module592 = CSifModuleAdapter(bind(&CCdvdfsv::Invoke592, this, 
        PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_3, PLACEHOLDER_4, PLACEHOLDER_5, PLACEHOLDER_6));
    m_module593 = CSifModuleAdapter(bind(&CCdvdfsv::Invoke593, this,
        PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_3, PLACEHOLDER_4, PLACEHOLDER_5, PLACEHOLDER_6));
    m_module595 = CSifModuleAdapter(bind(&CCdvdfsv::Invoke595, this,
        PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_3, PLACEHOLDER_4, PLACEHOLDER_5, PLACEHOLDER_6));
    m_module597 = CSifModuleAdapter(bind(&CCdvdfsv::Invoke597, this,
        PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_3, PLACEHOLDER_4, PLACEHOLDER_5, PLACEHOLDER_6));
    m_module59C = CSifModuleAdapter(bind(&CCdvdfsv::Invoke59C, this,
        PLACEHOLDER_1, PLACEHOLDER_2, PLACEHOLDER_3, PLACEHOLDER_4, PLACEHOLDER_5, PLACEHOLDER_6));

    sif.RegisterModule(MODULE_ID_1, &m_module592);
    sif.RegisterModule(MODULE_ID_2, &m_module593);
    sif.RegisterModule(MODULE_ID_4, &m_module595);
    sif.RegisterModule(MODULE_ID_6, &m_module597);
    sif.RegisterModule(MODULE_ID_7, &m_module59C);
}

CCdvdfsv::~CCdvdfsv()
{

}

string CCdvdfsv::GetId() const
{
    return "cdvdfsv";
}

string CCdvdfsv::GetFunctionName(unsigned int) const
{
    return "unknown";
}

void CCdvdfsv::SetIsoImage(CISO9660* iso)
{
    m_iso = iso;
}

void CCdvdfsv::Invoke(CMIPS& context, unsigned int functionId)
{
    throw runtime_error("Not implemented.");
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
		    assert(retSize >= 0x10);
		    uint32 nMode = args[0x00];
		    CLog::GetInstance().Print(LOG_NAME, "Init(mode = %i);\r\n", nMode);
		    ret[0x03] = 0xFF;
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
	case 1:
		Read(args, argsSize, ret, retSize, ram);
		break;

	case 4:
        {
		    //GetToc
            assert(argsSize >= 4);
		    assert(retSize >= 4);
		    uint32 nBuffer = args[0x00];
		    CLog::GetInstance().Print(LOG_NAME, "GetToc(buffer = 0x%0.8X);\r\n", nBuffer);
		    ret[0x00] = 1;
        }
		break;

	case 9:
		StreamCmd(args, argsSize, ret, retSize, ram);
		break;

	case 0x0E:
		//DiskReady (returns 2 if ready, 6 if not ready)
		assert(retSize >= 4);
		CLog::GetInstance().Print(LOG_NAME, "NDiskReady();\r\n");
		ret[0x00] = 2;
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
	uint32 nSector, nCount, nDstAddr, nMode;

	nSector		= args[0x00];
	nCount		= args[0x01];
	nDstAddr	= args[0x02];
	nMode		= args[0x04];

	CLog::GetInstance().Print(LOG_NAME, "Read(sector = 0x%0.8X, count = 0x%0.8X, addr = 0x%0.8X, mode = 0x%0.8X);\r\n", \
		nSector,
		nCount,
		nDstAddr,
		nMode);

    if(m_iso != NULL)
    {
	    for(unsigned int i = 0; i < nCount; i++)
	    {
		    m_iso->ReadBlock(nSector + i, ram + (nDstAddr + (i * 0x800)));
	    }
    }

	if(retSize >= 4)
	{
		ret[0] = 0;
	}
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
        nDstAddr &= (PS2::EERAMSIZE - 1);

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
	char* sPath;
	char* sTemp;
	char sFixedPath[256];
	ISO9660::CDirectoryRecord Record;

	assert(argsSize >= 0x128);
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

	sPath = reinterpret_cast<char*>(args) + 0x24;

	strcpy(sFixedPath, sPath);

	//Fix all slashes
	sTemp = strchr(sFixedPath, '\\');
	while(sTemp != NULL)
	{
		*sTemp = '/';
		sTemp = strchr(sTemp + 1, '\\');
	}

	CLog::GetInstance().Print(LOG_NAME, "SearchFile(path = %s);\r\n", sFixedPath);

	if(!m_iso->GetFileRecord(&Record, sFixedPath))
	{
		ret[0] = 0;
		return;
	}

	args[0x00] = Record.GetPosition();
	args[0x01] = Record.GetDataLength();

	ret[0] = 1;
}
