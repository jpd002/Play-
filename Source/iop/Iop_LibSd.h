#pragma once

#include "Iop_Module.h"

namespace Iop
{
	class CLibSd : public CModule
	{
	public:
		virtual std::string		GetId() const;
		virtual std::string		GetFunctionName(unsigned int) const;
		virtual void			Invoke(CMIPS&, unsigned int);

		static void				TraceCall(CMIPS&, unsigned int);
	};
}
