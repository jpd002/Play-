#pragma once

#include "../SifModule.h"
#include "../SifDefs.h"
#include "Iop_Module.h"

namespace Iop
{
	class CSysmem;

	class CSifMan : public CModule
	{
	public:
		typedef std::function<void(const std::string&)> ModuleResetHandler;
		typedef std::function<void(uint32)> CustomCommandHandler;

		CSifMan() = default;
		virtual ~CSifMan() = default;

		void PrepareModuleData(uint8*, CSysmem&);
		void CountTicks(int32);

		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;
		void Invoke(CMIPS&, unsigned int) override;

		virtual void RegisterModule(uint32, CSifModule*) = 0;
		virtual bool IsModuleRegistered(uint32) = 0;
		virtual void UnregisterModule(uint32) = 0;
		virtual void SendPacket(const void*, uint32) = 0;
		virtual void SetDmaBuffer(uint32, uint32) = 0;
		virtual void SetCmdBuffer(uint32, uint32) = 0;
		virtual void SendCallReply(uint32, const void*) = 0;
		virtual void GetOtherData(uint32, uint32, uint32) = 0;
		virtual void SetModuleResetHandler(const ModuleResetHandler&) = 0;
		virtual void SetCustomCommandHandler(const CustomCommandHandler&) = 0;

		virtual uint32 SifSetDma(uint32, uint32);

	private:
		uint32 SifDmaStat(uint32);
		uint32 SifCheckInit();
		uint32 SifSetDmaCallback(CMIPS&, uint32, uint32, uint32, uint32);

		enum
		{
			SIFSETDMACALLBACK_HANDLER_SIZE = 0x30,
		};

		struct MODULEDATA
		{
			uint8 sifSetDmaCallbackHandler[SIFSETDMACALLBACK_HANDLER_SIZE];
			int32 dmaTransferTime;
			int32 padding[3];
		};
		static_assert(sizeof(MODULEDATA) == 0x40);

		MODULEDATA* m_moduleData = nullptr;
		uint32 m_sifSetDmaCallbackHandlerAddr = 0;
	};

	typedef std::shared_ptr<CSifMan> SifManPtr;
}
