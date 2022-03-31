#pragma once

#include "Iop_SifMan.h"
#include "Iop_Module.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

class CIopBios;

namespace Iop
{
	class CIoman;

	class CFileIo : public CSifModule, public CModule
	{
	public:
		class CHandler
		{
		public:
			CHandler(CIoman*);
			virtual ~CHandler() = default;

			virtual void AllocateMemory(){};
			virtual void ReleaseMemory(){};

			virtual void Invoke(CMIPS&, unsigned int);
			virtual bool Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*) = 0;

			virtual void LoadState(Framework::CZipArchiveReader&){};
			virtual void SaveState(Framework::CZipArchiveWriter&) const {};

			virtual void ProcessCommands(CSifMan*){};

		protected:
			CIoman* m_ioman = nullptr;
		};

		enum SIF_MODULE_ID
		{
			SIF_MODULE_ID = 0x80000001
		};

		CFileIo(CIopBios&, uint8*, CSifMan&, CIoman&);

		void SetModuleVersion(unsigned int);

		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;
		void Invoke(CMIPS&, unsigned int) override;
		bool Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*) override;

		void LoadState(Framework::CZipArchiveReader&) override;
		void SaveState(Framework::CZipArchiveWriter&) const override;

		void ProcessCommands(Iop::CSifMan*);

		static const char* g_moduleId;

	private:
		typedef std::unique_ptr<CHandler> HandlerPtr;

		void SyncHandler();

		CIopBios& m_bios;
		uint8* m_ram = nullptr;
		CSifMan& m_sifMan;
		CIoman& m_ioman;
		unsigned int m_moduleVersion = 0;
		HandlerPtr m_handler;
	};

	typedef std::shared_ptr<CFileIo> FileIoPtr;
}
