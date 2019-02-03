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

		CSifMan();
		virtual ~CSifMan() = default;

		void GenerateHandlers(uint8*, CSysmem&);

		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;
		void Invoke(CMIPS&, unsigned int) override;

		virtual void RegisterModule(uint32, CSifModule*) = 0;
		virtual bool IsModuleRegistered(uint32) = 0;
		virtual void UnregisterModule(uint32) = 0;
		virtual void SendPacket(void*, uint32) = 0;
		virtual void SetDmaBuffer(uint32, uint32) = 0;
		virtual void SetCmdBuffer(uint32, uint32) = 0;
		virtual void SendCallReply(uint32, const void*) = 0;
		virtual void GetOtherData(uint32, uint32, uint32) = 0;
		virtual void SetModuleResetHandler(const ModuleResetHandler&) = 0;
		virtual void SetCustomCommandHandler(const CustomCommandHandler&) = 0;

		virtual uint32 SifSetDma(uint32, uint32);

	protected:
		virtual uint32 SifDmaStat(uint32);
		uint32 SifCheckInit();
		virtual uint32 SifSetDmaCallback(CMIPS&, uint32, uint32, uint32, uint32);

		uint32 m_sifSetDmaCallbackHandlerPtr;
	};

	typedef std::shared_ptr<CSifMan> SifManPtr;
}
