#pragma once

#include <string>
#include <memory>
#include "../MIPS.h"

namespace Iop
{
	class CModule
	{
	public:
		virtual					~CModule() = default;
		virtual std::string		GetId() const = 0;
		virtual std::string		GetFunctionName(unsigned int) const = 0;
		virtual void			Invoke(CMIPS&, unsigned int) = 0;

	};

	typedef std::shared_ptr<CModule> ModulePtr;
};
