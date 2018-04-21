#pragma once

#include "Types.h"

class CSifModule
{
public:
	virtual ~CSifModule() = default;
	virtual bool Invoke(uint32, uint32*, uint32, uint32*, uint32, uint8*) = 0;
};
