#pragma once

#include "Iop_Module.h"
#include "Iop_SifMan.h"
#include "../SifModuleAdapter.h"
#include "../OpticalMedia.h"

namespace Iop
{
	class CCdvdman;

	class CCdvdfsv : public CModule
	{
	public:
							CCdvdfsv(CSifMan&, CCdvdman&, uint8*);
		virtual				~CCdvdfsv();

		std::string			GetId() const override;
		std::string			GetFunctionName(unsigned int) const override;
		void				Invoke(CMIPS&, unsigned int) override;

		void				ProcessCommands(CSifMan*);
		void				SetOpticalMedia(COpticalMedia*);

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
		enum COMMAND
		{
			COMMAND_NONE,
			COMMAND_READ,
			COMMAND_READIOP,
			COMMAND_STREAM_READ,
		};

		bool				Invoke592(uint32, uint32*, uint32, uint32*, uint32, uint8*);
		bool				Invoke593(uint32, uint32*, uint32, uint32*, uint32, uint8*);
		bool				Invoke595(uint32, uint32*, uint32, uint32*, uint32, uint8*);
		bool				Invoke596(uint32, uint32*, uint32, uint32*, uint32, uint8*);
		bool				Invoke597(uint32, uint32*, uint32, uint32*, uint32, uint8*);
		bool				Invoke59A(uint32, uint32*, uint32, uint32*, uint32, uint8*);
		bool				Invoke59C(uint32, uint32*, uint32, uint32*, uint32, uint8*);

		//Methods
		void				Read(uint32*, uint32, uint32*, uint32, uint8*);
		void				ReadIopMem(uint32*, uint32, uint32*, uint32, uint8*);
		bool				StreamCmd(uint32*, uint32, uint32*, uint32, uint8*);
		void				SearchFile(uint32*, uint32, uint32*, uint32, uint8*);

		CCdvdman&			m_cdvdman;
		uint32				m_streamPos = 0;
		uint8*				m_iopRam = nullptr;
		COpticalMedia*		m_opticalMedia = nullptr;

		COMMAND				m_pendingCommand = COMMAND_NONE;
		uint32				m_pendingReadSector = 0;
		uint32				m_pendingReadCount = 0;
		uint32				m_pendingReadAddr = 0;
		bool				m_streaming = false;
		uint32				m_streamBufferSize = 0;

		CSifModuleAdapter	m_module592;
		CSifModuleAdapter	m_module593;
		CSifModuleAdapter	m_module595;
		CSifModuleAdapter	m_module596;
		CSifModuleAdapter	m_module597;
		CSifModuleAdapter	m_module59A;
		CSifModuleAdapter	m_module59C;
	};

	typedef std::shared_ptr<CCdvdfsv> CdvdfsvPtr;
}
