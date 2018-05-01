#pragma once

#include "Iop_Module.h"

namespace Iop
{
	class CLibSd : public CModule
	{
	public:
		std::string GetId() const override;
		std::string GetFunctionName(unsigned int) const override;
		void Invoke(CMIPS&, unsigned int) override;

		static void TraceCall(CMIPS&, unsigned int);
	};
}
