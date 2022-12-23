#include "SH_FileOutput.h"
#include <cassert>

CSH_FileOutput::CSH_FileOutput()
{
	m_outputStream = fopen("/work/output.raw", "wb");
	assert(m_outputStream != NULL);
}

CSoundHandler* CSH_FileOutput::HandlerFactory()
{
	return new CSH_FileOutput();
}

void CSH_FileOutput::Reset()
{
}

void CSH_FileOutput::Write(int16* samples, unsigned int sampleCount, unsigned int sampleRate)
{
	fwrite(samples, sizeof(int16), sampleCount, m_outputStream);
	fflush(m_outputStream);
}

bool CSH_FileOutput::HasFreeBuffers()
{
	return true;
}

void CSH_FileOutput::RecycleBuffers()
{
}
