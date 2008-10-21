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
							CCore(unsigned int, uint32);
			virtual			~CCore();

			uint16			ReadRegister(uint32);
			void			WriteRegister(uint32, uint16);

			enum REGISTERS
			{
				STATX = 0x344,
			};

		private:
			void            LogRead(uint32);
			void			LogWrite(uint32, uint16);

			uint32          m_baseAddress;
			unsigned int    m_coreId;
			std::string		m_logName;
		};
	};
};

#endif
