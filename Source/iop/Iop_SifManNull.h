#ifndef _IOP_SIFMANNULL_H_
#define _IOP_SIFMANNULL_H_

#include "Iop_SifMan.h"

namespace Iop
{
	class CSifManNull : public CSifMan
	{
	public:
		virtual void	RegisterModule(uint32, CSifModule*);
		virtual bool	IsModuleRegistered(uint32);
		virtual void	UnregisterModule(uint32);
		virtual void	SendPacket(void*, uint32);
		virtual void	SetDmaBuffer(uint32, uint32);
		virtual void	SendCallReply(uint32, void*);
		virtual void	GetOtherData(uint32, uint32, uint32);
	};
}

#endif
