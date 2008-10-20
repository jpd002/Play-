#ifndef _PSFLOADER_H_
#define _PSFLOADER_H_

#include "PsxVm.h"
#include "ps2/PsfVm.h"
#include "PsfBase.h"

namespace Psx
{
	class CPsfLoader
	{
	public:
		static void		LoadPsf(CPsxVm&, const char*, CPsfBase::TagMap* = NULL);
        static void		LoadPsf2(PS2::CPsfVm&, const char*, CPsfBase::TagMap* = NULL);
	};
}

#endif
