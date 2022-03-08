#pragma once

#include <vector>
#include <functional>
#include "Types.h"
#include "BasicUnion.h"
#include "Convertible.h"

namespace Iop
{
	class CIlink
	{
	public:
		enum
		{
			ADDR_BEGIN = 0x1F808400,
			ADDR_END = 0x1F808500
		};

		uint32 ReadRegister(uint32);
		void WriteRegister(uint32, uint32);

	private:
		void LogRead(uint32);
		void LogWrite(uint32, uint32);
	};
}
