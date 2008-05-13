#ifndef _INTC_H_
#define _INTC_H_

#include "Types.h"

namespace Psx
{
	class CIntc
	{
	public:
					CIntc();
		virtual		~CIntc();

		void		Reset();

		void		AssertLine(unsigned int);
		bool		IsInterruptPending() const;

		uint32		ReadRegister(uint32);
		void		WriteRegister(uint32, uint32);

		enum
		{
			LINE_DMAC	= 3,
			LINE_CNT0	= 4,
			LINE_CNT1	= 5,
			LINE_CNT2	= 6,
			LINE_CNT3	= 7,
		};

		enum
		{
			ADDR_BEGIN	= 0x1F801070,
			ADDR_END	= 0x1F801077
		};

		enum
		{
			STATUS	= 0x1F801070,
			MASK	= 0x1F801074
		};

	private:
		void		DisassembleRead(uint32);
		void		DisassembleWrite(uint32, uint32);

		uint32		m_status;
		uint32		m_mask;
	};
}

#endif
