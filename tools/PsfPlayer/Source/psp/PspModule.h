#ifndef _PSP_MODULE_H_
#define _PSP_MODULE_H_

#include "MIPS.h"
#include <string>

namespace Psp
{
	class CModule
	{
	public:
		virtual ~CModule()
		{
		}
		virtual void Invoke(uint32, CMIPS&) = 0;
		virtual std::string GetName() const = 0;
	};
};

#endif
