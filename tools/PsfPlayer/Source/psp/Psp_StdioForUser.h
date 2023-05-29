#pragma once

#include "PspModule.h"

namespace Psp
{
	class CStdioForUser : public CModule
	{
	public:
		std::string GetName() const override;
		void Invoke(uint32, CMIPS&) override;
	};
}
