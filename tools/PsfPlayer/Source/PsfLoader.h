#pragma once

#include "PsfVm.h"
#include "PsfBase.h"
#include "PsfStreamProvider.h"

class CPsxBios;

namespace PS2 { class CPsfBios; }
namespace Psp { class CPsfBios; }

class CPsfLoader
{
public:
	static void		LoadPsf(CPsfVm&, const CPsfPathToken&, const boost::filesystem::path&, CPsfBase::TagMap* = NULL);

private:
	static void		LoadPsx(CPsfVm&, const CPsfPathToken&, CPsfStreamProvider*, CPsfBase::TagMap* = NULL);
	static void		LoadPsxRecurse(CPsfVm&, CPsxBios*, const CPsfPathToken&, CPsfStreamProvider*, CPsfBase::TagMap* = NULL);

	static void		LoadPs2(CPsfVm&, const CPsfPathToken&, CPsfStreamProvider*, CPsfBase::TagMap* = NULL);
	static void		LoadPs2Recurse(CPsfVm&, PS2::CPsfBios*, const CPsfPathToken&, CPsfStreamProvider*, CPsfBase::TagMap* = NULL);

	static void		LoadPsp(CPsfVm&, const CPsfPathToken&, CPsfStreamProvider*, CPsfBase::TagMap* = NULL);
	static void		LoadPspRecurse(CPsfVm&, Psp::CPsfBios*, const CPsfPathToken&, CPsfStreamProvider*, CPsfBase::TagMap* = NULL);
};
