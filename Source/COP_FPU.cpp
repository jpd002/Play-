#include <string.h>
#include <stdio.h>
#include "COP_FPU.h"
#include "MIPS.h"
#include "Jitter.h"
#include "offsetof_def.h"
#include "MemoryUtils.h"
#include "FpUtils.h"

// clang-format off
const uint32 CCOP_FPU::m_ccMask[8] =
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
// clang-format on

CCOP_FPU::CCOP_FPU(MIPS_REGSIZE regSize)
    : CMIPSCoprocessor(regSize)
{
	SetupReflectionTables();
}

void CCOP_FPU::CompileInstruction(uint32 address, CMipsJitter* codeGen, CMIPS* ctx, uint32 instrPosition)
{
	SetupQuickVariables(address, codeGen, ctx, instrPosition);

	m_ft = static_cast<uint8>((m_nOpcode >> 16) & 0x1F);
	m_fs = static_cast<uint8>((m_nOpcode >> 11) & 0x1F);
	m_fd = static_cast<uint8>((m_nOpcode >> 6) & 0x1F);

	switch((m_nOpcode >> 26) & 0x3F)
	{
	case 0x11:
		//COP1
		((this)->*(m_opGeneral[(m_nOpcode >> 21) & 0x1F]))();
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

void CCOP_FPU::SetCCBit(bool condition, uint32 mask)
{
	m_codeGen->PushCst(0);

	m_codeGen->BeginIf(condition ? Jitter::CONDITION_NE : Jitter::CONDITION_EQ);
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nFCSR));
		m_codeGen->PushCst(mask);
		m_codeGen->Or();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nFCSR));
	}
	m_codeGen->Else();
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nFCSR));
		m_codeGen->PushCst(~mask);
		m_codeGen->And();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nFCSR));
	}
	m_codeGen->EndIf();
}

void CCOP_FPU::PushCCBit(uint32 mask)
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nFCSR));
	m_codeGen->PushCst(mask);
	m_codeGen->And();
}

//////////////////////////////////////////////////
//General Opcodes
//////////////////////////////////////////////////

//00
void CCOP_FPU::MFC1()
{
	if(m_ft == 0)
	{
		return;
	}

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP1[m_fs]));
	if(m_regSize == MIPS_REGSIZE_64)
	{
		m_codeGen->PushTop();
		m_codeGen->SignExt();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_ft].nV[1]));
	}
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_ft].nV[0]));
}

//02
void CCOP_FPU::CFC1()
{
	if(m_ft == 0)
	{
		return;
	}

	if(m_fs < 16)
	{
		//Implementation and Revision Register
		//Value given away by the PS2 (Impl 46, Revision 3.0)
		m_codeGen->PushCst(0x2E30);
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_ft].nV[0]));
		if(m_regSize == MIPS_REGSIZE_64)
		{
			m_codeGen->PushCst(0);
			m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_ft].nV[1]));
		}
	}
	else
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nFCSR));
		if(m_regSize == MIPS_REGSIZE_64)
		{
			m_codeGen->PushTop();
			m_codeGen->SignExt();
			m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_ft].nV[1]));
		}
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_ft].nV[0]));
	}
}

//04
void CCOP_FPU::MTC1()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_ft].nV[0]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP1[m_fs]));
}

//06
void CCOP_FPU::CTC1()
{
	if(m_fs == 31)
	{
		//Only condition code and status flags are writable
		static const uint32 FCSR_WRITE_MASK = 0x0083C078;

		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_ft].nV[0]));
		m_codeGen->PushCst(FCSR_WRITE_MASK);
		m_codeGen->And();

		m_codeGen->PushRel(offsetof(CMIPS, m_State.nFCSR));
		m_codeGen->PushCst(~FCSR_WRITE_MASK);
		m_codeGen->And();

		m_codeGen->Or();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nFCSR));
	}
	else
	{
		Illegal();
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
	((this)->*(m_opSingle[(m_nOpcode & 0x3F)]))();
}

//14
void CCOP_FPU::W()
{
	((this)->*(m_opWord[(m_nOpcode & 0x3F)]))();
}

//////////////////////////////////////////////////
//Branch Opcodes
//////////////////////////////////////////////////

//00
void CCOP_FPU::BC1F()
{
	PushCCBit(m_ccMask[(m_nOpcode >> 18) & 0x07]);
	m_codeGen->PushCst(0);
	Branch(Jitter::CONDITION_EQ);
}

//01
void CCOP_FPU::BC1T()
{
	PushCCBit(m_ccMask[(m_nOpcode >> 18) & 0x07]);
	m_codeGen->PushCst(0);
	Branch(Jitter::CONDITION_NE);
}

//02
void CCOP_FPU::BC1FL()
{
	PushCCBit(m_ccMask[(m_nOpcode >> 18) & 0x07]);
	m_codeGen->PushCst(0);
	BranchLikely(Jitter::CONDITION_EQ);
}

//03
void CCOP_FPU::BC1TL()
{
	PushCCBit(m_ccMask[(m_nOpcode >> 18) & 0x07]);
	m_codeGen->PushCst(0);
	BranchLikely(Jitter::CONDITION_NE);
}

//////////////////////////////////////////////////
//Single Precision Floating Point Opcodes
//////////////////////////////////////////////////

//00
void CCOP_FPU::ADD_S()
{
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_fs]));
	m_codeGen->FP_Clamp();
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_ft]));
	m_codeGen->FP_Clamp();
	m_codeGen->FP_Add();
	m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP1[m_fd]));
}

//01
void CCOP_FPU::SUB_S()
{
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_fs]));
	m_codeGen->FP_Clamp();
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_ft]));
	m_codeGen->FP_Clamp();
	m_codeGen->FP_Sub();
	m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP1[m_fd]));
}

//02
void CCOP_FPU::MUL_S()
{
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_fs]));
	m_codeGen->FP_Clamp();
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_ft]));
	m_codeGen->FP_Clamp();
	m_codeGen->FP_Mul();
	m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP1[m_fd]));
}

//03
void CCOP_FPU::DIV_S()
{
	FpUtils::IsZero(m_codeGen, offsetof(CMIPS, m_State.nCOP1[m_ft]));
	m_codeGen->BeginIf(Jitter::CONDITION_EQ);
	{
		FpUtils::ComputeDivisionByZero(m_codeGen,
		                               offsetof(CMIPS, m_State.nCOP1[m_fs]),
		                               offsetof(CMIPS, m_State.nCOP1[m_ft]));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP1[m_fd]));
	}
	m_codeGen->Else();
	{
		m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_fs]));
		m_codeGen->FP_Clamp();
		m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_ft]));
		m_codeGen->FP_Clamp();
		m_codeGen->FP_Div();
		m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP1[m_fd]));
	}
	m_codeGen->EndIf();
}

//04
void CCOP_FPU::SQRT_S()
{
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_ft]));
	m_codeGen->FP_Clamp();
	m_codeGen->FP_Abs();
	m_codeGen->FP_Sqrt();
	m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP1[m_fd]));
}

//05
void CCOP_FPU::ABS_S()
{
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_fs]));
	m_codeGen->FP_Abs();
	m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP1[m_fd]));
}

//06
void CCOP_FPU::MOV_S()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP1[m_fs]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP1[m_fd]));
}

//07
void CCOP_FPU::NEG_S()
{
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_fs]));
	m_codeGen->FP_Neg();
	m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP1[m_fd]));
}

//0D
void CCOP_FPU::TRUNC_W_S()
{
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_fs]));
	m_codeGen->FP_PullWordTruncate(offsetof(CMIPS, m_State.nCOP1[m_fd]));
}

//16
void CCOP_FPU::RSQRT_S()
{
	FpUtils::IsZero(m_codeGen, offsetof(CMIPS, m_State.nCOP1[m_ft]));
	m_codeGen->BeginIf(Jitter::CONDITION_EQ);
	{
		FpUtils::ComputeDivisionByZero(m_codeGen,
		                               offsetof(CMIPS, m_State.nCOP1[m_fs]),
		                               offsetof(CMIPS, m_State.nCOP1[m_ft]));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP1[m_fd]));
	}
	m_codeGen->Else();
	{
		m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_fs]));
		m_codeGen->FP_Clamp();
		m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_ft]));
		m_codeGen->FP_Clamp();
		m_codeGen->FP_Rsqrt();
		m_codeGen->FP_Mul();
		m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP1[m_fd]));
	}
	m_codeGen->EndIf();
}

//18
void CCOP_FPU::ADDA_S()
{
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_fs]));
	m_codeGen->FP_Clamp();
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_ft]));
	m_codeGen->FP_Clamp();
	m_codeGen->FP_Add();
	m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP1A));
}

//19
void CCOP_FPU::SUBA_S()
{
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_fs]));
	m_codeGen->FP_Clamp();
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_ft]));
	m_codeGen->FP_Clamp();
	m_codeGen->FP_Sub();
	m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP1A));
}

//1A
void CCOP_FPU::MULA_S()
{
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_fs]));
	m_codeGen->FP_Clamp();
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_ft]));
	m_codeGen->FP_Clamp();
	m_codeGen->FP_Mul();
	m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP1A));
}

//1C
void CCOP_FPU::MADD_S()
{
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1A));
	m_codeGen->FP_Clamp();
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_fs]));
	m_codeGen->FP_Clamp();
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_ft]));
	m_codeGen->FP_Clamp();
	m_codeGen->FP_Mul();
	m_codeGen->FP_Add();
	m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP1[m_fd]));
}

//1D
void CCOP_FPU::MSUB_S()
{
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1A));
	m_codeGen->FP_Clamp();
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_fs]));
	m_codeGen->FP_Clamp();
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_ft]));
	m_codeGen->FP_Clamp();
	m_codeGen->FP_Mul();
	m_codeGen->FP_Sub();
	m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP1[m_fd]));
}

//1E
void CCOP_FPU::MADDA_S()
{
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1A));
	m_codeGen->FP_Clamp();
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_fs]));
	m_codeGen->FP_Clamp();
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_ft]));
	m_codeGen->FP_Clamp();
	m_codeGen->FP_Mul();
	m_codeGen->FP_Add();
	m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP1A));
}

//1F
void CCOP_FPU::MSUBA_S()
{
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1A));
	m_codeGen->FP_Clamp();
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_fs]));
	m_codeGen->FP_Clamp();
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_ft]));
	m_codeGen->FP_Clamp();
	m_codeGen->FP_Mul();
	m_codeGen->FP_Sub();
	m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP1A));
}

//24
void CCOP_FPU::CVT_W_S()
{
	//Load the rounding mode from FCSR?
	//PS2 only supports truncate rounding mode
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_fs]));
	m_codeGen->FP_PullWordTruncate(offsetof(CMIPS, m_State.nCOP1[m_fd]));
}

//28
void CCOP_FPU::MAX_S()
{
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_fs]));
	m_codeGen->FP_Clamp();
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_ft]));
	m_codeGen->FP_Clamp();
	m_codeGen->FP_Max();
	m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP1[m_fd]));
}

//29
void CCOP_FPU::MIN_S()
{
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_fs]));
	m_codeGen->FP_Clamp();
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_ft]));
	m_codeGen->FP_Clamp();
	m_codeGen->FP_Min();
	m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP1[m_fd]));
}

//30
void CCOP_FPU::C_F_S()
{
	m_codeGen->PushCst(0);
	SetCCBit(true, m_ccMask[((m_nOpcode >> 8) & 0x07)]);
}

//32
void CCOP_FPU::C_EQ_S()
{
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_fs]));
	m_codeGen->FP_Clamp();

	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_ft]));
	m_codeGen->FP_Clamp();

	m_codeGen->FP_Cmp(Jitter::CONDITION_EQ);

	SetCCBit(true, m_ccMask[((m_nOpcode >> 8) & 0x07)]);
}

//34
void CCOP_FPU::C_LT_S()
{
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_fs]));
	m_codeGen->FP_Clamp();

	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_ft]));
	m_codeGen->FP_Clamp();

	m_codeGen->FP_Cmp(Jitter::CONDITION_BL);

	SetCCBit(true, m_ccMask[((m_nOpcode >> 8) & 0x07)]);
}

//36
void CCOP_FPU::C_LE_S()
{
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_fs]));
	m_codeGen->FP_Clamp();

	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP1[m_ft]));
	m_codeGen->FP_Clamp();

	m_codeGen->FP_Cmp(Jitter::CONDITION_BE);

	SetCCBit(true, m_ccMask[((m_nOpcode >> 8) & 0x07)]);
}

//////////////////////////////////////////////////
//Word Sized Integer Opcodes
//////////////////////////////////////////////////

//20
void CCOP_FPU::CVT_S_W()
{
	m_codeGen->FP_PushWord(offsetof(CMIPS, m_State.nCOP1[m_fs]));
	m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP1[m_fd]));
}

//////////////////////////////////////////////////
//Miscellanous Opcodes
//////////////////////////////////////////////////

//31
void CCOP_FPU::LWC1()
{
	bool usePageLookup = (m_pCtx->m_pageLookup != nullptr);

	if(usePageLookup)
	{
		ComputeMemAccessPageRef();

		m_codeGen->PushCst(0);
		m_codeGen->BeginIf(Jitter::CONDITION_NE);
		{
			ComputeMemAccessRefIdx(4);

			m_codeGen->LoadFromRefIdx(1);
			m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP1[m_ft]));
		}
		m_codeGen->Else();
	}

	//Standard memory access
	{
		ComputeMemAccessAddrNoXlat();

		m_codeGen->PushCtx();
		m_codeGen->PushIdx(1);
		m_codeGen->Call(reinterpret_cast<void*>(&MemoryUtils_GetWordProxy), 2, Jitter::CJitter::RETURN_VALUE_32);

		m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP1[m_ft]));

		m_codeGen->PullTop();
	}

	if(usePageLookup)
	{
		m_codeGen->EndIf();
	}
}

//39
void CCOP_FPU::SWC1()
{
	bool usePageLookup = (m_pCtx->m_pageLookup != nullptr);

	if(usePageLookup)
	{
		ComputeMemAccessPageRef();

		m_codeGen->PushCst(0);
		m_codeGen->BeginIf(Jitter::CONDITION_NE);
		{
			ComputeMemAccessRefIdx(4);

			m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP1[m_ft]));
			m_codeGen->StoreAtRefIdx(1);
		}
		m_codeGen->Else();
	}

	//Standard memory access
	{
		ComputeMemAccessAddrNoXlat();

		m_codeGen->PushCtx();
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP1[m_ft]));
		m_codeGen->PushIdx(2);
		m_codeGen->Call(reinterpret_cast<void*>(&MemoryUtils_SetWordProxy), 3, Jitter::CJitter::RETURN_VALUE_NONE);

		m_codeGen->PullTop();
	}

	if(usePageLookup)
	{
		m_codeGen->EndIf();
	}
}

//////////////////////////////////////////////////
//Opcode Tables
//////////////////////////////////////////////////

// clang-format off
CCOP_FPU::InstructionFuncConstant CCOP_FPU::m_opGeneral[0x20] = 
{
	//0x00
	&CCOP_FPU::MFC1,		&CCOP_FPU::Illegal,		&CCOP_FPU::CFC1,		&CCOP_FPU::Illegal,		&CCOP_FPU::MTC1,		&CCOP_FPU::Illegal,		&CCOP_FPU::CTC1,		&CCOP_FPU::Illegal,
	//0x08
	&CCOP_FPU::BC1,			&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,
	//0x10
	&CCOP_FPU::S,			&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::W,			&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,
	//0x18
	&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,
};

CCOP_FPU::InstructionFuncConstant CCOP_FPU::m_opSingle[0x40] =
{
	//0x00
	&CCOP_FPU::ADD_S,		&CCOP_FPU::SUB_S,		&CCOP_FPU::MUL_S,		&CCOP_FPU::DIV_S,		&CCOP_FPU::SQRT_S,		&CCOP_FPU::ABS_S,		&CCOP_FPU::MOV_S,		&CCOP_FPU::NEG_S,
	//0x08
	&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::TRUNC_W_S,	&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,
	//0x10
	&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::RSQRT_S,		&CCOP_FPU::Illegal,
	//0x18
	&CCOP_FPU::ADDA_S,		&CCOP_FPU::SUBA_S,		&CCOP_FPU::MULA_S,		&CCOP_FPU::Illegal,		&CCOP_FPU::MADD_S,		&CCOP_FPU::MSUB_S,		&CCOP_FPU::MADDA_S,		&CCOP_FPU::MSUBA_S,
	//0x20
	&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::CVT_W_S,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,
	//0x28
	&CCOP_FPU::MAX_S,		&CCOP_FPU::MIN_S,	    &CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,
	//0x30
	&CCOP_FPU::C_F_S,		&CCOP_FPU::Illegal,		&CCOP_FPU::C_EQ_S,		&CCOP_FPU::Illegal,		&CCOP_FPU::C_LT_S,		&CCOP_FPU::Illegal,		&CCOP_FPU::C_LE_S,		&CCOP_FPU::Illegal,
	//0x38
	&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::C_LT_S,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,
};

CCOP_FPU::InstructionFuncConstant CCOP_FPU::m_opWord[0x40] =
{
	//0x00
	&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,
	//0x08
	&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,
	//0x10
	&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,
	//0x18
	&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,
	//0x20
	&CCOP_FPU::CVT_S_W,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,
	//0x28
	&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,
	//0x30
	&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,
	//0x38
	&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,		&CCOP_FPU::Illegal,
};
// clang-format on
