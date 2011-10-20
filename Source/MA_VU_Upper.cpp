#include "MA_VU.h"
#include "MIPS.h"
#include "VUShared.h"
#include "VIF.h"

using namespace std;

CMA_VU::CUpper::CUpper() :
CMIPSInstructionFactory(MIPS_REGSIZE_32),
m_nFT(0),
m_nFS(0),
m_nFD(0),
m_nBc(0),
m_nDest(0)
{

}

void CMA_VU::CUpper::CompileInstruction(uint32 nAddress, CMipsJitter* codeGen, CMIPS* pCtx)
{
	SetupQuickVariables(nAddress, codeGen, pCtx);

    m_nDest		= (uint8 )((m_nOpcode >> 21) & 0x000F);
    m_nFT		= (uint8 )((m_nOpcode >> 16) & 0x001F);
    m_nFS		= (uint8 )((m_nOpcode >> 11) & 0x001F);
    m_nFD		= (uint8 )((m_nOpcode >>  6) & 0x001F);

    m_nBc		= (uint8 )((m_nOpcode >>  0) & 0x0003);

    ((this)->*(m_pOpVector[m_nOpcode & 0x3F]))();

    if(m_nOpcode & 0x80000000)
    {
        LOI(pCtx->m_pMemoryMap->GetInstruction(nAddress - 4));
    }

    if(m_nOpcode & 0x40000000)
    {
        //Force exception checking if microprogram is done
        m_codeGen->PushCst(1);
        m_codeGen->PullRel(offsetof(CMIPS, m_State.nHasException));
    }
}

void CMA_VU::CUpper::LOI(uint32 nValue)
{
    m_codeGen->PushCst(nValue);
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2I));
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
    VUShared::ADDbc(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc);
}

//04
//05
//06
//07
void CMA_VU::CUpper::SUBbc()
{
    VUShared::SUBbc(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc);
}

//08
//09
//0A
//0B
void CMA_VU::CUpper::MADDbc()
{
    VUShared::MADDbc(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc);
}

//0C
//0D
//0E
//0F
void CMA_VU::CUpper::MSUBbc()
{
    VUShared::MSUBbc(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc);
}

//10
//11
//12
//13
void CMA_VU::CUpper::MAXbc()
{
    VUShared::MAXbc(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc);
}

//14
//15
//16
//17
void CMA_VU::CUpper::MINIbc()
{
    VUShared::MINIbc(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc);
}

//18
//19
//1A
//1B
void CMA_VU::CUpper::MULbc()
{
    VUShared::MULbc(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc);
}

//1C
void CMA_VU::CUpper::MULq()
{
    VUShared::MULq(m_codeGen, m_nDest, m_nFD, m_nFS, m_nAddress);
}

//1E
void CMA_VU::CUpper::MULi()
{
	VUShared::MULi(m_codeGen, m_nDest, m_nFD, m_nFS);
}

//1F
void CMA_VU::CUpper::MINIi()
{
    VUShared::MINIi(m_codeGen, m_nDest, m_nFD, m_nFS);
}

//20
void CMA_VU::CUpper::ADDq()
{
    VUShared::ADDq(m_codeGen, m_nDest, m_nFD, m_nFS, m_nAddress);
}

//21
void CMA_VU::CUpper::MADDq()
{
    VUShared::MADDq(m_codeGen, m_nDest, m_nFD, m_nFS, m_nAddress);
}

//22
void CMA_VU::CUpper::ADDi()
{
    VUShared::ADDi(m_codeGen, m_nDest, m_nFD, m_nFS);
}

//26
void CMA_VU::CUpper::SUBi()
{
	VUShared::SUBi(m_codeGen, m_nDest, m_nFD, m_nFS);
}

//27
void CMA_VU::CUpper::MSUBi()
{
	VUShared::MSUBi(m_codeGen, m_nDest, m_nFD, m_nFS);
}

//28
void CMA_VU::CUpper::ADD()
{
    VUShared::ADD(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT);
}

//29
void CMA_VU::CUpper::MADD()
{
	VUShared::MADD(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT);
}

//2A
void CMA_VU::CUpper::MUL()
{
    VUShared::MUL(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT);
}

//2B
void CMA_VU::CUpper::MAX()
{
    VUShared::MAX(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT);
}

//2C
void CMA_VU::CUpper::SUB()
{
    VUShared::SUB(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT);
}

//2E
void CMA_VU::CUpper::OPMSUB()
{
    VUShared::OPMSUB(m_codeGen, m_nFD, m_nFS, m_nFT);
}

//2F
void CMA_VU::CUpper::MINI()
{
    VUShared::MINI(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT);
}

//3C
void CMA_VU::CUpper::VECTOR0()
{
	((this)->*(m_pOpVector0[(m_nOpcode >> 6) & 0x1F]))();
}

//3D
void CMA_VU::CUpper::VECTOR1()
{
	((this)->*(m_pOpVector1[(m_nOpcode >> 6) & 0x1F]))();
}

//3E
void CMA_VU::CUpper::VECTOR2()
{
	((this)->*(m_pOpVector2[(m_nOpcode >> 6) & 0x1F]))();
}

//3F
void CMA_VU::CUpper::VECTOR3()
{
	((this)->*(m_pOpVector3[(m_nOpcode >> 6) & 0x1F]))();
}

//////////////////////////////////////////////////
//VectorX Common Instructions
//////////////////////////////////////////////////

//00
void CMA_VU::CUpper::ADDAbc()
{
    VUShared::ADDAbc(m_codeGen, m_nDest, m_nFS, m_nFT, m_nBc);
}

//02
void CMA_VU::CUpper::MADDAbc()
{
    VUShared::MADDAbc(m_codeGen, m_nDest, m_nFS, m_nFT, m_nBc);
}

//03
void CMA_VU::CUpper::MSUBAbc()
{
    VUShared::MSUBAbc(m_codeGen, m_nDest, m_nFS, m_nFT, m_nBc);
}

//06
void CMA_VU::CUpper::MULAbc()
{
    VUShared::MULAbc(m_codeGen, m_nDest, m_nFS, m_nFT, m_nBc);
}

//////////////////////////////////////////////////
//Vector0 Instructions
//////////////////////////////////////////////////

//04
void CMA_VU::CUpper::ITOF0()
{
    VUShared::ITOF0(m_codeGen, m_nDest, m_nFT, m_nFS);
}

//05
void CMA_VU::CUpper::FTOI0()
{
    VUShared::FTOI0(m_codeGen, m_nDest, m_nFT, m_nFS);
}

//0A
void CMA_VU::CUpper::ADDA()
{
    VUShared::ADDA(m_codeGen, m_nDest, m_nFS, m_nFT);
}

//////////////////////////////////////////////////
//Vector1 Instructions
//////////////////////////////////////////////////

//04
void CMA_VU::CUpper::ITOF4()
{
    VUShared::ITOF4(m_codeGen, m_nDest, m_nFT, m_nFS);
}

//05
void CMA_VU::CUpper::FTOI4()
{
    VUShared::FTOI4(m_codeGen, m_nDest, m_nFT, m_nFS);
}

//07
void CMA_VU::CUpper::ABS()
{
	VUShared::ABS(m_codeGen, m_nDest, m_nFT, m_nFS);
}

//0A
void CMA_VU::CUpper::MADDA()
{
	VUShared::MADDA(m_codeGen, m_nDest, m_nFS, m_nFT);
}

//////////////////////////////////////////////////
//Vector2 Instructions
//////////////////////////////////////////////////

//04
void CMA_VU::CUpper::ITOF12()
{
    VUShared::ITOF12(m_codeGen, m_nDest, m_nFT, m_nFS);
}

//05
void CMA_VU::CUpper::FTOI12()
{
    VUShared::FTOI12(m_codeGen, m_nDest, m_nFT, m_nFS);
}

//07
void CMA_VU::CUpper::MULAi()
{
	VUShared::MULAi(m_codeGen, m_nDest, m_nFS);
}

//0A
void CMA_VU::CUpper::MULA()
{
	VUShared::MULA(m_codeGen, m_nDest, m_nFS, m_nFT);
}

//0B
void CMA_VU::CUpper::OPMULA()
{
    VUShared::OPMULA(m_codeGen, m_nFS, m_nFT);
}

//////////////////////////////////////////////////
//Vector3 Instructions
//////////////////////////////////////////////////

//07
void CMA_VU::CUpper::CLIP()
{
    VUShared::CLIP(m_codeGen, m_nFS, m_nFT);
}

//08
void CMA_VU::CUpper::MADDAi()
{
	VUShared::MADDAi(m_codeGen, m_nDest, m_nFS);
}

//09
void CMA_VU::CUpper::MSUBAi()
{
	VUShared::MSUBAi(m_codeGen, m_nDest, m_nFS);
}

//0B
void CMA_VU::CUpper::NOP()
{

}

//////////////////////////////////////////////////
//Opcode Tables
//////////////////////////////////////////////////

CMA_VU::CUpper::InstructionFuncConstant CMA_VU::CUpper::m_pOpVector[0x40] =
{
	//0x00
	&CMA_VU::CUpper::ADDbc,			&CMA_VU::CUpper::ADDbc,			&CMA_VU::CUpper::ADDbc,			&CMA_VU::CUpper::ADDbc,			&CMA_VU::CUpper::SUBbc,		    &CMA_VU::CUpper::SUBbc,			&CMA_VU::CUpper::SUBbc,		    &CMA_VU::CUpper::SUBbc,
	//0x08
	&CMA_VU::CUpper::MADDbc,		&CMA_VU::CUpper::MADDbc,		&CMA_VU::CUpper::MADDbc,		&CMA_VU::CUpper::MADDbc,		&CMA_VU::CUpper::MSUBbc, 		&CMA_VU::CUpper::MSUBbc,		&CMA_VU::CUpper::MSUBbc,		&CMA_VU::CUpper::MSUBbc,
	//0x10
	&CMA_VU::CUpper::MAXbc,			&CMA_VU::CUpper::MAXbc,	    	&CMA_VU::CUpper::MAXbc,			&CMA_VU::CUpper::MAXbc,			&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::MINIbc,
	//0x18
	&CMA_VU::CUpper::MULbc,			&CMA_VU::CUpper::MULbc,			&CMA_VU::CUpper::MULbc,			&CMA_VU::CUpper::MULbc,			&CMA_VU::CUpper::MULq,			&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::MULi,			&CMA_VU::CUpper::MINIi,
	//0x20
	&CMA_VU::CUpper::ADDq,			&CMA_VU::CUpper::MADDq,		    &CMA_VU::CUpper::ADDi,			&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::SUBi,			&CMA_VU::CUpper::MSUBi,
	//0x28
	&CMA_VU::CUpper::ADD,			&CMA_VU::CUpper::MADD,			&CMA_VU::CUpper::MUL,			&CMA_VU::CUpper::MAX,			&CMA_VU::CUpper::SUB,			&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::OPMSUB,		&CMA_VU::CUpper::MINI,
	//0x30
	&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,
	//0x38
	&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::VECTOR0,		&CMA_VU::CUpper::VECTOR1,		&CMA_VU::CUpper::VECTOR2,		&CMA_VU::CUpper::VECTOR3,
};

CMA_VU::CUpper::InstructionFuncConstant CMA_VU::CUpper::m_pOpVector0[0x20] =
{
	//0x00
	&CMA_VU::CUpper::ADDAbc,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::MSUBAbc,		&CMA_VU::CUpper::ITOF0,			&CMA_VU::CUpper::FTOI0,			&CMA_VU::CUpper::MULAbc,		&CMA_VU::CUpper::Illegal,
	//0x08
	&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::ADDA,		    &CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,
	//0x10
	&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,
	//0x18
	&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,
};

CMA_VU::CUpper::InstructionFuncConstant CMA_VU::CUpper::m_pOpVector1[0x20] =
{
	//0x00
	&CMA_VU::CUpper::ADDAbc,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::MADDAbc,		&CMA_VU::CUpper::MSUBAbc,		&CMA_VU::CUpper::ITOF4,		    &CMA_VU::CUpper::FTOI4,			&CMA_VU::CUpper::MULAbc,		&CMA_VU::CUpper::ABS,
	//0x08
	&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::MADDA,			&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,
	//0x10
	&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,
	//0x18
	&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,
};

CMA_VU::CUpper::InstructionFuncConstant CMA_VU::CUpper::m_pOpVector2[0x20] =
{
	//0x00
	&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::MADDAbc,		&CMA_VU::CUpper::MSUBAbc,		&CMA_VU::CUpper::ITOF12, 		&CMA_VU::CUpper::FTOI12,		&CMA_VU::CUpper::MULAbc,		&CMA_VU::CUpper::MULAi,
	//0x08
	&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::MULA,			&CMA_VU::CUpper::OPMULA,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,
	//0x10
	&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,
	//0x18
	&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,
};

CMA_VU::CUpper::InstructionFuncConstant CMA_VU::CUpper::m_pOpVector3[0x20] =
{
	//0x00
	&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::MADDAbc,		&CMA_VU::CUpper::MSUBAbc,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::MULAbc,		&CMA_VU::CUpper::CLIP,
	//0x08
	&CMA_VU::CUpper::MADDAi,		&CMA_VU::CUpper::MSUBAi,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::NOP,			&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,
	//0x10
	&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,
	//0x18
	&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,
};
