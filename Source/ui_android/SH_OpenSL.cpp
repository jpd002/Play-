#include <cassert>
#include "SH_OpenSL.h"

CSH_OpenSL::CSH_OpenSL()
{
	SLresult result = SL_RESULT_SUCCESS;
	
	result = slCreateEngine(&m_engineObject, 0, nullptr, 0, nullptr, nullptr);
	assert(result == SL_RESULT_SUCCESS);
	
	result = (*m_engineObject)->Realize(m_engineObject, SL_BOOLEAN_FALSE);
	assert(result == SL_RESULT_SUCCESS);
	
	result = (*m_engineObject)->GetInterface(m_engineObject, SL_IID_ENGINE, &m_engine);
	assert(result == SL_RESULT_SUCCESS);
	
	CreateOutputMix();
	CreateAudioPlayer();
}

CSH_OpenSL::~CSH_OpenSL()
{
	Reset();
	(*m_playerObject)->Destroy(m_playerObject);
	(*m_outputMixObject)->Destroy(m_outputMixObject);
	(*m_engineObject)->Destroy(m_engineObject);
}

CSoundHandler* CSH_OpenSL::HandlerFactory()
{
	return new CSH_OpenSL();
}

void CSH_OpenSL::CreateOutputMix()
{
	assert(m_outputMixObject == nullptr);
	
	SLresult result = SL_RESULT_SUCCESS;
	
	static const SLInterfaceID interfaceIds[] = { SL_IID_VOLUME };
	static const SLboolean required[] = { SL_BOOLEAN_FALSE };
	result = (*m_engine)->CreateOutputMix(m_engine, &m_outputMixObject, 1, interfaceIds, required);
	assert(result == SL_RESULT_SUCCESS);
	
	result = (*m_outputMixObject)->Realize(m_outputMixObject, SL_BOOLEAN_FALSE);
	assert(result == SL_RESULT_SUCCESS);
}

void CSH_OpenSL::CreateAudioPlayer()
{
	assert(m_playerObject == nullptr);
	
	SLresult result = SL_RESULT_SUCCESS;
	
	SLDataLocator_AndroidSimpleBufferQueue bufferQueueLocator = {};
	bufferQueueLocator.locatorType = SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE;
	bufferQueueLocator.numBuffers  = BUFFER_COUNT;
	
	SLDataFormat_PCM dataFormat = {};
	dataFormat.formatType    = SL_DATAFORMAT_PCM;
	dataFormat.numChannels   = 2;
	dataFormat.samplesPerSec = SL_SAMPLINGRATE_44_1;
	dataFormat.bitsPerSample = SL_PCMSAMPLEFORMAT_FIXED_16;
	dataFormat.containerSize = SL_PCMSAMPLEFORMAT_FIXED_16;
	dataFormat.channelMask   = SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT;
	dataFormat.endianness    = SL_BYTEORDER_LITTLEENDIAN;
	
	SLDataSource dataSource = {};
	dataSource.pLocator = &bufferQueueLocator;
	dataSource.pFormat  = &dataFormat;
	
	SLDataLocator_OutputMix outputMixLocator = {};
	outputMixLocator.locatorType = SL_DATALOCATOR_OUTPUTMIX;
	outputMixLocator.outputMix   = m_outputMixObject;
	
	SLDataSink dataSink = {};
	dataSink.pLocator = &outputMixLocator;
	
	static const SLInterfaceID interfaceIds[] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE };
	static const SLboolean required[] = { SL_BOOLEAN_TRUE };
	result = (*m_engine)->CreateAudioPlayer(m_engine, &m_playerObject, &dataSource, &dataSink, 
		1, interfaceIds, required);
	assert(result == SL_RESULT_SUCCESS);
	
	result = (*m_playerObject)->Realize(m_playerObject, SL_BOOLEAN_FALSE);
	assert(result == SL_RESULT_SUCCESS);
	
	result = (*m_playerObject)->GetInterface(m_playerObject, SL_IID_PLAY, &m_playerPlay);
	assert(result == SL_RESULT_SUCCESS);
	
	result = (*m_playerObject)->GetInterface(m_playerObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &m_playerQueue);
	assert(result == SL_RESULT_SUCCESS);
	
	result = (*m_playerQueue)->RegisterCallback(m_playerQueue, &CSH_OpenSL::QueueCallback, this);
	assert(result == SL_RESULT_SUCCESS);
	
	result = (*m_playerPlay)->SetPlayState(m_playerPlay, SL_PLAYSTATE_PLAYING);
	assert(result == SL_RESULT_SUCCESS);
}

void CSH_OpenSL::QueueCallback(SLAndroidSimpleBufferQueueItf, void* context)
{
	reinterpret_cast<CSH_OpenSL*>(context)->QueueCallbackImpl();
}

void CSH_OpenSL::QueueCallbackImpl()
{
	assert(m_bufferCount != BUFFER_COUNT);
	m_bufferCount++;
}

void CSH_OpenSL::Reset()
{
	assert(m_playerQueue != nullptr);
	
	SLresult result = SL_RESULT_SUCCESS;
	
	result = (*m_playerQueue)->Clear(m_playerQueue);
	assert(result == SL_RESULT_SUCCESS);
	
	m_bufferCount = BUFFER_COUNT;
}

void CSH_OpenSL::Write(int16* buffer, unsigned int sampleCount, unsigned int)
{
	if(m_bufferCount == 0) return;
	
	assert(m_playerQueue != nullptr);
	
	SLresult result = SL_RESULT_SUCCESS;
	
	result = (*m_playerQueue)->Enqueue(m_playerQueue, buffer, sampleCount * sizeof(int16));
	assert(result == SL_RESULT_SUCCESS);
	
	m_bufferCount--;
}

bool CSH_OpenSL::HasFreeBuffers()
{
	return false;
}

void CSH_OpenSL::RecycleBuffers()
{
	
}
