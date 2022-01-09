#pragma once

#include "PS2VM.h"

class CPs2VmJs : public CPS2VM
{
public:
	void CreateVM() override;

	void BootElf(std::string);
	void BootDiscImage(std::string);
};
