#pragma once

#include "Iop_FileIo.h"

namespace Iop
{
	class CFileIoHandler1000 : public CFileIo::CHandler
	{
	public:
		CFileIoHandler1000(CIopBios&, uint8*, CIoman*, CSifMan&);
		virtual ~CFileIoHandler1000() = default;

		void AllocateMemory() override;
		void ReleaseMemory() override;

		void Invoke(CMIPS&, uint32) override;
		bool Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*) override;

		void LoadState(Framework::CZipArchiveReader&) override;
		void SaveState(Framework::CZipArchiveWriter&) const override;

	private:
		enum
		{
			TRAMPOLINE_SIZE = 0x80,
			BUFFER_SIZE = 0x400,
		};

		struct MODULEDATA
		{
			uint8 trampoline[TRAMPOLINE_SIZE];
			uint8 buffer[BUFFER_SIZE];
			uint32 method;
			uint32 handle;
			uint32 resultAddr;
			uint32 eeBufferAddr;
			uint32 size;
			uint32 bytesProcessed;
		};

		void LaunchOpenRequest(uint32*, uint32, uint32*, uint32, uint8*);
		void LaunchCloseRequest(uint32*, uint32, uint32*, uint32, uint8*);
		void LaunchReadRequest(uint32*, uint32, uint32*, uint32, uint8*);
		void LaunchSeekRequest(uint32*, uint32, uint32*, uint32, uint8*);

		std::pair<bool, int32> FinishReadRequest(MODULEDATA*, uint8*, int32);

		void ExecuteRequest(CMIPS&);
		void FinishRequest(CMIPS&);
		void BuildExportTable();

		CIopBios& m_bios;
		uint8* m_iopRam = nullptr;
		CSifMan& m_sifMan;
		uint32 m_moduleDataAddr = 0;
		uint32 m_trampolineAddr = 0;
		uint32 m_bufferAddr = 0;
	};
}
