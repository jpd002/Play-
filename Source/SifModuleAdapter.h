#pragma once

#include <functional>
#include "SifModule.h"

class CSifModuleAdapter : public CSifModule
{
public:
	typedef std::function<void (uint32, uint32*, uint32, uint32*, uint32, uint8*)> SifCommandHandler;

	CSifModuleAdapter()
	{

	}

	CSifModuleAdapter(const SifCommandHandler& handler) :
	m_handler(handler)
	{

	}
	
	virtual ~CSifModuleAdapter()
	{
		
	}

	virtual bool Invoke(uint32 method, uint32* args, uint32 argsSize, uint32* ret, uint32 retSize, uint8* ram) override
	{ 
		m_handler(method, args, argsSize, ret, retSize, ram);
		return true;
	}

private:
	SifCommandHandler m_handler;
};
