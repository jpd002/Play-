#pragma once

#include "Types.h"
#include "iop/Iop_Spu2.h"

#define TEST_VERIFY(a) \
	if(!(a))           \
	{                  \
		int* p = 0;    \
		(*p) = 0;      \
	}

class CTest
{
public:
	CTest();
	virtual ~CTest();
	virtual void Execute() = 0;

protected:
	enum
	{
		VOICE_COUNT = 24,
	};

	void RunSpu(unsigned int);

	uint32 GetCoreRegister(unsigned int, uint32);
	void SetCoreRegister(unsigned int, uint32, uint32);

	uint32 GetCoreAddress(unsigned int, uint32);
	void SetCoreAddress(unsigned int, uint32, uint32);

	uint32 GetVoiceRegister(unsigned int, unsigned int, uint32);
	void SetVoiceRegister(unsigned int, unsigned int, uint32, uint32);

	uint32 GetVoiceAddress(unsigned int, unsigned int, uint32);
	void SetVoiceAddress(unsigned int, unsigned int, uint32, uint32);

	uint8* m_ram = nullptr;
	Iop::CSpuBase m_spuCore0;
	Iop::CSpuBase m_spuCore1;
	Iop::CSpu2 m_spu;
};
