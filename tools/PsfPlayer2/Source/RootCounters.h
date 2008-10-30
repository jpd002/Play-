#ifndef _ROOTCOUNTERS_H_
#define _ROOTCOUNTERS_H_

#include "Types.h"
#include "Convertible.h"
#include <boost/static_assert.hpp>

namespace Iop
{
	class CIntc;
}

namespace Psx
{
	class CRootCounters
	{
	public:
					CRootCounters(unsigned int, Iop::CIntc&);
		virtual		~CRootCounters();

		void		Reset();
		void		Update(unsigned int);

		uint32		ReadRegister(uint32);
		uint32		WriteRegister(uint32, uint32);

		enum
		{
			ADDR_BEGIN		= 0x1F801100,
			ADDR_END		= 0x1F80113F,
		};

		enum
		{
			CNT0_BASE		= 0x1F801100,
			CNT1_BASE		= 0x1F801110,
			CNT2_BASE		= 0x1F801120,
			CNT3_BASE		= 0x1F801130,
		};

		enum
		{
			CNT_COUNT		= 0,
			CNT_MODE		= 4,
			CNT_TARGET		= 8,
		};

		enum
		{
			MAX_COUNTERS	= 4,
		};

	private:
		struct MODE : public convertible<uint32>
		{
			unsigned int en			: 1;
			unsigned int unused0	: 2;
			unsigned int tar		: 1;
			unsigned int iq1		: 1;
			unsigned int unused1	: 1;
			unsigned int iq2		: 1;
			unsigned int unused2	: 1;
			unsigned int clc		: 1;
			unsigned int div		: 1;
			unsigned int unused3	: 22;
		};
		BOOST_STATIC_ASSERT(sizeof(MODE) == sizeof(uint32));
	
		struct COUNTER
		{
			uint16			count;
			MODE			mode;
			uint16			target;
			unsigned int	clockRatio;
			unsigned int	clockRemain;
		};

		void			DisassembleRead(uint32);
		void			DisassembleWrite(uint32, uint32);

		COUNTER			m_counter[MAX_COUNTERS];
		Iop::CIntc&		m_intc;
		unsigned int	m_clockFreq;
	};
}

#endif
