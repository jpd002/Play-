#ifndef _SPU2_H_
#define _SPU2_H_

#include <vector>
#include "Spu2_Core.h"

namespace PS2
{
	class CSpu2
	{
	public:
						CSpu2(uint32);
		virtual			~CSpu2();

		uint32			ReadRegister(uint32);
		uint32			WriteRegister(uint32, uint32);

        enum
        {
            REGS_BEGIN  = 0x1F900000,
            REGS_END    = 0x1F90FFFF,
        };

	private:
		typedef std::vector<Spu2::CCore> CoreArrayType;

		void            LogRead(uint32);
		void			LogWrite(uint32, uint32);

		uint32          m_baseAddress;
		CoreArrayType   m_cores;
	};
}

#endif
