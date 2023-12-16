#pragma once

#include "ArcadeDriver.h"

class CNamcoSys147Driver : public CArcadeDriver
{
public:
	void PrepareEnvironment(CPS2VM*, const ARCADE_MACHINE_DEF&) override;
	void Launch(CPS2VM*, const ARCADE_MACHINE_DEF&) override;
};

