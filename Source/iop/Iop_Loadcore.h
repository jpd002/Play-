#pragma once

#include "Iop_Module.h"
#include "Iop_SifMan.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"
#include <functional>

class CIopBios;

namespace Iop
{
	class CLoadcore : public CModule, public CSifModule
	{
	public:
		enum MODULE_ID
		{
			MODULE_ID = 0x80000006
		};

		typedef std::function<uint32(const char*, const char*)> LoadExecutableHandler;

		CLoadcore(CIopBios&, uint8*, CSifMan&);
		virtual ~CLoadcore() = default;

		void SetModuleVersion(unsigned int);

		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;
		void Invoke(CMIPS&, unsigned int) override;
		bool Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*) override;

		void LoadState(Framework::CZipArchiveReader&) override;
		void SaveState(Framework::CZipArchiveWriter&) const override;

		void SetLoadExecutableHandler(const LoadExecutableHandler&);

	private:
		uint32 GetLibraryEntryTable();
		uint32 RegisterLibraryEntries(uint32);
		int32 ReleaseLibraryEntries(uint32);
		uint32 QueryBootMode(uint32);
		uint32 SetRebootTimeLibraryHandlingMode(uint32, uint32);

		bool LoadModule(uint32*, uint32, uint32*, uint32);
		void LoadExecutable(uint32*, uint32, uint32*, uint32);
		void LoadModuleFromMemory(uint32*, uint32, uint32*, uint32);
		bool StopModule(uint32*, uint32, uint32*, uint32);
		void UnloadModule(uint32*, uint32, uint32*, uint32);
		void SearchModuleByName(uint32*, uint32, uint32*, uint32);
		void Initialize(uint32*, uint32, uint32*, uint32);

		CIopBios& m_bios;
		uint8* m_ram;
		unsigned int m_moduleVersion = 1000;

		LoadExecutableHandler m_loadExecutableHandler;
	};

	typedef std::shared_ptr<CLoadcore> LoadcorePtr;
}
