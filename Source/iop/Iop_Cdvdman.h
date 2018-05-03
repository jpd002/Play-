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

		CCdvdman(CIopBios&, uint8*);
		virtual ~CCdvdman() = default;

		virtual std::string GetId() const override;
		virtual std::string GetFunctionName(unsigned int) const override;
		virtual void Invoke(CMIPS&, unsigned int) override;

		void ProcessCommands();
		void SetOpticalMedia(COpticalMedia*);

		void LoadState(Framework::CZipArchiveReader&);
		void SaveState(Framework::CZipArchiveWriter&);

		uint32 CdReadClockDirect(uint8*);
		uint32 CdGetDiskTypeDirect(COpticalMedia*);

	private:
		enum COMMAND : uint32
		{
			COMMAND_NONE,
			COMMAND_SEEK
		};

		enum CDVD_FUNCTION
		{
			CDVD_FUNCTION_READ = 1,
			CDVD_FUNCTION_SEEK = 4,
		};

		uint32 CdInit(uint32);
		uint32 CdRead(uint32, uint32, uint32, uint32);
		uint32 CdSeek(uint32);
		uint32 CdGetError();
		uint32 CdSearchFile(uint32, uint32);
		uint32 CdSync(uint32);
		uint32 CdGetDiskType();
		uint32 CdDiskReady(uint32);
		uint32 CdTrayReq(uint32, uint32);
		uint32 CdReadClock(uint32);
		uint32 CdStatus();
		uint32 CdCallback(uint32);
		uint32 CdStInit(uint32, uint32, uint32);
		uint32 CdStRead(uint32, uint32, uint32, uint32);
		uint32 CdStStart(uint32, uint32);
		uint32 CdStStat();
		uint32 CdStStop();
		uint32 CdSetMmode(uint32);
		uint32 CdStSeekF(uint32);
		uint32 CdReadDvdDualInfo(uint32, uint32);
		uint32 CdLayerSearchFile(uint32, uint32, uint32);

		CIopBios& m_bios;
		COpticalMedia* m_opticalMedia = nullptr;
		uint8* m_ram = nullptr;

		uint32 m_callbackPtr = 0;
		uint32 m_status = CDVD_STATUS_STOPPED;
		uint32 m_streamPos = 0;
		uint32 m_streamBufferSize = 0;
		COMMAND m_pendingCommand = COMMAND_NONE;
	};

	typedef std::shared_ptr<CCdvdman> CdvdmanPtr;
};
