#pragma once

#include "../Iop_Module.h"
#include "../Iop_SifMan.h"
#include "../Iop_SifModuleProvider.h"

namespace Iop
{
	namespace Namco
	{
		class CPadMan : public CModule, public CSifModule, public CSifModuleProvider
		{
		public:
			CPadMan() = default;

			std::string GetId() const override;
			std::string GetFunctionName(unsigned int) const override;

			void RegisterSifModules(CSifMan&) override;

			void Invoke(CMIPS&, unsigned int) override;
			bool Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*) override;

			enum MODULE_ID
			{
				MODULE_ID_1 = 0x80000100,
				MODULE_ID_2 = 0x80000101,
				MODULE_ID_3 = 0x8000010F,
				MODULE_ID_4 = 0x8000011F,
			};

		private:
			void Init(uint32*, uint32, uint32*, uint32, uint8*);
			void GetModuleVersion(uint32*, uint32, uint32*, uint32, uint8*);
		};
	}
}
