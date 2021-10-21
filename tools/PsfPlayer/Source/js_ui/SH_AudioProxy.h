#pragma once

#include "../SoundHandler.h"

class CSH_AudioProxy : public CSoundHandler
{
public:
	CSH_AudioProxy(CSoundHandler*);

	static FactoryFunction GetFactoryFunction(CSoundHandler*);

	void Reset() override;
	void Write(int16*, unsigned int, unsigned int) override;
	bool HasFreeBuffers() override;
	void RecycleBuffers() override;

private:
	CSoundHandler* m_parent = nullptr;
};
