#ifndef _SPU2_CORE_H_
#define _SPU2_CORE_H_

#include "Types.h"
#include <string>

namespace PS2
{
	namespace Spu2
	{
		class CCore
		{
		public:
							CCore(unsigned int);
			virtual			~CCore();

			uint32			ReadRegister(uint32, uint32);
			uint32			WriteRegister(uint32, uint32);

			enum REGISTERS
			{
				A_TSA_HI	= 0x1F9001A8,
				A_TSA_LO	= 0x1F9001AA,
				A_STD		= 0x1F9001AC,
				STATX		= 0x1F900344,
			};

		private:
			void            LogRead(uint32);
			void			LogWrite(uint32, uint16);

			unsigned int    m_coreId;
			std::string		m_logName;
		};
	};
};

#endif
