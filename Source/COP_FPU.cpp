#include <string.h>
#include <stdio.h>
#include "COP_FPU.h"
#include "MIPS.h"
#include "CodeGen.h"
#include "offsetof_def.h"
#include "MemoryUtils.h"

uint32				CCOP_FPU::m_nCCMask[8] =
{
	0x00800000,
	0x02000000,
	0x04000000,
	0x08000000,
	0x10000000,
	0x20000000,
	0x40000000,
	0x80000000
};

CCOP_FPU::CCOP_FPU(MIPS_REGSIZE nRegSize) :
CMIPSCoprocessor(nRegSize),
m_nFT(0),
m_nFS(0),
m_nFD(0)
{
	SetupReflectionTables();
}

void CCOP_FPU::CompileInstruction(uint32 nAddress, CCodeGen* codeGen, CMIPS* pCtx)
{
	SetupQuickVariables(nAddress, codeGen, pCtx);

	m_nFT			= (uint8)((m_nOpcode >> 16) & 0x1F);
	m_nFS			= (uint8)((m_nOpcode >> 11) & 0x1F);
	m_nFD			= (uint8)((m_nOpcode >> 6) & 0x1F);

	switch((m_nOpcode >> 26) & 0x3F)
	{
	case 0x11:
		//COP1
		((this)->*(m_pOpGeneral[(m_nOpcode >> 21) & 0x1F]))();
		break;
	case 0x31:
		LWC1();
		break;
	case 0x39:
		SWC1();
		break;
	default:
		Illegal();
		break;
	}
}

void CCOP_FPU::SetCCBit(bool nCondition, uint32 nMask)
{
	m_codeGen->BeginIfElse(nCondition);
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nFCSR));
		m_codeGen->PushCst(nMask);
		m_codeGen->Or();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nFCSR));
	}
	m_codeGen->BeginIfElseAlt();
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nFCSR));
		m_codeGen->PushCst(~nMask);
		m_codeGen->And();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nFCSR));
	}
	m_codeGen->EndIf();
}

void CCOP_FPU::TestCCBit(uint32 nMask)
{
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nFCSR));
    m_codeGen->PushCst(nMask);
    m_codeGen->And();
}

//////////////////////////////////////////////////
//General Opcodes
//////////////////////////////////////////////////

//00
void CCOP_FPU::MFC1()
{
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP10[m_nFS * 2]));
    m_codeGen->SeX();
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nFT].nV[1]));
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nFT].nV[0]));
}

//04
void CCOP_FPU::MTC1()
{
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nFT].nV[0]));
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP10[m_nFS * 2]));
}

//06
void CCOP_FPU::CTC1()
{
	if(m_nFS == 31)
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nFT].nV[0]));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nFCSR));
	}
    else
    {
        assert(0);
    }
}

//08
void CCOP_FPU::BC1()
{
	switch((m_nOpcode >> 16) & 0x03)
	{
	case 0x00:
		BC1F();
		break;
	case 0x01:
		BC1T();
		break;
	case 0x02:
		BC1FL();
		break;
	case 0x03:
		BC1TL();
		break;
	default:
		Illegal();
		break;
	}
}

//10
void CCOP_FPU::S()
{
	((this)->*(m_pOpSingle[(m_nOpcode & 0x3F)]))();
}

//14
void CCOP_FPU::W()
{
	((this)->*(m_pOpWord[(m_nOpcode & 0x3F)]))();
}

//////////////////////////////////////////////////
//Branch Opcodes
//////////////////////////////////////////////////

//00
void CCOP_FPU::BC1F()
{
	TestCCBit(m_nCCMask[(m_nOpcode >> 18) & 0x07]);
	Branch(false);
}

//01
void CCOP_FPU::BC1T()
{
	TestCCBit(m_nCCMask[(m_nOpcode >> 18) & 0x07]);
	Branch(true);
}

//02
void CCOP_FPU::BC1FL()
{
	TestCCBit(m_nCCMask[(m_nOpcode >> 18) & 0x07]);
	BranchLikely(false);
}

//03
void CCOP_FPU::BC1TL()
{
	TestCCBit(m_nCCMask[(m_nOpcode >> 18) & 0x07]);
	BranchLikely(true);
}

//////////////////////////////////////////////////
//Single Precision Floating Point Opcodes
//////////////////////////////////////////////////

//00
void CCOP_FPU::ADD_S()
{
    m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP10[m_nFS * 2]));
    m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP10[m_nFT * 2]));
    m_codeGen->FP_Add();
    m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP10[m_nFD * 2]));
}

//01
void CCOP_FPU::SUB_S()
{
    m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP10[m_nFS * 2]));
    m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP10[m_nFT * 2]));
    m_codeGen->FP_Sub();
    m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP10[m_nFD * 2]));
}

//02
void CCOP_FPU::MUL_S()
{
    m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP10[m_nFS * 2]));
    m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP10[m_nFT * 2]));
    m_codeGen->FP_Mul();
    m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP10[m_nFD * 2]));
}

//03
void CCOP_FPU::DIV_S()
{
	//Check if FT equals to 0
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP10[m_nFT * 2]));
    m_codeGen->PushCst(0);
    m_codeGen->Cmp(CCodeGen::CONDITION_EQ);

	m_codeGen->BeginIfElse(true);
	{
        m_codeGen->PushCst(0x7F7FFFFF);
        m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP10[m_nFD * 2]));
	}
	m_codeGen->BeginIfElseAlt();
	{
        m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP10[m_nFS * 2]));
        m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP10[m_nFT * 2]));
        m_codeGen->FP_Div();
        m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP10[m_nFD * 2]));
	}
	m_codeGen->EndIf();
}

//04
void CCOP_FPU::SQRT_S()
{
    m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP10[m_nFT * 2]));
    m_codeGen->FP_Sqrt();
    m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP10[m_nFD * 2]));
}

//05
void CCOP_FPU::ABS_S()
{
    m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP10[m_nFS * 2]));
    m_codeGen->FP_Abs();
    m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP10[m_nFD * 2]));
}

//06
void CCOP_FPU::MOV_S()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP10[m_nFS * 2]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP10[m_nFD * 2]));
}

//07
void CCOP_FPU::NEG_S()
{
    m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP10[m_nFS * 2]));
    m_codeGen->FP_Neg();
    m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP10[m_nFD * 2]));
}

//18
void CCOP_FPU::ADDA_S()
{
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP10[m_nFS * 2]));
    m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP10[m_nFT * 2]));
	m_codeGen->FP_Add();
	m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP1A));
}

//1A
void CCOP_FPU::MULA_S()
{
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP10[m_nFS * 2]));
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP10[m_nFT * 2]));
	m_codeGen->FP_Mul();
	m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP1A));
}

//1C
void CCOP_FPU::MADD_S()
{
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1A));
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP10[m_nFS * 2]));
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP10[m_nFT * 2]));
	m_codeGen->FP_Mul();
	m_codeGen->FP_Add();
	m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP10[m_nFD * 2]));
}

//1D
void CCOP_FPU::MSUB_S()
{
    m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1A));
    m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP10[m_nFS * 2]));
    m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP10[m_nFT * 2]));
    m_codeGen->FP_Mul();
    m_codeGen->FP_Sub();
    m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP10[m_nFD * 2]));
}

//24
void CCOP_FPU::CVT_W_S()
{
	//Load the rounding mode from FCSR?
    //PS2 only supports truncate rounding mode
    m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP10[m_nFS * 2]));
    m_codeGen->FP_PullWordTruncate(offsetof(CMIPS, m_State.nCOP10[m_nFD * 2]));
}

//28
void CCOP_FPU::MAX_S()
{
    m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP10[m_nFS * 2]));
    m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP10[m_nFT * 2]));
    m_codeGen->FP_Max();
    m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP10[m_nFD * 2]));
}

//29
void CCOP_FPU::MIN_S()
{
    m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP10[m_nFS * 2]));
    m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP10[m_nFT * 2]));
    m_codeGen->FP_Min();
    m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP10[m_nFD * 2]));
}

//32
void CCOP_FPU::C_EQ_S()
{
    m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP10[m_nFT * 2]));
    m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP10[m_nFS * 2]));

    m_codeGen->FP_Cmp(CCodeGen::CONDITION_EQ);

    SetCCBit(true, m_nCCMask[((m_nOpcode >> 8) & 0x07)]);
}

//34
void CCOP_FPU::C_LT_S()
{
    m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP10[m_nFT * 2]));
    m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP10[m_nFS * 2]));

    m_codeGen->FP_Cmp(CCodeGen::CONDITION_BL);

    SetCCBit(true, m_nCCMask[((m_nOpcode >> 8) & 0x07)]);
}

//36
void CCOP_FPU::C_LE_S()
{
    m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP10[m_nFT * 2]));
    m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP10[m_nFS * 2]));

    m_codeGen->FP_Cmp(CCodeGen::CONDITION_BE);

    SetCCBit(true, m_nCCMask[((m_nOpcode >> 8) & 0x07)]);
}

//////////////////////////////////////////////////
//Word Sized Integer Opcodes
//////////////////////////////////////////////////

//20
void CCOP_FPU::CVT_S_W()
{
	m_codeGen->FP_PushWord(offsetof(CMIPS, m_State.nCOP10[m_nFS * 2]));
	m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP10[m_nFD * 2]));
}

//////////////////////////////////////////////////
//Miscellanous Opcodes
//////////////////////////////////////////////////

//31
void CCOP_FPU::LWC1()
{
    ComputeMemAccessAddr();

	m_codeGen->PushRef(m_pCtx);
	m_codeGen->PushIdx(1);
	m_codeGen->Call(reinterpret_cast<void*>(&CMemoryUtils::GetWordProxy), 2, true);

    m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP10[m_nFT * 2]));

    m_codeGen->PullTop();
}

//39
void CCOP_FPU::SWC1()
{
	ComputeMemAccessAddr();

	m_codeGen->PushRef(m_pCtx);
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP10[m_nFT * 2]));
	m_codeGen->PushIdx(2);
	m_codeGen->Call(reinterpret_cast<void*>(&CMemoryUtils::SetWordProxy), 3, false);

	m_codeGen->PullTop();
}

//////////////////////////////////////////////////
//Opcode Tables
//////////////////////////////////////////////////

CCOP_FPU::InstructionFuncConstant CCOP_FPU::m_pOpGeneral[0x20] = 
{
	//0x00
	&MFC1,			&Illegal,		&Illegal,		&Illegal,		&MTC1,			&Illegal,		&CTC1,			&Illegal,
	//0x08
	&BC1,			&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,
	//0x10
	&S,				&Illegal,		&Illegal,		&Illegal,		&W,				&Illegal,		&Illegal,		&Illegal,
	//0x18
	&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,
};

CCOP_FPU::InstructionFuncConstant CCOP_FPU::m_pOpSingle[0x40] =
{
	//0x00
	&ADD_S,			&SUB_S,			&MUL_S,			&DIV_S,			&SQRT_S,		&ABS_S,			&MOV_S,			&NEG_S,
	//0x08
	&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,
	//0x10
	&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,
	//0x18
	&ADDA_S,		&Illegal,		&MULA_S,		&Illegal,		&MADD_S,		&MSUB_S,		&Illegal,		&Illegal,
	//0x20
	&Illegal,		&Illegal,		&Illegal,		&Illegal,		&CVT_W_S,		&Illegal,		&Illegal,		&Illegal,
	//0x28
	&MAX_S,		    &MIN_S,	    	&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,
	//0x30
	&Illegal,		&Illegal,		&C_EQ_S,		&Illegal,		&C_LT_S,		&Illegal,		&C_LE_S,		&Illegal,
	//0x38
	&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,
};

CCOP_FPU::InstructionFuncConstant CCOP_FPU::m_pOpWord[0x40] =
{
	//0x00
	&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,
	//0x08
	&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,
	//0x10
	&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,
	//0x18
	&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,
	//0x20
	&CVT_S_W,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,
	//0x28
	&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,
	//0x30
	&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,
	//0x38
	&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,
};
