#pragma once

#include "PsfVm.h"
#include "PsfBase.h"
#include "PsfStreamProvider.h"

class CPsxBios;

namespace PS2
{
	class CPsfDevice;
}
namespace Psp
{
	class CPsfBios;
}

class CPsfLoader
{
public:
	static void LoadPsf(CPsfVm&, const CPsfPathToken&, const boost::filesystem::path&, CPsfBase::TagMap* = NULL);

private:
	static void LoadPsx(CPsfVm&, const CPsfPathToken&, CPsfStreamProvider*, CPsfBase::TagMap* = NULL);
	static void LoadPsxRecurse(CPsxBios*, CMIPS&, const CPsfPathToken&, CPsfStreamProvider*, CPsfBase::TagMap* = NULL);

	static void LoadPs2(CPsfVm&, const CPsfPathToken&, CPsfStreamProvider*, CPsfBase::TagMap* = NULL);
	static void LoadPs2Recurse(PS2::CPsfDevice*, const CPsfPathToken&, CPsfStreamProvider*, CPsfBase::TagMap* = NULL);

	static void LoadPsp(CPsfVm&, const CPsfPathToken&, CPsfStreamProvider*, CPsfBase::TagMap* = NULL);
	static void LoadPspRecurse(Psp::CPsfBios*, const CPsfPathToken&, CPsfStreamProvider*, CPsfBase::TagMap* = NULL);
};
