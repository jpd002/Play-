#pragma once

#include "../Iop_Module.h"
#include "../Iop_SifMan.h"
#include "../../SifModuleAdapter.h"

namespace Iop
{
	namespace Namco
	{
		class CSys147 : public CModule
		{
		public:
			CSys147(CSifMan&);
			virtual ~CSys147() = default;

			std::string GetId() const override;
			std::string GetFunctionName(unsigned int) const override;
			void Invoke(CMIPS&, unsigned int) override;

		private:
			enum MODULE_ID
			{
				MODULE_ID_000 = 0x01470000,
				MODULE_ID_001 = 0x01470001,
				MODULE_ID_002 = 0x01470002,
				MODULE_ID_003 = 0x01470003,
				MODULE_ID_200 = 0x01470200, //S147RPC_REQID_SRAM_WRITE
				MODULE_ID_201 = 0x01470201, //S147RPC_REQID_SRAM_READ
			};

			bool Invoke000(uint32, uint32*, uint32, uint32*, uint32, uint8*);
			bool Invoke001(uint32, uint32*, uint32, uint32*, uint32, uint8*);
			bool Invoke002(uint32, uint32*, uint32, uint32*, uint32, uint8*);
			bool Invoke003(uint32, uint32*, uint32, uint32*, uint32, uint8*);
			bool Invoke200(uint32, uint32*, uint32, uint32*, uint32, uint8*);
			bool Invoke201(uint32, uint32*, uint32, uint32*, uint32, uint8*);

			CSifModuleAdapter m_module000;
			CSifModuleAdapter m_module001;
			CSifModuleAdapter m_module002;
			CSifModuleAdapter m_module003;
			CSifModuleAdapter m_module200;
			CSifModuleAdapter m_module201;
		};
	}
}
