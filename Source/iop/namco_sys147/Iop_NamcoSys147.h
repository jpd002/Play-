#pragma once

#include "../Iop_Module.h"
#include "../Iop_SifMan.h"
#include "../../SifModuleAdapter.h"
#include "../../PadInterface.h"
#include "http/HttpServer.h"

namespace Iop
{
	namespace Namco
	{
		class CSys147 : public CModule, public CPadInterface
		{
		public:
			enum class IO_MODE
			{
				DEFAULT,
				AI,
			};

			CSys147(CSifMan&, const std::string&);
			virtual ~CSys147() = default;

			std::string GetId() const override;
			std::string GetFunctionName(unsigned int) const override;
			void Invoke(CMIPS&, unsigned int) override;

			void SetIoMode(IO_MODE);
			void SetButton(unsigned int, unsigned int, PS2::CControllerInfo::BUTTON);

			//CPadInterface
			void SetButtonState(unsigned int, PS2::CControllerInfo::BUTTON, bool, uint8*) override;
			void SetAxisState(unsigned int, PS2::CControllerInfo::BUTTON, uint8, uint8*) override;
			void GetVibration(unsigned int, uint8&, uint8&) override{};

		private:
			using ButtonSelector = std::pair<int, PS2::CControllerInfo::BUTTON>;

			enum MODULE_ID
			{
				MODULE_ID_000 = 0x01470000,
				MODULE_ID_001 = 0x01470001,
				MODULE_ID_002 = 0x01470002,
				MODULE_ID_003 = 0x01470003,
				MODULE_ID_200 = 0x01470200, //S147RPC_REQID_SRAM_WRITE
				MODULE_ID_201 = 0x01470201, //S147RPC_REQID_SRAM_READ
				MODULE_ID_99 = 0x00014799,  //S147LINK
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

			void ProcessIcCard(MODULE_99_PACKET&, const MODULE_99_PACKET&);
			uint8 ComputePacketChecksum(const MODULE_99_PACKET&);

			static constexpr int32 BACKUP_RAM_SIZE = 0x20000;

			void ReadBackupRam(uint32, uint8*, uint32);
			void WriteBackupRam(uint32, const uint8*, uint32);

			void HandleIoServerRequest(const Framework::CHttpServer::Request&);

			CSifModuleAdapter m_module000;
			CSifModuleAdapter m_module001;
			CSifModuleAdapter m_module002;
			CSifModuleAdapter m_module003;
			CSifModuleAdapter m_module200;
			CSifModuleAdapter m_module201;
			CSifModuleAdapter m_module99;

			std::string m_gameId;
			std::map<ButtonSelector, uint8> m_switchBindings;
			IO_MODE m_ioMode = IO_MODE::DEFAULT;

			std::vector<MODULE_99_PACKET> m_pendingReplies;
			std::map<uint8, uint8> m_switchStates;

			//AI board state
			uint16 m_systemSwitchState = 0xFFFF;
			uint16 m_playerSwitchState = 0xFFFF;

			std::unique_ptr<Framework::CHttpServer> m_ioServer;
			std::mutex m_barcodeMutex;
			std::string m_currentBarcode;
		};
	}
}
