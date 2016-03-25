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
}

CSH_OpenSL::~CSH_OpenSL()
{
	Reset();
	(*m_engineObject)->Destroy(m_engineObject);
}

CSoundHandler* CSH_OpenSL::HandlerFactory()
{
	return new CSH_OpenSL();
}

void CSH_OpenSL::Reset()
{
	
}

void CSH_OpenSL::Write(int16*, unsigned int, unsigned int)
{
	
}

bool CSH_OpenSL::HasFreeBuffers()
{
	return false;
}

void CSH_OpenSL::RecycleBuffers()
{
	
}
