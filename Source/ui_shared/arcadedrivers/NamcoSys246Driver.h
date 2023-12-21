#pragma once

#include "ArcadeDriver.h"

class CNamcoSys246Driver : public CArcadeDriver
{
public:
	void PrepareEnvironment(CPS2VM*, const ARCADE_MACHINE_DEF&) override;
	void Launch(CPS2VM*, const ARCADE_MACHINE_DEF&) override;
};
