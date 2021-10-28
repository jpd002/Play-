#pragma once

#include "PsfVm.h"
#include "PsfBase.h"

class CPsfVmJs : public CPsfVm
{
public:
	CPsfVmJs();

	CPsfBase::TagMap LoadPsf(std::string, std::string);
};
