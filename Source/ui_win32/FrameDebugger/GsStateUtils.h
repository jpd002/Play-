#pragma once

#include "../../gs/GSHandler.h"
#include <string>

class CGsStateUtils
{
public:
	static std::string GetInputState(CGSHandler*);
	static std::string GetContextState(CGSHandler*, unsigned int);
};
