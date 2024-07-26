#pragma once

#include "../Iop_Module.h"

class CIopBios;

namespace Iop
{
	namespace Namco
	{
		class CAcAta : public CModule
		{
		public:
			CAcAta(CIopBios&, uint8*);
			virtual ~CAcAta() = default;

			std::string GetId() const override;
			std::string GetFunctionName(unsigned int) const override;
			void Invoke(CMIPS&, unsigned int) override;

		private:
			int32 AtaSetup(uint32, uint32, uint32, uint32);
			int32 AtaRequest(uint32, uint32, uint32, uint32, uint32, uint32);

			CIopBios& m_bios;
			uint8* m_ram = nullptr;
		};
	}
}
