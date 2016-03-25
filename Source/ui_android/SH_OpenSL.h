#pragma once

#include "../../tools/PsfPlayer/Source/SoundHandler.h"
#include <SLES/OpenSLES.h>

class CSH_OpenSL : public CSoundHandler
{
public:
	        CSH_OpenSL();
	virtual ~CSH_OpenSL();
	
	static CSoundHandler*    HandlerFactory();

	void    Reset() override;
	void    Write(int16*, unsigned int, unsigned int) override;
	bool    HasFreeBuffers() override;
	void    RecycleBuffers() override;
	
private:
	SLObjectItf    m_engineObject;
	SLEngineItf    m_engine;
};
