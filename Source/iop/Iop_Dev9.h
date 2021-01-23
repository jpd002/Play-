#pragma once

#include "Types.h"

namespace Iop
{
	class CDev9
	{
	public:
		enum
		{
			ADDR_BEGIN = 0x1F801460,
			ADDR_END = 0x1F80147F
		};

		uint32 ReadRegister(uint32);
		void WriteRegister(uint32, uint32);
		
	private:
		enum
		{
			REG_REV = 0x1F80146E,
		};

		void LogRead(uint32);
		void LogWrite(uint32, uint32);
	};
}
