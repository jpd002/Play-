#ifndef _IOP_SIFDYNAMIC_H_
#define _IOP_SIFDYNAMIC_H_

#include "Iop_SifMan.h"

namespace Iop
{
	class CSifCmd;

	class CSifDynamic : public CSifModule
	{
	public:
						CSifDynamic(CSifCmd&, uint32);
		virtual			~CSifDynamic();
		virtual bool	Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*);

		uint32			GetServerDataAddress() const;

	private:
		CSifCmd&		m_sifCmd;
		uint32			m_serverDataAddress;
	};
}

#endif
