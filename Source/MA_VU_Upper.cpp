#include "MA_VU.h"
#include "MIPS.h"
#include "VUShared.h"
#include "VIF.h"

using namespace std;

uint32			CMA_VU::CUpper::m_nOpcode;
uint8			CMA_VU::CUpper::m_nFT;
uint8			CMA_VU::CUpper::m_nFS;
uint8			CMA_VU::CUpper::m_nFD;
uint8			CMA_VU::CUpper::m_nBc;
uint8			CMA_VU::CUpper::m_nDest;

void CMA_VU::CUpper::CompileInstruction(uint32 nAddress, CCacheBlock* pBlock, CMIPS* pCtx, bool nParent)
{
	m_nOpcode	= pCtx->m_pMemoryMap->GetWord(nAddress);
	m_nAddress	= nAddress;
	m_pB		= pBlock;
	m_pCtx		= pCtx;

	m_nDest		= (uint8 )((m_nOpcode >> 21) & 0x000F);
	m_nFT		= (uint8 )((m_nOpcode >> 16) & 0x001F);
	m_nFS		= (uint8 )((m_nOpcode >> 11) & 0x001F);
	m_nFD		= (uint8 )((m_nOpcode >>  6) & 0x001F);

	m_nBc		= (uint8 )((m_nOpcode >>  0) & 0x0003);

	m_pOpVector[m_nOpcode & 0x3F]();

	if(m_nOpcode & 0x80000000)
	{
		LOI(pCtx->m_pMemoryMap->GetWord(nAddress - 4));
	}

	if(m_nOpcode & 0x40000000)
	{
        assert(0);
//		m_pB->PushRef(m_pCtx);
//		m_pB->Call(CVIF::StopVU, 1, false);

		m_pB->PushImm(1);
		m_pB->PullAddr(&m_pCtx->m_nQuota);
	}
}

void CMA_VU::CUpper::LOI(uint32 nValue)
{
	m_pB->PushImm(nValue);
	m_pB->PullAddr(&m_pCtx->m_State.nCOP2I);
}

//////////////////////////////////////////////////
//Vector Instructions
//////////////////////////////////////////////////

//00
//01
//02
//03
void CMA_VU::CUpper::ADDbc()
{
    throw new runtime_error("Reimplement.");
//	VUShared::ADDbc(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc);
}

//04
//05
//06
//07
void CMA_VU::CUpper::SUBbc()
{
    throw new runtime_error("Reimplement.");
//	VUShared::SUBbc(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc);
}

//08
//09
//0A
//0B
void CMA_VU::CUpper::MADDbc()
{
    throw new runtime_error("Reimplement.");
//	VUShared::MADDbc(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc);
}

//10
//11
//12
//13
void CMA_VU::CUpper::MAXbc()
{
    throw new runtime_error("Reimplement.");
//	VUShared::MAXbc(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc);
}

//14
//15
//16
//17
void CMA_VU::CUpper::MINIbc()
{
    throw new runtime_error("Reimplement.");
//	VUShared::MINIbc(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc);
}

//18
//19
//1A
//1B
void CMA_VU::CUpper::MULbc()
{
    throw new runtime_error("Reimplement.");
//	VUShared::MULbc(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc);
}

//1C
void CMA_VU::CUpper::MULq()
{
    throw new runtime_error("Reimplement.");
//	VUShared::MULq(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS);
}

//1E
void CMA_VU::CUpper::MULi()
{
	VUShared::MULi(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS);
}

//1F
void CMA_VU::CUpper::MINIi()
{
	VUShared::MINIi(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS);
}

//20
void CMA_VU::CUpper::ADDq()
{
    throw new runtime_error("Reimplement.");
//	VUShared::ADDq(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS);
}

//22
void CMA_VU::CUpper::ADDi()
{
	VUShared::ADDi(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS);
}

//26
void CMA_VU::CUpper::SUBi()
{
	VUShared::SUBi(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS);
}

//27
void CMA_VU::CUpper::MSUBi()
{
	VUShared::MSUBi(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS);
}

//28
void CMA_VU::CUpper::ADD()
{
    throw new runtime_error("Reimplement.");
//	VUShared::ADD(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS, m_nFT);
}

//29
void CMA_VU::CUpper::MADD()
{
	VUShared::MADD(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS, m_nFT);
}

//2A
void CMA_VU::CUpper::MUL()
{
    throw new runtime_error("Reimplement.");
//	VUShared::MUL(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS, m_nFT);
}

//2B
void CMA_VU::CUpper::MAX()
{
    throw new runtime_error("Reimplement.");
//	VUShared::MAX(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS, m_nFT);
}

//2C
void CMA_VU::CUpper::SUB()
{
    throw new runtime_error("Reimplement.");
//	VUShared::SUB(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS, m_nFT);
}

//2E
void CMA_VU::CUpper::OPMSUB()
{
    throw new runtime_error("Reimplement.");
//	VUShared::OPMSUB(m_pB, m_pCtx, m_nFD, m_nFS, m_nFT);
}

//3C
void CMA_VU::CUpper::VECTOR0()
{
	m_pOpVector0[(m_nOpcode >> 6) & 0x1F]();
}

//3D
void CMA_VU::CUpper::VECTOR1()
{
	m_pOpVector1[(m_nOpcode >> 6) & 0x1F]();
}

//3E
void CMA_VU::CUpper::VECTOR2()
{
	m_pOpVector2[(m_nOpcode >> 6) & 0x1F]();
}

//3F
void CMA_VU::CUpper::VECTOR3()
{
	m_pOpVector3[(m_nOpcode >> 6) & 0x1F]();
}

//////////////////////////////////////////////////
//VectorX Common Instructions
//////////////////////////////////////////////////

//00
void CMA_VU::CUpper::ADDAbc()
{
	VUShared::ADDAbc(m_pB, m_pCtx, m_nDest, m_nFS, m_nFT, m_nBc);
}

//02
void CMA_VU::CUpper::MADDAbc()
{
    throw new runtime_error("Reimplement.");
//	VUShared::MADDAbc(m_pB, m_pCtx, m_nDest, m_nFS, m_nFT, m_nBc);
}

//06
void CMA_VU::CUpper::MULAbc()
{
    throw new runtime_error("Reimplement.");
//	VUShared::MULAbc(m_pB, m_pCtx, m_nDest, m_nFS, m_nFT, m_nBc);
}

//////////////////////////////////////////////////
//Vector0 Instructions
//////////////////////////////////////////////////

//04
void CMA_VU::CUpper::ITOF0()
{
    throw new runtime_error("Reimplement.");
//	VUShared::ITOF0(m_pB, m_pCtx, m_nDest, m_nFT, m_nFS);
}

//05
void CMA_VU::CUpper::FTOI0()
{
    throw new runtime_error("Reimplement.");
//	VUShared::FTOI0(m_pB, m_pCtx, m_nDest, m_nFT, m_nFS);
}

//////////////////////////////////////////////////
//Vector1 Instructions
//////////////////////////////////////////////////

//05
void CMA_VU::CUpper::FTOI4()
{
    throw new runtime_error("Reimplement.");
//	VUShared::FTOI4(m_pB, m_pCtx, m_nDest, m_nFT, m_nFS);
}

//07
void CMA_VU::CUpper::ABS()
{
	VUShared::ABS(m_pB, m_pCtx, m_nDest, m_nFT, m_nFS);
}

//0A
void CMA_VU::CUpper::MADDA()
{
	VUShared::MADDA(m_pB, m_pCtx, m_nDest, m_nFS, m_nFT);
}

//////////////////////////////////////////////////
//Vector2 Instructions
//////////////////////////////////////////////////

//07
void CMA_VU::CUpper::MULAi()
{
	VUShared::MULAi(m_pB, m_pCtx, m_nDest, m_nFS);
}

//0A
void CMA_VU::CUpper::MULA()
{
	VUShared::MULA(m_pB, m_pCtx, m_nDest, m_nFS, m_nFT);
}

//0B
void CMA_VU::CUpper::OPMULA()
{
    throw new runtime_error("Reimplement.");
//	VUShared::OPMULA(m_pB, m_pCtx, m_nFS, m_nFT);
}

//////////////////////////////////////////////////
//Vector3 Instructions
//////////////////////////////////////////////////

//07
void CMA_VU::CUpper::CLIP()
{
	VUShared::CLIP(m_pB, m_pCtx, m_nFS, m_nFT);
}

//08
void CMA_VU::CUpper::MADDAi()
{
	VUShared::MADDAi(m_pB, m_pCtx, m_nDest, m_nFS);
}

//09
void CMA_VU::CUpper::MSUBAi()
{
	VUShared::MSUBAi(m_pB, m_pCtx, m_nDest, m_nFS);
}

//0B
void CMA_VU::CUpper::NOP()
{

}

//////////////////////////////////////////////////
//Opcode Tables
//////////////////////////////////////////////////

void (*CMA_VU::CUpper::m_pOpVector[0x40])() =
{
	//0x00
	ADDbc,			ADDbc,			ADDbc,			ADDbc,			Illegal,		SUBbc,			Illegal,		SUBbc,
	//0x08
	MADDbc,			MADDbc,			MADDbc,			MADDbc,			Illegal,		Illegal,		Illegal,		Illegal,
	//0x10
	MAXbc,			Illegal,		Illegal,		MAXbc,			Illegal,		Illegal,		Illegal,		MINIbc,
	//0x18
	MULbc,			MULbc,			MULbc,			MULbc,			MULq,			Illegal,		MULi,			MINIi,
	//0x20
	ADDq,			Illegal,		ADDi,			Illegal,		Illegal,		Illegal,		SUBi,			MSUBi,
	//0x28
	ADD,			MADD,			MUL,			MAX,			SUB,			Illegal,		OPMSUB,			Illegal,
	//0x30
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x38
	Illegal,		Illegal,		Illegal,		Illegal,		VECTOR0,		VECTOR1,		VECTOR2,		VECTOR3,
};

void (*CMA_VU::CUpper::m_pOpVector0[0x20])() =
{
	//0x00
	ADDAbc,			Illegal,		Illegal,		Illegal,		ITOF0,			FTOI0,			MULAbc,			Illegal,
	//0x08
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x10
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x18
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
};

void (*CMA_VU::CUpper::m_pOpVector1[0x20])() =
{
	//0x00
	ADDAbc,			Illegal,		MADDAbc,		Illegal,		Illegal,		FTOI4,			Illegal,		ABS,
	//0x08
	Illegal,		Illegal,		MADDA,			Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x10
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x18
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
};

void (*CMA_VU::CUpper::m_pOpVector2[0x20])() =
{
	//0x00
	Illegal,		Illegal,		MADDAbc,		Illegal,		Illegal,		Illegal,		Illegal,		MULAi,
	//0x08
	Illegal,		Illegal,		MULA,			OPMULA,			Illegal,		Illegal,		Illegal,		Illegal,
	//0x10
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x18
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
};

void (*CMA_VU::CUpper::m_pOpVector3[0x20])() =
{
	//0x00
	Illegal,		Illegal,		MADDAbc,		Illegal,		Illegal,		Illegal,		MULAbc,			CLIP,
	//0x08
	MADDAi,			MSUBAi,			Illegal,		NOP,			Illegal,		Illegal,		Illegal,		Illegal,
	//0x10
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x18
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
};
