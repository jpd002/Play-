#ifndef _PSFLOADER_H_
#define _PSFLOADER_H_

#include "PsfVm.h"
#include "PsfBase.h"
#include "PsfStreamProvider.h"

class CPsxBios;

namespace PS2 { class CPsfBios; }
namespace Psp { class CPsfBios; }

class CPsfLoader
{
public:
    static void     LoadPsf(CPsfVm&, const char*, const char*, CPsfBase::TagMap* = NULL);

private:
	static void     LoadPsx(CPsfVm&, const char*, CPsfStreamProvider*, CPsfBase::TagMap* = NULL);
	static void		LoadPsxRecurse(CPsfVm&, CPsxBios*, const char*, CPsfStreamProvider*, CPsfBase::TagMap* = NULL);

    static void		LoadPs2(CPsfVm&, const char*, CPsfStreamProvider*, CPsfBase::TagMap* = NULL);
	static void		LoadPs2Recurse(CPsfVm&, PS2::CPsfBios*, const char*, CPsfStreamProvider*, CPsfBase::TagMap* = NULL);

	static void		LoadPsp(CPsfVm&, const char*, CPsfStreamProvider*, CPsfBase::TagMap* = NULL);
	static void		LoadPspRecurse(CPsfVm&, Psp::CPsfBios*, const char*, CPsfStreamProvider*, CPsfBase::TagMap* = NULL);
};

#endif
