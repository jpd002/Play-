#pragma once

#include "filesystem_def.h"

class CPS2VM;

namespace ArcadeUtils
{
	void RegisterArcadeMachines();
	void BootArcadeMachine(CPS2VM*, const fs::path&);
}
