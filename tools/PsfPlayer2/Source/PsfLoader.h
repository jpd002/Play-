#ifndef _PSFLOADER_H_
#define _PSFLOADER_H_

#include "PsxVm.h"
#include "PsfBase.h"

namespace Psx
{
	class CPsfLoader
	{
	public:
		static void		LoadPsf(CPsxVm&, const char*, CPsfBase::TagMap* = NULL);
		static void		LoadPsf2(const char*, CPsfBase::TagMap* = NULL);
	};
}

#endif
