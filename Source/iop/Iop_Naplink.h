#pragma once

#include "Iop_Module.h"
#include "Iop_SifMan.h"

namespace Iop
{
	class CIoman;

	class CNaplink : public CModule, public CSifModule
	{
	public:
		        CNaplink(CSifMan&, CIoman&);
		virtual ~CNaplink() = default;

		std::string    GetId() const override;
		std::string    GetFunctionName(unsigned int) const override;
		void           Invoke(CMIPS&, unsigned int) override;
		bool           Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*) override;

	private:
		enum SIF_MODULE_ID
		{
			SIF_MODULE_ID = 0x014D704E
		};

		CIoman& m_ioMan;
	};
}
