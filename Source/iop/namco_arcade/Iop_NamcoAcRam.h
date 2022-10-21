#pragma once

#include "../Iop_Module.h"

namespace Iop
{
	class CCdvdman;

	namespace Namco
	{
		class CAcRam : public CModule
		{
		public:
			CAcRam(uint8*);
			virtual ~CAcRam() = default;

			std::string GetId() const override;
			std::string GetFunctionName(unsigned int) const override;
			void Invoke(CMIPS&, unsigned int) override;

		private:
			static const uint32_t g_extRamSize = 0x4000000;
			uint8 m_extRam[g_extRamSize];
			uint8* m_iopRam = nullptr;
		};
	}
}
