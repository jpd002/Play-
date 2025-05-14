#pragma once

#include <vector>
#include "Types.h"
#include "filesystem_def.h"
#include "ArcadeDriver.h"

class CNamcoSys246Driver : public CArcadeDriver
{
public:
	void PrepareEnvironment(CPS2VM*, const ARCADE_MACHINE_DEF&) override;
	void Launch(CPS2VM*, const ARCADE_MACHINE_DEF&) override;

private:
	std::vector<uint8> ReadDongleData(const ARCADE_MACHINE_DEF&);
	fs::path LocateImageFile(const ARCADE_MACHINE_DEF&, const std::string&);
};
