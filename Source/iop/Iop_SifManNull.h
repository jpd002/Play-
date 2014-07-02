#ifndef _IOP_SIFMANNULL_H_
#define _IOP_SIFMANNULL_H_

#include "Iop_SifMan.h"

namespace Iop
{
	class CSifManNull : public CSifMan
	{
	public:
		virtual void	RegisterModule(uint32, CSifModule*) override;
		virtual bool	IsModuleRegistered(uint32) override;
		virtual void	UnregisterModule(uint32) override;
		virtual void	SendPacket(void*, uint32) override;
		virtual void	SetDmaBuffer(uint32, uint32) override;
		virtual void	SendCallReply(uint32, void*) override;
		virtual void	GetOtherData(uint32, uint32, uint32) override;
	};
}

#endif
