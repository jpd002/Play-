#pragma once

#include "Iop_Module.h"
#include "Iop_SifMan.h"
#include "../SifModuleAdapter.h"
#include "../OpticalMedia.h"
#include "../PadListener.h"

namespace Iop
{
	class CCdvdman;

	class CNamcoArcade : public CModule, public CPadListener
	{
	public:
		CNamcoArcade(CSifMan&, CCdvdman&, uint8*);
		virtual ~CNamcoArcade() = default;

		void SetOpticalMedia(COpticalMedia*);

		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;
		void Invoke(CMIPS&, unsigned int) override;

		void SetButtonState(unsigned int, PS2::CControllerInfo::BUTTON, bool, uint8*) override;
		void SetAxisState(unsigned int, PS2::CControllerInfo::BUTTON, uint8, uint8*) override;

	private:
		enum MODULE_ID
		{
			MODULE_ID_1 = 0x76500001,
			MODULE_ID_2 = 0x76500002,
			MODULE_ID_3 = 0x76500003,
			MODULE_ID_4 = 0x76500004,
		};

		bool Invoke001(uint32, uint32*, uint32, uint32*, uint32, uint8*);
		bool Invoke002(uint32, uint32*, uint32, uint32*, uint32, uint8*);
		bool Invoke003(uint32, uint32*, uint32, uint32*, uint32, uint8*);
		bool Invoke004(uint32, uint32*, uint32, uint32*, uint32, uint8*);

		CCdvdman& m_cdvdman;
		uint8* m_iopRam = nullptr;
		COpticalMedia* m_opticalMedia = nullptr;

		uint32 m_streamPos = 0;
		
		CSifModuleAdapter m_module001;
		CSifModuleAdapter m_module002;
		CSifModuleAdapter m_module003;
		CSifModuleAdapter m_module004;
	};
}
