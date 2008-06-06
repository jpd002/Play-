#ifndef _PSFLOADER_H_
#define _PSFLOADER_H_

#include "PsxVm.h"

namespace Psx
{
	class CPsfLoader
	{
	public:
		static void		LoadPsf(CPsxVm&, const char*);
	};
}

#endif
