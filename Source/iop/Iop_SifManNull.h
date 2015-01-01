#pragma once

#include "Iop_SifMan.h"

namespace Iop
{
	class CSifManNull : public CSifMan
	{
	public:
		void	RegisterModule(uint32, CSifModule*) override;
		bool	IsModuleRegistered(uint32) override;
		void	UnregisterModule(uint32) override;
		void	SendPacket(void*, uint32) override;
		void	SetDmaBuffer(uint32, uint32) override;
		void	SetCmdBuffer(uint32, uint32) override;
		void	SendCallReply(uint32, const void*) override;
		void	GetOtherData(uint32, uint32, uint32) override;
		void	SetCustomCommandHandler(const CustomCommandHandler&) override;
	};
}
