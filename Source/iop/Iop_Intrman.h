#pragma once

#include "Iop_Module.h"

class CIopBios;

namespace Iop
{
	class CIntrman : public CModule
	{
	public:
		CIntrman(CIopBios&, uint8*);
		virtual ~CIntrman() = default;

		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;
		void Invoke(CMIPS&, unsigned int) override;

	private:
		uint32 RegisterIntrHandler(uint32, uint32, uint32, uint32);
		uint32 ReleaseIntrHandler(uint32);
		uint32 EnableIntrLine(CMIPS&, uint32);
		uint32 DisableIntrLine(CMIPS&, uint32, uint32);
		uint32 EnableInterrupts(CMIPS&);
		uint32 DisableInterrupts(CMIPS&);
		uint32 SuspendInterrupts(CMIPS&, uint32);
		uint32 ResumeInterrupts(CMIPS&, uint32);
		uint32 QueryIntrContext(CMIPS&);
		uint8* m_ram = nullptr;
		CIopBios& m_bios;
	};
}
