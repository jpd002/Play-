#pragma once

#include "PspModule.h"

namespace Psp
{
	class CKernelLibrary : public CModule
	{
	public:
		std::string GetName() const override;
		void Invoke(uint32, CMIPS&) override;
	};
}
