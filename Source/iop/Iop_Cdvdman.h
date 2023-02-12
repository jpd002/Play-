#pragma once

#include "Iop_Module.h"
#include "../OpticalMedia.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

class CIopBios;

namespace Iop
{
	class CCdvdman : public CModule
	{
	public:
		enum CDVD_TRAQ_REQUEST_MODE
		{
			CDVD_TRAY_OPEN = 0,
			CDVD_TRAY_CLOSE = 1,
			CDVD_TRAY_CHECK = 2,
		};

		enum CDVD_STATUS
		{
			CDVD_STATUS_STOPPED = 0,
			CDVD_STATUS_SPINNING = 2,
			CDVD_STATUS_READING = 6,
			CDVD_STATUS_PAUSED = 10,
			CDVD_STATUS_SEEK = 18,
		};

		enum CDVD_DISKTYPE
		{
			CDVD_DISKTYPE_PS2CD = 0x12,
			CDVD_DISKTYPE_PS2DVD = 0x14,
		};

		struct FILEINFO
		{
			uint32 sector;
			uint32 size;
			char name[16];
			uint8 date[8];
		};

		CCdvdman(CIopBios&, uint8*);
		virtual ~CCdvdman() = default;

		virtual std::string GetId() const override;
		virtual std::string GetFunctionName(unsigned int) const override;
		virtual void Invoke(CMIPS&, unsigned int) override;

		void CountTicks(uint32);
		void SetOpticalMedia(COpticalMedia*);

		void LoadState(Framework::CZipArchiveReader&) override;
		void SaveState(Framework::CZipArchiveWriter&) const override;

		uint32 CdRead(uint32, uint32, uint32, uint32);
		uint32 CdSync(uint32);

		//These won't modify this module's state
		uint32 CdReadClockDirect(uint8*);
		uint32 CdGetDiskTypeDirect(COpticalMedia*);
		uint32 CdLayerSearchFileDirect(COpticalMedia*, FILEINFO*, const char*, uint32);

	private:
		enum COMMAND : uint32
		{
			COMMAND_NONE,
			COMMAND_READ,
			COMMAND_SEEK
		};

		enum CDVD_FUNCTION
		{
			CDVD_FUNCTION_READ = 1,
			CDVD_FUNCTION_SEEK = 4,
			CDVD_FUNCTION_STANDBY = 5,
		};

		uint32 CdInit(uint32);
		uint32 CdStandby();
		uint32 CdSeek(uint32);
		uint32 CdGetError();
		uint32 CdSearchFile(uint32, uint32);
		uint32 CdGetDiskType();
		uint32 CdDiskReady(uint32);
		uint32 CdTrayReq(uint32, uint32);
		uint32 CdReadILinkId(uint32, uint32);
		uint32 CdReadClock(uint32);
		uint32 CdStatus();
		uint32 CdCallback(uint32);
		uint32 CdGetReadPos();
		uint32 CdStInit(uint32, uint32, uint32);
		uint32 CdStRead(uint32, uint32, uint32, uint32);
		uint32 CdStSeek(uint32);
		uint32 CdStStart(uint32, uint32);
		uint32 CdStStat();
		uint32 CdStStop();
		uint32 CdReadModel(uint32, uint32);
		uint32 CdSetMmode(uint32);
		uint32 CdStSeekF(uint32);
		uint32 CdReadDvdDualInfo(uint32, uint32);
		uint32 CdLayerSearchFile(uint32, uint32, uint32);

		CIopBios& m_bios;
		COpticalMedia* m_opticalMedia = nullptr;
		uint8* m_ram = nullptr;

		uint32 m_callbackPtr = 0;
		uint32 m_status = CDVD_STATUS_PAUSED;
		uint32 m_discChanged = 1;
		uint32 m_streamPos = 0;
		uint32 m_streamBufferSize = 0;
		COMMAND m_pendingCommand = COMMAND_NONE;
		int32 m_pendingCommandDelay = 0;
	};

	typedef std::shared_ptr<CCdvdman> CdvdmanPtr;
};
