#pragma once

struct ARCADE_MACHINE_DEF;
class CPS2VM;

class CArcadeDriver
{
public:
	virtual ~CArcadeDriver() = default;
	virtual void PrepareEnvironment(CPS2VM*, const ARCADE_MACHINE_DEF&) = 0;
	virtual void Launch(CPS2VM*, const ARCADE_MACHINE_DEF&) = 0;
};
