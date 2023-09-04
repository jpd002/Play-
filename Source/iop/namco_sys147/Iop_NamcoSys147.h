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
				MODULE_ID_99 = 0x00014799, //S147LINK
			};
			
			struct MODULE_99_PACKET
			{
				uint8 type;
				uint8 unknown[4];
				uint8 command;
				uint8 data[0x39];
				uint8 checksum;
			};
			static_assert(sizeof(MODULE_99_PACKET) == 0x40);

			bool Invoke000(uint32, uint32*, uint32, uint32*, uint32, uint8*);
			bool Invoke001(uint32, uint32*, uint32, uint32*, uint32, uint8*);
			bool Invoke002(uint32, uint32*, uint32, uint32*, uint32, uint8*);
			bool Invoke003(uint32, uint32*, uint32, uint32*, uint32, uint8*);
			bool Invoke200(uint32, uint32*, uint32, uint32*, uint32, uint8*);
			bool Invoke201(uint32, uint32*, uint32, uint32*, uint32, uint8*);
			bool Invoke99(uint32, uint32*, uint32, uint32*, uint32, uint8*);

			CSifModuleAdapter m_module000;
			CSifModuleAdapter m_module001;
			CSifModuleAdapter m_module002;
			CSifModuleAdapter m_module003;
			CSifModuleAdapter m_module200;
			CSifModuleAdapter m_module201;
			CSifModuleAdapter m_module99;
			
			std::vector<MODULE_99_PACKET> m_pendingReplies;
		};
	}
}
