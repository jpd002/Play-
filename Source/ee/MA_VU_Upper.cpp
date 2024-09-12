#include "MA_VU.h"
#include "Log.h"
#include "../MIPS.h"
#include "VUShared.h"
#include "Vif.h"
#include "offsetof_def.h"

#undef MAX

CMA_VU::CUpper::CUpper()
    : CMIPSInstructionFactory(MIPS_REGSIZE_32)
    , m_nFT(0)
    , m_nFS(0)
    , m_nFD(0)
    , m_nBc(0)
    , m_nDest(0)
{
}

void CMA_VU::CUpper::CompileInstruction(uint32 nAddress, CMipsJitter* codeGen, CMIPS* pCtx, uint32 instrPositon)
{
	SetupQuickVariables(nAddress, codeGen, pCtx, instrPositon);

	m_nDest = (uint8)((m_nOpcode >> 21) & 0x000F);
	m_nFT = (uint8)((m_nOpcode >> 16) & 0x001F);
	m_nFS = (uint8)((m_nOpcode >> 11) & 0x001F);
	m_nFD = (uint8)((m_nOpcode >> 6) & 0x001F);

	m_nBc = (uint8)((m_nOpcode >> 0) & 0x0003);

	((this)->*(m_pOpVector[m_nOpcode & 0x3F]))();

	auto setTDBitException = [&](uint32 exceptionValue) {
		//Only set TD bit exception if there's no other outstanding
		//exceptions (ie.: E bit). We could set a bit for every stop bit
		//we encounter, but we assume that no game relies on multiple bits
		//being set.
		//This assumes that QUOTADONE is only set at the end
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nHasException));
		m_codeGen->PushCst(0);
		m_codeGen->BeginIf(Jitter::CONDITION_EQ);
		{
			m_codeGen->PushCst(exceptionValue);
			m_codeGen->PullRel(offsetof(CMIPS, m_State.nHasException));
		}
		m_codeGen->EndIf();
	};

	if(m_nOpcode & VUShared::VU_UPPEROP_BIT_D)
	{
		setTDBitException(MIPS_EXCEPTION_VU_DBIT);
	}

	if(m_nOpcode & VUShared::VU_UPPEROP_BIT_T)
	{
		setTDBitException(MIPS_EXCEPTION_VU_TBIT);
	}

	if(m_nOpcode & VUShared::VU_UPPEROP_BIT_I)
	{
		LOI(pCtx->m_pMemoryMap->GetInstruction(nAddress - 4));
	}

	if(m_nOpcode & VUShared::VU_UPPEROP_BIT_E)
	{
		//Force exception checking if microprogram is done
		m_codeGen->PushCst(MIPS_EXCEPTION_VU_EBIT);
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nHasException));
	}
}

void CMA_VU::CUpper::SetRelativePipeTime(uint32 relativePipeTime, uint32 compileHints)
{
	m_relativePipeTime = relativePipeTime;
	m_compileHints = compileHints;
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
	VUShared::ADDbc(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc, m_relativePipeTime, m_compileHints);
}

//04
//05
//06
//07
void CMA_VU::CUpper::SUBbc()
{
	VUShared::SUBbc(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc, m_relativePipeTime, m_compileHints);
}

//08
//09
//0A
//0B
void CMA_VU::CUpper::MADDbc()
{
	VUShared::MADDbc(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc, m_relativePipeTime, m_compileHints);
}

//0C
//0D
//0E
//0F
void CMA_VU::CUpper::MSUBbc()
{
	VUShared::MSUBbc(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc, m_relativePipeTime, m_compileHints);
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
	VUShared::MULbc(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc, m_relativePipeTime, m_compileHints);
}

//1C
void CMA_VU::CUpper::MULq()
{
	VUShared::MULq(m_codeGen, m_nDest, m_nFD, m_nFS, m_relativePipeTime, m_compileHints);
}

//1D
void CMA_VU::CUpper::MAXi()
{
	VUShared::MAXi(m_codeGen, m_nDest, m_nFD, m_nFS);
}

//1E
void CMA_VU::CUpper::MULi()
{
	VUShared::MULi(m_codeGen, m_nDest, m_nFD, m_nFS, m_relativePipeTime, m_compileHints);
}

//1F
void CMA_VU::CUpper::MINIi()
{
	VUShared::MINIi(m_codeGen, m_nDest, m_nFD, m_nFS);
}

//20
void CMA_VU::CUpper::ADDq()
{
	VUShared::ADDq(m_codeGen, m_nDest, m_nFD, m_nFS, m_relativePipeTime, m_compileHints);
}

//21
void CMA_VU::CUpper::MADDq()
{
	VUShared::MADDq(m_codeGen, m_nDest, m_nFD, m_nFS, m_relativePipeTime, m_compileHints);
}

//22
void CMA_VU::CUpper::ADDi()
{
	VUShared::ADDi(m_codeGen, m_nDest, m_nFD, m_nFS, m_relativePipeTime, m_compileHints);
}

//23
void CMA_VU::CUpper::MADDi()
{
	VUShared::MADDi(m_codeGen, m_nDest, m_nFD, m_nFS, m_relativePipeTime, m_compileHints);
}

//24
void CMA_VU::CUpper::SUBq()
{
	VUShared::SUBq(m_codeGen, m_nDest, m_nFD, m_nFS, m_relativePipeTime, m_compileHints);
}

//25
void CMA_VU::CUpper::MSUBq()
{
	VUShared::MSUBq(m_codeGen, m_nDest, m_nFD, m_nFS, m_relativePipeTime, m_compileHints);
}

//26
void CMA_VU::CUpper::SUBi()
{
	VUShared::SUBi(m_codeGen, m_nDest, m_nFD, m_nFS, m_relativePipeTime, m_compileHints);
}

//27
void CMA_VU::CUpper::MSUBi()
{
	VUShared::MSUBi(m_codeGen, m_nDest, m_nFD, m_nFS, m_relativePipeTime, m_compileHints);
}

//28
void CMA_VU::CUpper::ADD()
{
	VUShared::ADD(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_relativePipeTime, m_compileHints);
}

//29
void CMA_VU::CUpper::MADD()
{
	VUShared::MADD(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_relativePipeTime, m_compileHints);
}

//2A
void CMA_VU::CUpper::MUL()
{
	VUShared::MUL(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_relativePipeTime, m_compileHints);
}

//2B
void CMA_VU::CUpper::MAX()
{
	VUShared::MAX(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT);
}

//2C
void CMA_VU::CUpper::SUB()
{
	VUShared::SUB(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_relativePipeTime, m_compileHints);
}

//2D
void CMA_VU::CUpper::MSUB()
{
	VUShared::MSUB(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_relativePipeTime, m_compileHints);
}

//2E
void CMA_VU::CUpper::OPMSUB()
{
	VUShared::OPMSUB(m_codeGen, m_nFD, m_nFS, m_nFT, m_relativePipeTime, m_compileHints);
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
	VUShared::ADDAbc(m_codeGen, m_nDest, m_nFS, m_nFT, m_nBc, m_relativePipeTime, m_compileHints);
}

//01
void CMA_VU::CUpper::SUBAbc()
{
	VUShared::SUBAbc(m_codeGen, m_nDest, m_nFS, m_nFT, m_nBc, m_relativePipeTime, m_compileHints);
}

//02
void CMA_VU::CUpper::MADDAbc()
{
	VUShared::MADDAbc(m_codeGen, m_nDest, m_nFS, m_nFT, m_nBc, m_relativePipeTime, m_compileHints);
}

//03
void CMA_VU::CUpper::MSUBAbc()
{
	VUShared::MSUBAbc(m_codeGen, m_nDest, m_nFS, m_nFT, m_nBc, m_relativePipeTime, m_compileHints);
}

//06
void CMA_VU::CUpper::MULAbc()
{
	VUShared::MULAbc(m_codeGen, m_nDest, m_nFS, m_nFT, m_nBc, m_relativePipeTime, m_compileHints);
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

//07
void CMA_VU::CUpper::MULAq()
{
	VUShared::MULAq(m_codeGen, m_nDest, m_nFS, m_relativePipeTime, m_compileHints);
}

//08
void CMA_VU::CUpper::ADDAq()
{
	VUShared::ADDAq(m_codeGen, m_nDest, m_nFS, m_relativePipeTime, m_compileHints);
}

//0A
void CMA_VU::CUpper::ADDA()
{
	VUShared::ADDA(m_codeGen, m_nDest, m_nFS, m_nFT, m_relativePipeTime, m_compileHints);
}

//0B
void CMA_VU::CUpper::SUBA()
{
	VUShared::SUBA(m_codeGen, m_nDest, m_nFS, m_nFT, m_relativePipeTime, m_compileHints);
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

//08
void CMA_VU::CUpper::MADDAq()
{
	VUShared::MADDAq(m_codeGen, m_nDest, m_nFS, m_relativePipeTime, m_compileHints);
}

//09
void CMA_VU::CUpper::MSUBAq()
{
	VUShared::MSUBAq(m_codeGen, m_nDest, m_nFS, m_relativePipeTime, m_compileHints);
}

//0A
void CMA_VU::CUpper::MADDA()
{
	VUShared::MADDA(m_codeGen, m_nDest, m_nFS, m_nFT, m_relativePipeTime, m_compileHints);
}

//0B
void CMA_VU::CUpper::MSUBA()
{
	VUShared::MSUBA(m_codeGen, m_nDest, m_nFS, m_nFT, m_relativePipeTime, m_compileHints);
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
	VUShared::MULAi(m_codeGen, m_nDest, m_nFS, m_relativePipeTime, m_compileHints);
}

//08
void CMA_VU::CUpper::ADDAi()
{
	VUShared::ADDAi(m_codeGen, m_nDest, m_nFS, m_relativePipeTime, m_compileHints);
}

//09
void CMA_VU::CUpper::SUBAi()
{
	VUShared::SUBAi(m_codeGen, m_nDest, m_nFS, m_relativePipeTime, m_compileHints);
}

//0A
void CMA_VU::CUpper::MULA()
{
	VUShared::MULA(m_codeGen, m_nDest, m_nFS, m_nFT, m_relativePipeTime, m_compileHints);
}

//0B
void CMA_VU::CUpper::OPMULA()
{
	VUShared::OPMULA(m_codeGen, m_nFS, m_nFT);
}

//////////////////////////////////////////////////
//Vector3 Instructions
//////////////////////////////////////////////////

//04
void CMA_VU::CUpper::ITOF15()
{
	VUShared::ITOF15(m_codeGen, m_nDest, m_nFT, m_nFS);
}

//05
void CMA_VU::CUpper::FTOI15()
{
	VUShared::FTOI15(m_codeGen, m_nDest, m_nFT, m_nFS);
}

//07
void CMA_VU::CUpper::CLIP()
{
	VUShared::CLIP(m_codeGen, m_nFS, m_nFT, m_relativePipeTime);
}

//08
void CMA_VU::CUpper::MADDAi()
{
	VUShared::MADDAi(m_codeGen, m_nDest, m_nFS, m_relativePipeTime, m_compileHints);
}

//09
void CMA_VU::CUpper::MSUBAi()
{
	VUShared::MSUBAi(m_codeGen, m_nDest, m_nFS, m_relativePipeTime, m_compileHints);
}

//0B
void CMA_VU::CUpper::NOP()
{
}

//////////////////////////////////////////////////
//Opcode Tables
//////////////////////////////////////////////////

// clang-format off
CMA_VU::CUpper::InstructionFuncConstant CMA_VU::CUpper::m_pOpVector[0x40] =
{
	//0x00
	&CMA_VU::CUpper::ADDbc,			&CMA_VU::CUpper::ADDbc,			&CMA_VU::CUpper::ADDbc,			&CMA_VU::CUpper::ADDbc,			&CMA_VU::CUpper::SUBbc,			&CMA_VU::CUpper::SUBbc,			&CMA_VU::CUpper::SUBbc,			&CMA_VU::CUpper::SUBbc,
	//0x08
	&CMA_VU::CUpper::MADDbc,		&CMA_VU::CUpper::MADDbc,		&CMA_VU::CUpper::MADDbc,		&CMA_VU::CUpper::MADDbc,		&CMA_VU::CUpper::MSUBbc, 		&CMA_VU::CUpper::MSUBbc,		&CMA_VU::CUpper::MSUBbc,		&CMA_VU::CUpper::MSUBbc,
	//0x10
	&CMA_VU::CUpper::MAXbc,			&CMA_VU::CUpper::MAXbc,			&CMA_VU::CUpper::MAXbc,			&CMA_VU::CUpper::MAXbc,			&CMA_VU::CUpper::MINIbc,		&CMA_VU::CUpper::MINIbc,		&CMA_VU::CUpper::MINIbc,		&CMA_VU::CUpper::MINIbc,
	//0x18
	&CMA_VU::CUpper::MULbc,			&CMA_VU::CUpper::MULbc,			&CMA_VU::CUpper::MULbc,			&CMA_VU::CUpper::MULbc,			&CMA_VU::CUpper::MULq,			&CMA_VU::CUpper::MAXi,			&CMA_VU::CUpper::MULi,			&CMA_VU::CUpper::MINIi,
	//0x20
	&CMA_VU::CUpper::ADDq,			&CMA_VU::CUpper::MADDq,			&CMA_VU::CUpper::ADDi,			&CMA_VU::CUpper::MADDi,			&CMA_VU::CUpper::SUBq,			&CMA_VU::CUpper::MSUBq,			&CMA_VU::CUpper::SUBi,			&CMA_VU::CUpper::MSUBi,
	//0x28
	&CMA_VU::CUpper::ADD,			&CMA_VU::CUpper::MADD,			&CMA_VU::CUpper::MUL,			&CMA_VU::CUpper::MAX,			&CMA_VU::CUpper::SUB,			&CMA_VU::CUpper::MSUB,			&CMA_VU::CUpper::OPMSUB,		&CMA_VU::CUpper::MINI,
	//0x30
	&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,
	//0x38
	&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::VECTOR0,		&CMA_VU::CUpper::VECTOR1,		&CMA_VU::CUpper::VECTOR2,		&CMA_VU::CUpper::VECTOR3,
};

CMA_VU::CUpper::InstructionFuncConstant CMA_VU::CUpper::m_pOpVector0[0x20] =
{
	//0x00
	&CMA_VU::CUpper::ADDAbc,		&CMA_VU::CUpper::SUBAbc,		&CMA_VU::CUpper::MADDAbc,		&CMA_VU::CUpper::MSUBAbc,		&CMA_VU::CUpper::ITOF0,			&CMA_VU::CUpper::FTOI0,			&CMA_VU::CUpper::MULAbc,		&CMA_VU::CUpper::MULAq,
	//0x08
	&CMA_VU::CUpper::ADDAq,			&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::ADDA,			&CMA_VU::CUpper::SUBA,			&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,
	//0x10
	&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,
	//0x18
	&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,
};

CMA_VU::CUpper::InstructionFuncConstant CMA_VU::CUpper::m_pOpVector1[0x20] =
{
	//0x00
	&CMA_VU::CUpper::ADDAbc,		&CMA_VU::CUpper::SUBAbc,		&CMA_VU::CUpper::MADDAbc,		&CMA_VU::CUpper::MSUBAbc,		&CMA_VU::CUpper::ITOF4,			&CMA_VU::CUpper::FTOI4,			&CMA_VU::CUpper::MULAbc,		&CMA_VU::CUpper::ABS,
	//0x08
	&CMA_VU::CUpper::MADDAq,		&CMA_VU::CUpper::MSUBAq,		&CMA_VU::CUpper::MADDA,			&CMA_VU::CUpper::MSUBA,			&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,
	//0x10
	&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,
	//0x18
	&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,
};

CMA_VU::CUpper::InstructionFuncConstant CMA_VU::CUpper::m_pOpVector2[0x20] =
{
	//0x00
	&CMA_VU::CUpper::ADDAbc,		&CMA_VU::CUpper::SUBAbc,		&CMA_VU::CUpper::MADDAbc,		&CMA_VU::CUpper::MSUBAbc,		&CMA_VU::CUpper::ITOF12, 		&CMA_VU::CUpper::FTOI12,		&CMA_VU::CUpper::MULAbc,		&CMA_VU::CUpper::MULAi,
	//0x08
	&CMA_VU::CUpper::ADDAi,			&CMA_VU::CUpper::SUBAi,			&CMA_VU::CUpper::MULA,			&CMA_VU::CUpper::OPMULA,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,
	//0x10
	&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,
	//0x18
	&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,
};

CMA_VU::CUpper::InstructionFuncConstant CMA_VU::CUpper::m_pOpVector3[0x20] =
{
	//0x00
	&CMA_VU::CUpper::ADDAbc,		&CMA_VU::CUpper::SUBAbc,		&CMA_VU::CUpper::MADDAbc,		&CMA_VU::CUpper::MSUBAbc,		&CMA_VU::CUpper::ITOF15,		&CMA_VU::CUpper::FTOI15,		&CMA_VU::CUpper::MULAbc,		&CMA_VU::CUpper::CLIP,
	//0x08
	&CMA_VU::CUpper::MADDAi,		&CMA_VU::CUpper::MSUBAi,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::NOP,			&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,
	//0x10
	&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,
	//0x18
	&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,		&CMA_VU::CUpper::Illegal,
};
// clang-format on
