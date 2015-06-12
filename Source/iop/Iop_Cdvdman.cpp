#include "../Log.h"
#include "../RegisterStateFile.h"
#include "IopBios.h"
#include "Iop_Cdvdman.h"

#define LOG_NAME			"iop_cdvdman"

#define STATE_FILENAME			("iop_cdvdman/state.xml")
#define STATE_CALLBACK_ADDRESS	("CallbackAddress")
#define STATE_STATUS			("Status")

#define FUNCTION_CDREAD			"CdRead"
#define FUNCTION_CDSEEK			"CdSeek"
#define FUNCTION_CDGETERROR		"CdGetError"
#define FUNCTION_CDSEARCHFILE	"CdSearchFile"
#define FUNCTION_CDSYNC			"CdSync"
#define FUNCTION_CDGETDISKTYPE	"CdGetDiskType"
#define FUNCTION_CDDISKREADY	"CdDiskReady"
#define FUNCTION_CDREADCLOCK	"CdReadClock"
#define FUNCTION_CDSTATUS		"CdStatus"
#define FUNCTION_CDCALLBACK		"CdCallback"

using namespace Iop;

CCdvdman::CCdvdman(CIopBios& bios, uint8* ram)
: m_bios(bios)
, m_ram(ram)
{

}

CCdvdman::~CCdvdman()
{

}

void CCdvdman::LoadState(Framework::CZipArchiveReader& archive)
{
	CRegisterStateFile registerFile(*archive.BeginReadFile(STATE_FILENAME));
	m_callbackPtr = registerFile.GetRegister32(STATE_CALLBACK_ADDRESS);
	m_status = registerFile.GetRegister32(STATE_STATUS);
}

void CCdvdman::SaveState(Framework::CZipArchiveWriter& archive)
{
	auto registerFile = new CRegisterStateFile(STATE_FILENAME);
	registerFile->SetRegister32(STATE_CALLBACK_ADDRESS, m_callbackPtr);
	registerFile->SetRegister32(STATE_STATUS, m_status);
	archive.InsertFile(registerFile);
}

std::string CCdvdman::GetId() const
{
	return "cdvdman";
}

std::string CCdvdman::GetFunctionName(unsigned int functionId) const
{
	switch(functionId)
	{
	case 6:
		return FUNCTION_CDREAD;
		break;
	case 7:
		return FUNCTION_CDSEEK;
		break;
	case 8:
		return FUNCTION_CDGETERROR;
		break;
	case 10:
		return FUNCTION_CDSEARCHFILE;
		break;
	case 11:
		return FUNCTION_CDSYNC;
		break;
	case 12:
		return FUNCTION_CDGETDISKTYPE;
		break;
	case 13:
		return FUNCTION_CDDISKREADY;
		break;
	case 24:
		return FUNCTION_CDREADCLOCK;
		break;
	case 28:
		return FUNCTION_CDSTATUS;
		break;
	case 37:
		return FUNCTION_CDCALLBACK;
		break;
	default:
		return "unknown";
		break;
	}
}

void CCdvdman::Invoke(CMIPS& ctx, unsigned int functionId)
{
	switch(functionId)
	{
	case 6:
		ctx.m_State.nGPR[CMIPS::V0].nV0 = CdRead(
			ctx.m_State.nGPR[CMIPS::A0].nV0,
			ctx.m_State.nGPR[CMIPS::A1].nV0,
			ctx.m_State.nGPR[CMIPS::A2].nV0,
			ctx.m_State.nGPR[CMIPS::A3].nV0);
		break;
	case 7:
		ctx.m_State.nGPR[CMIPS::V0].nV0 = CdSeek(
			ctx.m_State.nGPR[CMIPS::A0].nV0
			);
		break;
	case 8:
		ctx.m_State.nGPR[CMIPS::V0].nV0 = CdGetError();
		break;
	case 10:
		ctx.m_State.nGPR[CMIPS::V0].nV0 = CdSearchFile(
			ctx.m_State.nGPR[CMIPS::A0].nV0,
			ctx.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case 11:
		ctx.m_State.nGPR[CMIPS::V0].nV0 = CdSync(ctx.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case 12:
		ctx.m_State.nGPR[CMIPS::V0].nV0 = CdGetDiskType();
		break;
	case 13:
		ctx.m_State.nGPR[CMIPS::V0].nV0 = CdDiskReady(ctx.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case 24:
		ctx.m_State.nGPR[CMIPS::V0].nV0 = CdReadClock(ctx.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case 28:
		ctx.m_State.nGPR[CMIPS::V0].nV0 = CdStatus();
		break;
	case 37:
		ctx.m_State.nGPR[CMIPS::V0].nV0 = CdCallback(ctx.m_State.nGPR[CMIPS::A0].nV0);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown function called (%d).\r\n", 
			functionId);
		break;
	}
}

void CCdvdman::SetIsoImage(CISO9660* image)
{
	m_image = image;
}

uint32 CCdvdman::CdRead(uint32 startSector, uint32 sectorCount, uint32 bufferPtr, uint32 modePtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CDREAD "(startSector = 0x%X, sectorCount = 0x%X, bufferPtr = 0x%0.8X, modePtr = 0x%0.8X);\r\n",
		startSector, sectorCount, bufferPtr, modePtr);
	if(modePtr != 0)
	{
		uint8* mode = &m_ram[modePtr];
		//Does that make sure it's 2048 byte mode?
		assert(mode[2] == 0);
	}
	if(m_image != NULL && bufferPtr != 0)
	{
		uint8* buffer = &m_ram[bufferPtr];
		uint32 sectorSize = 2048;
		for(unsigned int i = 0; i < sectorCount; i++)
		{
			m_image->ReadBlock(startSector + i, buffer);
			buffer += sectorSize;
		}
	}
	if(m_callbackPtr != 0)
	{
		static const uint32 callbackTypeCdRead = 1;
		m_bios.TriggerCallback(m_callbackPtr, callbackTypeCdRead, 0);
	}
	m_status = CDVD_STATUS_READING;
	return 1;
}

uint32 CCdvdman::CdSeek(uint32 sector)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CDSEEK "(sector = 0x%X);\r\n",
		sector);
	return 1;
}

uint32 CCdvdman::CdGetError()
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CDGETERROR "();\r\n");
	return 0;
}

uint32 CCdvdman::CdSearchFile(uint32 fileInfoPtr, uint32 namePtr)
{
	struct FILEINFO
	{
		uint32		sector;
		uint32		size;
		char		name[16];
		uint8		date[8];
	};

	const char* name = NULL;
	FILEINFO* fileInfo = NULL;

	if(namePtr != 0)
	{
		name = reinterpret_cast<const char*>(m_ram + namePtr);
	}
	if(fileInfoPtr != 0)
	{
		fileInfo = reinterpret_cast<FILEINFO*>(m_ram + fileInfoPtr);
	}

#ifdef _DEBUG
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CDSEARCHFILE "(fileInfo = 0x%0.8X, name = '%s');\r\n",
		fileInfoPtr, name);
#endif

	uint32 result = 0;

	if(m_image != NULL && name != NULL && fileInfo != NULL)
	{
		std::string fixedPath(name);

		//Fix all slashes
		std::string::size_type slashPos = fixedPath.find('\\');
		while(slashPos != std::string::npos)
		{
			fixedPath[slashPos] = '/';
			slashPos = fixedPath.find('\\', slashPos + 1);
		}

		ISO9660::CDirectoryRecord record;
		if(m_image->GetFileRecord(&record, fixedPath.c_str()))
		{
			fileInfo->sector	= record.GetPosition();
			fileInfo->size		= record.GetDataLength();
			strncpy(fileInfo->name, record.GetName(), 16);
			fileInfo->name[15] = 0;
			memset(fileInfo->date, 0, 8);

			result = 1;
		}
	}

	return result;
}

uint32 CCdvdman::CdSync(uint32 mode)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CDSYNC "(mode = %i);\r\n",
		mode);
	if(m_status == CDVD_STATUS_READING)
	{
		m_status = CDVD_STATUS_PAUSED;
	}
	return 0;
}

uint32 CCdvdman::CdGetDiskType()
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CDGETDISKTYPE "();\r\n");
	//0x14 = PS2DVD
	return 0x14;
}

uint32 CCdvdman::CdDiskReady(uint32 mode)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CDDISKREADY "(mode = %i);\r\n",
		mode);
	m_status = CDVD_STATUS_PAUSED;
	return 2;
}

uint32 CCdvdman::CdReadClock(uint32 clockPtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CDREADCLOCK "(clockPtr = %0.8X);\r\n",
		clockPtr);

	time_t currentTime = time(0);
	tm* localTime = localtime(&currentTime);

	uint8* clockResult = m_ram + clockPtr;
	clockResult[0] = 0x01;											//Status ?
	clockResult[1] = static_cast<uint8>(localTime->tm_sec);			//Seconds
	clockResult[2] = static_cast<uint8>(localTime->tm_min);			//Minutes
	clockResult[3] = static_cast<uint8>(localTime->tm_hour);		//Hour
	clockResult[4] = 0;												//Week
	clockResult[5] = static_cast<uint8>(localTime->tm_mday);		//Day
	clockResult[6] = static_cast<uint8>(localTime->tm_mon);			//Month
	clockResult[7] = static_cast<uint8>(localTime->tm_year % 100);	//Year

	return 0;
}

uint32 CCdvdman::CdStatus()
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CDSTATUS "();\r\n");
	return m_status;
}

uint32 CCdvdman::CdCallback(uint32 callbackPtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CDCALLBACK "(callbackPtr = %0.8X);\r\n",
		callbackPtr);

	uint32 oldCallbackPtr = m_callbackPtr;
	m_callbackPtr = callbackPtr;
	return oldCallbackPtr;
}
