#pragma once

#include "Iop_Module.h"
#include "Iop_Dmac.h"

class CIopBios;

namespace Iop
{
	class CDmacman : public CModule
	{
	public:
		CDmacman() = default;
		virtual ~CDmacman() = default;

		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;
		void Invoke(CMIPS&, unsigned int) override;

	private:
		void DmacSetDpcr(CMIPS&, uint32);
		uint32 DmacGetDpcr(CMIPS&);
		void DmacSetDpcr2(CMIPS&, uint32);
		uint32 DmacGetDpcr2(CMIPS&);
		void DmacSetDpcr3(CMIPS&, uint32);
		uint32 DmacGetDpcr3(CMIPS&);
		uint32 DmacRequest(CMIPS&, uint32, uint32, uint32, uint32, uint32);
		void DmacTransfer(CMIPS&, uint32);
		void DmacChSetDpcr(CMIPS&, uint32, uint32);
		void DmacEnable(CMIPS&, uint32);
		void DmacDisable(CMIPS&, uint32);
		uint32 GetDPCRAddr(uint32 channel);
	};
}
