#pragma once

#include "Iop_Module.h"
#include "IopBios.h"

namespace Iop
{
	class CThfpool : public CModule
	{
	public:
		CThfpool(CIopBios&);
		virtual ~CThfpool() = default;

		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;
		void Invoke(CMIPS&, unsigned int) override;

	private:
		uint32 CreateFpl(uint32);
		uint32 AllocateFpl(uint32);
		uint32 pAllocateFpl(uint32);
		uint32 FreeFpl(uint32, uint32);

		CIopBios& m_bios;
	};
}
