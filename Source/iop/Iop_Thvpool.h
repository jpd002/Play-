#pragma once

#include "Iop_Module.h"
#include "IopBios.h"

namespace Iop
{
	class CThvpool : public CModule
	{
	public:
		CThvpool(CIopBios&);
		virtual ~CThvpool() = default;

		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;
		void Invoke(CMIPS&, unsigned int) override;

	private:
		uint32 CreateVpl(uint32);
		uint32 DeleteVpl(uint32);
		uint32 pAllocateVpl(uint32, uint32);
		uint32 FreeVpl(uint32, uint32);
		uint32 ReferVplStatus(uint32, uint32);

		CIopBios& m_bios;
	};
}
