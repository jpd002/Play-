#pragma once

#include "Iop_SifMan.h"
#include "Iop_Module.h"
#include "zip/ZipArchiveWriter.h"
#include "zip/ZipArchiveReader.h"

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
			virtual			~CHandler() = default;
			
			virtual void	Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*) = 0;

		protected:
			CIoman*			m_ioman = nullptr;
		};

		enum SIF_MODULE_ID
		{
			SIF_MODULE_ID	= 0x80000001
		};

								CFileIo(CSifMan&, CIoman&);

		void					SetModuleVersion(unsigned int);

		virtual std::string		GetId() const override;
		virtual std::string		GetFunctionName(unsigned int) const override;
		virtual void			Invoke(CMIPS&, unsigned int) override;
		virtual bool			Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*) override;

		void					LoadState(Framework::CZipArchiveReader&);
		void					SaveState(Framework::CZipArchiveWriter&) const;

	private:
		typedef std::unique_ptr<CHandler> HandlerPtr;

		CSifMan&				m_sifMan;
		CIoman&					m_ioman;
		unsigned int			m_moduleVersion = 0;
		HandlerPtr				m_handler;
	};

	typedef std::shared_ptr<CFileIo> FileIoPtr;
}
