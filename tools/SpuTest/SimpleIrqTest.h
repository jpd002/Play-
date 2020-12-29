#pragma once

#include "Test.h"
#include "Types.h"
#include "iop/Iop_Spu2.h"

class CSimpleIrqTest : public CTest
{
public:
	CSimpleIrqTest();
	~CSimpleIrqTest();
	
	void Execute() override;
	
private:
	void RunSpu(unsigned int);
	void SetCoreRegister(unsigned int, uint32, uint32);
	void SetCoreAddress(unsigned int, uint32, uint32);
	void SetVoiceRegister(unsigned int, unsigned int, uint32, uint32);
	void SetVoiceAddress(unsigned int, unsigned int, uint32, uint32);

	uint8* m_ram = nullptr;
	Iop::CSpuBase m_spuCore0;
	Iop::CSpuBase m_spuCore1;
	Iop::CSpu2 m_spu;
};
