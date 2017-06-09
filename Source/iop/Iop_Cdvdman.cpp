#include "../Log.h"
#include "../RegisterStateFile.h"
#include "IopBios.h"
#include "Iop_Cdvdman.h"

#define LOG_NAME			"iop_cdvdman"

#define STATE_FILENAME			("iop_cdvdman/state.xml")
#define STATE_CALLBACK_ADDRESS	("CallbackAddress")
#define STATE_STATUS			("Status")

#define FUNCTION_CDINIT				"CdInit"
#define FUNCTION_CDREAD				"CdRead"
#define FUNCTION_CDSEEK				"CdSeek"
#define FUNCTION_CDGETERROR			"CdGetError"
#define FUNCTION_CDSEARCHFILE		"CdSearchFile"
#define FUNCTION_CDSYNC				"CdSync"
#define FUNCTION_CDGETDISKTYPE		"CdGetDiskType"
#define FUNCTION_CDDISKREADY		"CdDiskReady"
#define FUNCTION_CDTRAYREQ			"CdTrayReq"
#define FUNCTION_CDREADCLOCK		"CdReadClock"
#define FUNCTION_CDSTATUS			"CdStatus"
#define FUNCTION_CDCALLBACK			"CdCallback"
#define FUNCTION_CDSTINIT			"CdStInit"
#define FUNCTION_CDSTREAD			"CdStRead"
#define FUNCTION_CDSTSTART			"CdStStart"
#define FUNCTION_CDSTSTOP			"CdStStop"
#define FUNCTION_CDSETMMODE			"CdSetMmode"
#define FUNCTION_CDSTSEEKF			"CdStSeekF"
#define FUNCTION_CDREADDVDDUALINFO	"CdReadDvdDualInfo"
#define FUNCTION_CDLAYERSEARCHFILE	"CdLayerSearchFile"

using namespace Iop;

CCdvdman::CCdvdman(CIopBios& bios, uint8* ram)
: m_bios(bios)
, m_ram(ram)
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

static uint8 Uint8ToBcd(uint8 input)
{
	uint8 digit0 = input % 10;
	uint8 digit1 = (input / 10) % 10;
	return digit0 | (digit1 << 4);
}

uint32 CCdvdman::CdReadClockDirect(uint8* clockBuffer)
{
	auto currentTime = time(0);
	auto localTime = localtime(&currentTime);
	clockBuffer[0] = 0;                                                           //Status (0 = ok, anything else = error)
	clockBuffer[1] = Uint8ToBcd(static_cast<uint8>(localTime->tm_sec));           //Seconds
	clockBuffer[2] = Uint8ToBcd(static_cast<uint8>(localTime->tm_min));           //Minutes
	clockBuffer[3] = Uint8ToBcd(static_cast<uint8>(localTime->tm_hour));          //Hour
	clockBuffer[4] = 0;                                                           //Padding
	clockBuffer[5] = Uint8ToBcd(static_cast<uint8>(localTime->tm_mday));          //Day
	clockBuffer[6] = Uint8ToBcd(static_cast<uint8>(localTime->tm_mon + 1));       //Month
	clockBuffer[7] = Uint8ToBcd(static_cast<uint8>(localTime->tm_year % 100));    //Year
	//Returns 1 on success and 0 on error
	return 1;
}

uint32 CCdvdman::CdGetDiskTypeDirect(COpticalMedia* opticalMedia)
{
	//Assert just to make sure that we're not handling different optical medias
	//(Only one can be inserted at once)
	assert(m_opticalMedia == opticalMedia);
	switch(m_opticalMedia->GetTrackDataType(0))
	{
	case COpticalMedia::TRACK_DATA_TYPE_MODE2_2352:
		return CCdvdman::CDVD_DISKTYPE_PS2CD;
		break;
	case COpticalMedia::TRACK_DATA_TYPE_MODE1_2048:
	default:
		return CCdvdman::CDVD_DISKTYPE_PS2DVD;
		break;
	}
}

std::string CCdvdman::GetId() const
{
	return "cdvdman";
}

std::string CCdvdman::GetFunctionName(unsigned int functionId) const
{
	switch(functionId)
	{
	case 4:
		return FUNCTION_CDINIT;
		break;
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
	case 14:
		return FUNCTION_CDTRAYREQ;
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
	case 56:
		return FUNCTION_CDSTINIT;
		break;
	case 57:
		return FUNCTION_CDSTREAD;
		break;
	case 59:
		return FUNCTION_CDSTSTART;
		break;
	case 61:
		return FUNCTION_CDSTSTOP;
		break;
	case 75:
		return FUNCTION_CDSETMMODE;
		break;
	case 77:
		return FUNCTION_CDSTSEEKF;
		break;
	case 83:
		return FUNCTION_CDREADDVDDUALINFO;
		break;
	case 84:
		return FUNCTION_CDLAYERSEARCHFILE;
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
	case 4:
		ctx.m_State.nGPR[CMIPS::V0].nV0 = CdInit(ctx.m_State.nGPR[CMIPS::A0].nV0);
		break;
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
	case 14:
		ctx.m_State.nGPR[CMIPS::V0].nV0 = CdTrayReq(
			ctx.m_State.nGPR[CMIPS::A0].nV0,
			ctx.m_State.nGPR[CMIPS::A1].nV0);
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
	case 56:
		ctx.m_State.nGPR[CMIPS::V0].nV0 = CdStInit(
			ctx.m_State.nGPR[CMIPS::A0].nV0,
			ctx.m_State.nGPR[CMIPS::A1].nV0,
			ctx.m_State.nGPR[CMIPS::A2].nV0);
		break;
	case 57:
		ctx.m_State.nGPR[CMIPS::V0].nV0 = CdStRead(
			ctx.m_State.nGPR[CMIPS::A0].nV0,
			ctx.m_State.nGPR[CMIPS::A1].nV0,
			ctx.m_State.nGPR[CMIPS::A2].nV0,
			ctx.m_State.nGPR[CMIPS::A3].nV0);
		break;
	case 59:
		ctx.m_State.nGPR[CMIPS::V0].nV0 = CdStStart(
			ctx.m_State.nGPR[CMIPS::A0].nV0,
			ctx.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case 61:
		ctx.m_State.nGPR[CMIPS::V0].nV0 = CdStStop();
		break;
	case 75:
		ctx.m_State.nGPR[CMIPS::V0].nV0 = CdSetMmode(ctx.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case 77:
		ctx.m_State.nGPR[CMIPS::V0].nV0 = CdStSeekF(
			ctx.m_State.nGPR[CMIPS::A0].nV0);
		break;
	case 83:
		ctx.m_State.nGPR[CMIPS::V0].nV0 = CdReadDvdDualInfo(
			ctx.m_State.nGPR[CMIPS::A0].nV0,
			ctx.m_State.nGPR[CMIPS::A1].nV0);
		break;
	case 84:
		ctx.m_State.nGPR[CMIPS::V0].nV0 = CdLayerSearchFile(
			ctx.m_State.nGPR[CMIPS::A0].nV0,
			ctx.m_State.nGPR[CMIPS::A1].nV0,
			ctx.m_State.nGPR[CMIPS::A2].nV0);
		break;
	default:
		CLog::GetInstance().Print(LOG_NAME, "Unknown function called (%d).\r\n", 
			functionId);
		break;
	}
}

void CCdvdman::SetOpticalMedia(COpticalMedia* opticalMedia)
{
	m_opticalMedia = opticalMedia;
}

uint32 CCdvdman::CdInit(uint32 mode)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CDINIT "(mode = %d);\r\n", mode);
	//Mode
	//0 - Initialize
	//1 - Init & No Check
	//5 - Exit
	return 1;
}

uint32 CCdvdman::CdRead(uint32 startSector, uint32 sectorCount, uint32 bufferPtr, uint32 modePtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CDREAD "(startSector = 0x%X, sectorCount = 0x%X, bufferPtr = 0x%08X, modePtr = 0x%08X);\r\n",
		startSector, sectorCount, bufferPtr, modePtr);
	if(modePtr != 0)
	{
		uint8* mode = &m_ram[modePtr];
		//Does that make sure it's 2048 byte mode?
		assert(mode[2] == 0);
	}
	if(m_opticalMedia && (bufferPtr != 0))
	{
		uint8* buffer = &m_ram[bufferPtr];
		static const uint32 sectorSize = 2048;
		auto fileSystem = m_opticalMedia->GetFileSystem();
		for(unsigned int i = 0; i < sectorCount; i++)
		{
			fileSystem->ReadBlock(startSector + i, buffer);
			buffer += sectorSize;
		}
	}
	if(m_callbackPtr != 0)
	{
		m_bios.TriggerCallback(m_callbackPtr, CDVD_FUNCTION_OPEN, 0);
	}
	m_status = CDVD_STATUS_READING;
	return 1;
}

uint32 CCdvdman::CdSeek(uint32 sector)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CDSEEK "(sector = 0x%X);\r\n",
		sector);
	if(m_callbackPtr != 0)
	{
		m_bios.TriggerCallback(m_callbackPtr, CDVD_FUNCTION_SEEK, 0);
	}
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
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CDSEARCHFILE "(fileInfo = 0x%08X, name = '%s');\r\n",
		fileInfoPtr, name);
#endif

	uint32 result = 0;

	if(m_opticalMedia && name && fileInfo)
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
		auto fileSystem = m_opticalMedia->GetFileSystem();
		if(fileSystem->GetFileRecord(&record, fixedPath.c_str()))
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
	return CdGetDiskTypeDirect(m_opticalMedia);
}

uint32 CCdvdman::CdDiskReady(uint32 mode)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CDDISKREADY "(mode = %i);\r\n",
		mode);
	m_status = CDVD_STATUS_PAUSED;
	return 2;
}

uint32 CCdvdman::CdTrayReq(uint32 mode, uint32 trayCntPtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CDTRAYREQ "(mode = %d, trayCntPtr = 0x%08X);\r\n",
		mode, trayCntPtr);

	auto trayCnt = reinterpret_cast<uint32*>(m_ram + trayCntPtr);
	(*trayCnt) = 0;

	return 1;
}

uint32 CCdvdman::CdReadClock(uint32 clockPtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CDREADCLOCK "(clockPtr = 0x%08X);\r\n",
		clockPtr);

	auto clockBuffer = m_ram + clockPtr;
	return CdReadClockDirect(clockBuffer);
}

uint32 CCdvdman::CdStatus()
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CDSTATUS "();\r\n");
	return m_status;
}

uint32 CCdvdman::CdCallback(uint32 callbackPtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CDCALLBACK "(callbackPtr = 0x%08X);\r\n",
		callbackPtr);

	uint32 oldCallbackPtr = m_callbackPtr;
	m_callbackPtr = callbackPtr;
	return oldCallbackPtr;
}

uint32 CCdvdman::CdStInit(uint32 bufMax, uint32 bankMax, uint32 bufPtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CDSTINIT "(bufMax = %d, bankMax = %d, bufPtr = 0x%08X);\r\n",
		bufMax, bankMax, bufPtr);
	m_streamPos = 0;
	return 1;
}

uint32 CCdvdman::CdStRead(uint32 sectors, uint32 bufPtr, uint32 mode, uint32 errPtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CDSTREAD "(sectors = %d, bufPtr = 0x%08X, mode = %d, errPtr = 0x%08X);\r\n",
		sectors, bufPtr, mode, errPtr);
	auto fileSystem = m_opticalMedia->GetFileSystem();
	for(unsigned int i = 0; i < sectors; i++)
	{
		fileSystem->ReadBlock(m_streamPos, m_ram + (bufPtr + (i * 0x800)));
		m_streamPos++;
	}
	if(errPtr != 0)
	{
		auto err = reinterpret_cast<uint32*>(m_ram + errPtr);
		(*err) = 0; //No error
	}
	return sectors;
}

uint32 CCdvdman::CdStStart(uint32 sector, uint32 modePtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CDSTSTART "(sector = %d, modePtr = 0x%08X);\r\n",
		sector, modePtr);
	m_streamPos = sector;
	return 1;
}

uint32 CCdvdman::CdStStop()
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CDSTSTOP "();\r\n");
	return 1;
}

uint32 CCdvdman::CdSetMmode(uint32 mode)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CDSETMMODE "(mode = %d);\r\n", mode);
	return 1;
}

uint32 CCdvdman::CdStSeekF(uint32 sector)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CDSTSEEKF "(sector = %d);\r\n",
		sector);
	m_streamPos = sector;
	return 1;
}

uint32 CCdvdman::CdReadDvdDualInfo(uint32 onDualPtr, uint32 layer1StartPtr)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CDREADDVDDUALINFO "(onDualPtr = 0x%08X, layer1StartPtr = 0x%08X);\r\n",
		onDualPtr, layer1StartPtr);

	auto onDual = reinterpret_cast<uint32*>(m_ram + onDualPtr);
	auto layer1Start = reinterpret_cast<uint32*>(m_ram + layer1StartPtr);
	(*onDual) = m_opticalMedia->GetDvdIsDualLayer() ? 1 : 0;
	(*layer1Start) = m_opticalMedia->GetDvdSecondLayerStart() - 0x10;

	return 1;
}

uint32 CCdvdman::CdLayerSearchFile(uint32 fileInfoPtr, uint32 namePtr, uint32 layer)
{
	CLog::GetInstance().Print(LOG_NAME, FUNCTION_CDLAYERSEARCHFILE "(fileInfoPtr = 0x%08X, namePtr = 0x%08X, layer = %d);\r\n",
		fileInfoPtr, namePtr, layer);
	assert(layer == 0);
	return CdSearchFile(fileInfoPtr, namePtr);
}
