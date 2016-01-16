#pragma once

#include "Iop_Module.h"
#include "Iop_SifMan.h"
#include "../SifModuleAdapter.h"

namespace Iop
{
	class CMtapMan : public CModule
	{
	public:
		CMtapMan();

		std::string    GetId() const override;
		std::string    GetFunctionName(unsigned int) const override;

		void    RegisterSifModule(CSifMan&);

		void    Invoke(CMIPS&, unsigned int) override;

	private:
		enum MODULE_ID
		{
			MODULE_ID_1 = 0x80000901,
			MODULE_ID_2 = 0x80000902,
			MODULE_ID_3 = 0x80000903,
		};

		bool    Invoke901(uint32, uint32*, uint32, uint32*, uint32, uint8*);
		bool    Invoke902(uint32, uint32*, uint32, uint32*, uint32, uint8*);
		bool    Invoke903(uint32, uint32*, uint32, uint32*, uint32, uint8*);

		uint32    PortOpen(uint32);

		CSifModuleAdapter    m_module901;
		CSifModuleAdapter    m_module902;
		CSifModuleAdapter    m_module903;
	};

	typedef std::shared_ptr<CMtapMan> MtapManPtr;
}
