#pragma once

#include "Iop_SifMan.h"
#include "../ee/SIF.h"

namespace Iop
{
	class CSifManPs2 : public CSifMan
	{
	public:
						CSifManPs2(CSIF&, uint8*, uint8*);
		virtual			~CSifManPs2();

		void			RegisterModule(uint32, CSifModule*) override;
		bool			IsModuleRegistered(uint32) override;
		void			UnregisterModule(uint32) override;
		void			SendPacket(void*, uint32) override;
		void			SetDmaBuffer(uint32, uint32) override;
		void			SetCmdBuffer(uint32, uint32) override;
		void			SendCallReply(uint32, const void*) override;
		void			GetOtherData(uint32, uint32, uint32) override;
		void			SetModuleResetHandler(const ModuleResetHandler&) override;
		void			SetCustomCommandHandler(const CustomCommandHandler&) override;

		uint32			SifSetDma(uint32, uint32) override;

		uint8*			GetEeRam() const;

	private:
		CSIF&			m_sif;
		uint8*			m_eeRam;
		uint8*			m_iopRam;
	};
}
