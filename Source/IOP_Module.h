#ifndef _IOP_MODULE_H_
#define _IOP_MODULE_H_

#include "Types.h"
#include "Stream.h"

namespace IOP
{
	class CModule
	{
	public:
		virtual			~CModule() {}
		virtual void	Invoke(uint32, void*, uint32, void*, uint32) = 0;
		virtual void	SaveState(Framework::CStream*) = 0;
		virtual void	LoadState(Framework::CStream*) = 0;
	};
};

#endif
