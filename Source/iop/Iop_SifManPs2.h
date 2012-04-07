#ifndef _IOP_SIFMANPS2_H_
#define _IOP_SIFMANPS2_H_

#include "Iop_SifMan.h"
#include "../Sif.h"

namespace Iop
{
	class CSifManPs2 : public CSifMan
	{
	public:
						CSifManPs2(CSIF&, uint8*, uint8*);
		virtual			~CSifManPs2();

		virtual void	RegisterModule(uint32, CSifModule*);
		virtual void	UnregisterModule(uint32);
		virtual void	SendPacket(void*, uint32);
		virtual void	SetDmaBuffer(uint32, uint32);
		virtual void	SendCallReply(uint32, void*);

	protected:
		virtual uint32	SifSetDma(uint32, uint32);

	private:
		CSIF&			m_sif;
		uint8*			m_eeRam;
		uint8*			m_iopRam;
	};
}

#endif
