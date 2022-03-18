#pragma once

#include "Iop_Module.h"
#include "Iop_SifMan.h"

// Not an actual implementation of the PowerOff module.
// For insights what this emulates see here:
// https://github.com/ps2dev/ps2sdk/blob/c80310b3967a0ef3f8cb3cf20e469d7763fe0e9a/iop/dev9/poweroff/src/poweroff.c
// Some homebrew (e.g. ScummVM) expect the module to be loaded beforehand on the IOP
// via the ELF loader (e.g. ps2link), and thus don't load it themselves.
// In the future, this module should only be loaded on ELF files, and not on discs.

namespace Iop
{
	class CPowerOff : public CModule, public CSifModule
	{
	public:
		CPowerOff(Iop::CSifMan& sifMan);
		virtual ~CPowerOff() = default;

		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;
		void Invoke(CMIPS&, unsigned int) override;
		bool Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*) override;

	private:
		enum MODULE_ID
		{
			MODULE_ID = 0x09090900,
		};
	};

	typedef std::shared_ptr<CPowerOff> PowerOffPtr;
}