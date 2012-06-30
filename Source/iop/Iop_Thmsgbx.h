#ifndef _IOP_THMSGBX_H_
#define _IOP_THMSGBX_H_

#include "Iop_Module.h"
#include "IopBios.h"

namespace Iop
{
	class CThmsgbx : public CModule
	{
	public:
						CThmsgbx(CIopBios&, uint8*);
		virtual			~CThmsgbx();

		std::string		GetId() const;
		std::string		GetFunctionName(unsigned int) const;
		void			Invoke(CMIPS&, unsigned int);

	private:
		struct MSGBX
		{
			uint32	attr;
			uint32	options;
		};

		uint32			CreateMbx(const MSGBX*);
		uint32			SendMbx(uint32, uint32);
		uint32			ReceiveMbx(uint32, uint32);
		uint32			PollMbx(uint32, uint32);

		uint8*			m_ram;
		CIopBios&		m_bios;
	};
}

#endif
