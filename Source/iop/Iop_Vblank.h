#pragma once

#include "Iop_Module.h"

class CIopBios;

namespace Iop
{
	class CVblank : public CModule
	{
	public:
		CVblank(CIopBios&);
		virtual ~CVblank() = default;

		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;
		void Invoke(CMIPS&, unsigned int) override;

	private:
		int32 WaitVblankStart();
		int32 WaitVblankEnd();
		int32 WaitVblank();
		int32 WaitNonVblank();
		int32 RegisterVblankHandler(uint32, uint32, uint32, uint32);
		int32 ReleaseVblankHandler(uint32, uint32);

		CIopBios& m_bios;
	};
}
