#ifndef _PSFLOADER_H_
#define _PSFLOADER_H_

#include "PsfVm.h"
#include "PsfBase.h"

class CPsxBios;

namespace PS2 { class CPsfBios; }

class CPsfLoader
{
public:
    static void     LoadPsf(CPsfVm&, const char*, CPsfBase::TagMap* = NULL);

private:
    static void     LoadPsx(CPsfVm&, const char*, CPsfBase::TagMap* = NULL);
	static void		LoadPsxRecurse(CPsfVm&, CPsxBios*, const char*, CPsfBase::TagMap* = NULL);

    static void		LoadPs2(CPsfVm&, const char*, CPsfBase::TagMap* = NULL);
	static void		LoadPs2Recurse(CPsfVm&, PS2::CPsfBios*, const char*, CPsfBase::TagMap* = NULL);
};

#endif
