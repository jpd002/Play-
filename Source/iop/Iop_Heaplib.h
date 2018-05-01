#pragma once

#include "Iop_Module.h"
#include "Iop_Sysmem.h"

namespace Iop
{
	class CHeaplib : public CModule
	{
	public:
		CHeaplib(CSysmem&);
		virtual ~CHeaplib() = default;

		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;
		void Invoke(CMIPS&, unsigned int) override;

	private:
		int32 CreateHeap(uint32, uint32);
		int32 AllocHeapMemory(uint32, uint32);
		int32 FreeHeapMemory(uint32, uint32);

		CSysmem& m_sysMem;
	};
}
