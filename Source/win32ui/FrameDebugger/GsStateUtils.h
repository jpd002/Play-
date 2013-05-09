#pragma once

#include <string>
#include "../../GSHandler.h"

class CGsStateUtils
{
public:
	static std::string		GetInputState(CGSHandler*);
	static std::string		GetContextState(CGSHandler*, unsigned int);
};
