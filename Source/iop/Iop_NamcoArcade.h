#pragma once

#include "Iop_Module.h"
#include "Iop_SifMan.h"
#include "../SifModuleAdapter.h"
#include "../PadListener.h"

namespace Iop
{
	class CNamcoArcade : public CModule, public CPadListener
	{
	public:
		CNamcoArcade(CSifMan&);
		virtual ~CNamcoArcade() = default;

		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;
		void Invoke(CMIPS&, unsigned int) override;

		void SetButtonState(unsigned int, PS2::CControllerInfo::BUTTON, bool, uint8*) override;
		void SetAxisState(unsigned int, PS2::CControllerInfo::BUTTON, uint8, uint8*) override;

	private:
		enum MODULE_ID
		{
			MODULE_ID_1 = 0x76500001,
			MODULE_ID_3 = 0x76500003,
			MODULE_ID_4 = 0x76500004,
		};

		bool Invoke001(uint32, uint32*, uint32, uint32*, uint32, uint8*);
		bool Invoke003(uint32, uint32*, uint32, uint32*, uint32, uint8*);
		bool Invoke004(uint32, uint32*, uint32, uint32*, uint32, uint8*);

		CSifModuleAdapter m_module001;
		CSifModuleAdapter m_module003;
		CSifModuleAdapter m_module004;
		
		uint32 m_recvAddr = 0;
		uint32 m_sendAddr = 0;
	};
}
