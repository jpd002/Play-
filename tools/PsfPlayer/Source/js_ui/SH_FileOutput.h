#pragma once

#include "../SoundHandler.h"
#include <cstdio>

class CSH_FileOutput : public CSoundHandler
{
public:
	CSH_FileOutput();

	static CSoundHandler* HandlerFactory();

	void Reset() override;
	void Write(int16*, unsigned int, unsigned int) override;
	bool HasFreeBuffers() override;
	void RecycleBuffers() override;

private:
	FILE* m_outputStream = nullptr;
};
