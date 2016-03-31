#pragma once

#include "../../tools/PsfPlayer/Source/SoundHandler.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

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
	enum
	{
		BUFFER_COUNT = 5,
	};
	
	void    CreateOutputMix();
	void    CreateAudioPlayer();
	
	static void    QueueCallback(SLAndroidSimpleBufferQueueItf, void*);
	void           QueueCallbackImpl();
	
	SLObjectItf    m_engineObject = nullptr;
	SLEngineItf    m_engine = nullptr;
	
	SLObjectItf    m_outputMixObject = nullptr;

	SLObjectItf                      m_playerObject = nullptr;
	SLPlayItf                        m_playerPlay = nullptr;
	SLAndroidSimpleBufferQueueItf    m_playerQueue = nullptr;
	
	uint32    m_bufferCount = BUFFER_COUNT;
};
