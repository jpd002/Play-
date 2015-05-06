#pragma once

#include <string>
#include "../../gs/GSHandler.h"

class CGsStateUtils
{
public:
	static std::string		GetInputState(CGSHandler*);
	static std::string		GetContextState(CGSHandler*, unsigned int);
};
