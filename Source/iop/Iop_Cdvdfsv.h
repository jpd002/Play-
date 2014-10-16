#pragma once

#include "Iop_Module.h"
#include "Iop_SifMan.h"
#include "../SifModuleAdapter.h"
#include "../ISO9660/ISO9660.h"

namespace Iop
{

	class CCdvdfsv : public CModule
	{
	public:
		typedef std::function<void (uint32, uint32)> ReadToEeRamHandler;

							CCdvdfsv(CSifMan&, uint8*);
		virtual				~CCdvdfsv();

		std::string			GetId() const override;
		std::string			GetFunctionName(unsigned int) const override;
		void				Invoke(CMIPS&, unsigned int) override;

		void				SetIsoImage(CISO9660*);
		void				SetReadToEeRamHandler(const ReadToEeRamHandler&);

		enum MODULE_ID
		{
			MODULE_ID_1 = 0x80000592,
			MODULE_ID_2 = 0x80000593,
			MODULE_ID_3 = 0x80000594,
			MODULE_ID_4 = 0x80000595,
			MODULE_ID_5 = 0x80000596,
			MODULE_ID_6 = 0x80000597,
			MODULE_ID_7 = 0x8000059A,
			MODULE_ID_8 = 0x8000059C,
		};

	private:
		void				Invoke592(uint32, uint32*, uint32, uint32*, uint32, uint8*);
		void				Invoke593(uint32, uint32*, uint32, uint32*, uint32, uint8*);
		void				Invoke595(uint32, uint32*, uint32, uint32*, uint32, uint8*);
		void				Invoke597(uint32, uint32*, uint32, uint32*, uint32, uint8*);
		void				Invoke59A(uint32, uint32*, uint32, uint32*, uint32, uint8*);
		void				Invoke59C(uint32, uint32*, uint32, uint32*, uint32, uint8*);

		//Methods
		void				Read(uint32*, uint32, uint32*, uint32, uint8*);
		void				ReadIopMem(uint32*, uint32, uint32*, uint32, uint8*);
		void				StreamCmd(uint32*, uint32, uint32*, uint32, uint8*);
		void				SearchFile(uint32*, uint32, uint32*, uint32, uint8*);

		uint32				m_streamPos = 0;
		uint8*				m_iopRam = nullptr;
		CISO9660*			m_iso = nullptr;

		bool				m_delayReadSuccess = false;
		uint32				m_lastReadSector = 0;
		uint32				m_lastReadCount = 0;
		uint32				m_lastReadAddr = 0;

		CSifModuleAdapter	m_module592;
		CSifModuleAdapter	m_module593;
		CSifModuleAdapter	m_module595;
		CSifModuleAdapter	m_module597;
		CSifModuleAdapter	m_module59A;
		CSifModuleAdapter	m_module59C;

		ReadToEeRamHandler	m_readToEeRamHandler;
	};
}
