#pragma once

#include <vector>
#include "Types.h"
#include "ArcadeDriver.h"

class CNamcoSys246Driver : public CArcadeDriver
{
public:
	void PrepareEnvironment(CPS2VM*, const ARCADE_MACHINE_DEF&) override;
	void Launch(CPS2VM*, const ARCADE_MACHINE_DEF&) override;

private:
	std::vector<uint8> ReadDongleData(const ARCADE_MACHINE_DEF&);
};
