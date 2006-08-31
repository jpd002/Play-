#include <stddef.h>
#include "MA_MIPSIV.h"
#include "MIPS.h"
#include "CodeGen.h"

CMA_MIPSIV			g_MAMIPSIV(MIPS_REGSIZE_64);

uint8				CMA_MIPSIV::m_nRS;
uint8				CMA_MIPSIV::m_nRT;
uint8				CMA_MIPSIV::m_nRD;
uint8				CMA_MIPSIV::m_nSA;
uint16				CMA_MIPSIV::m_nImmediate;

uint32				CMA_MIPSIV::m_nLWLMask[4] =
{
	0x00FFFFFF,
	0x0000FFFF,
	0x000000FF,
	0x00000000,
};

uint32				CMA_MIPSIV::m_nLWLShift[4] =
{
	24,
	16,
	 8,
	 0,
};

uint32				CMA_MIPSIV::m_nLWRMask[4] =
{
	0x00000000,
	0xFF000000,
	0xFFFF0000,
	0xFFFFFF00,
};

uint32				CMA_MIPSIV::m_nLWRShift[4] =
{
	0,
	8,
	16,
	24,
};

uint32				CMA_MIPSIV::m_nSWLMask[4] =
{
	0xFFFFFF00,
	0xFFFF0000,
	0xFF000000,
	0x00000000,
};

uint32				CMA_MIPSIV::m_nSWLShift[4] =
{
	24,
	16,
	 8,
	 0,
};

uint32				CMA_MIPSIV::m_nSWRMask[4] =
{
	0x00000000,
	0x000000FF,
	0x0000FFFF,
	0x00FFFFFF,
};

uint32				CMA_MIPSIV::m_nSWRShift[4] =
{
	 0,
	 8,
	16,
	24,
};

uint32				CMA_MIPSIV::m_nLDLMaskLo[8] =
{
	0xFFFFFFFF,
	0xFFFFFFFF,
	0xFFFFFFFF,
	0xFFFFFFFF,
	0x00FFFFFF,
	0x0000FFFF,
	0x000000FF,
	0x00000000,
};

uint32				CMA_MIPSIV::m_nLDLMaskHi[8] =
{
	0x00FFFFFF,
	0x0000FFFF,
	0x000000FF,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
};

uint32				CMA_MIPSIV::m_nLDLShift[8] =
{
	56,
	48,
	40,
	32,
	24,
	16,
	 8,
	 0
};

uint32				CMA_MIPSIV::m_nLDRMaskLo[8] =
{
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0xFF000000,
	0xFFFF0000,
	0xFFFFFF00,
};

uint32				CMA_MIPSIV::m_nLDRMaskHi[8] = 
{
	0x00000000,
	0xFF000000,
	0xFFFF0000,
	0xFFFFFF00,
	0xFFFFFFFF,
	0xFFFFFFFF,
	0xFFFFFFFF,
	0xFFFFFFFF,
};

uint32				CMA_MIPSIV::m_nLDRShift[8] =
{
	 0,
	 8,
	16,
	24,
	32,
	40,
	48,
	56,
};

uint32				CMA_MIPSIV::m_nSDLMaskLo[8] =
{
	0xFFFFFF00,
	0xFFFF0000,
	0xFF000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
};

uint32				CMA_MIPSIV::m_nSDLMaskHi[8] =
{
	0xFFFFFFFF,
	0xFFFFFFFF,
	0xFFFFFFFF,
	0xFFFFFFFF,
	0xFFFFFF00,
	0xFFFF0000,
	0xFF000000,
	0x00000000,
};

uint32				CMA_MIPSIV::m_nSDLShift[8] =
{
	56,
	48,
	40,
	32,
	24,
	16,
	 8,
	 0,
};

uint32				CMA_MIPSIV::m_nSDRMaskLo[8] =
{
	0x00000000,
	0x000000FF,
	0x0000FFFF,
	0x00FFFFFF,
	0xFFFFFFFF,
	0xFFFFFFFF,
	0xFFFFFFFF,
	0xFFFFFFFF,
};

uint32				CMA_MIPSIV::m_nSDRMaskHi[8] =
{
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x00000000,
	0x000000FF,
	0x0000FFFF,
	0x00FFFFFF,
};

uint32				CMA_MIPSIV::m_nSDRShift[8] =
{
	 0,
	 8,
	16,
	24,
	32,
	40,
	48,
	56,
};

CMA_MIPSIV::CMA_MIPSIV(MIPS_REGSIZE nRegSize) :
CMIPSArchitecture(nRegSize)
{
	SetupReflectionTables();
}

void CMA_MIPSIV::CompileInstruction(uint32 nAddress, CCacheBlock* pBlock, CMIPS* pCtx, bool nParent)
{
	if(nParent)
	{
		SetupQuickVariables(nAddress, pBlock, pCtx);
	}

	m_nRS			= (uint8)((m_nOpcode >> 21) & 0x1F);
	m_nRT			= (uint8)((m_nOpcode >> 16) & 0x1F);
	m_nRD			= (uint8)((m_nOpcode >> 11) & 0x1F);
	m_nSA			= (uint8)((m_nOpcode >> 6) & 0x1F);
	m_nImmediate	= (uint16)(m_nOpcode & 0xFFFF);

	if(m_nOpcode)
	{
		m_pOpGeneral[(m_nOpcode >> 26)]();
	}
}

void CMA_MIPSIV::SPECIAL()
{
	m_pOpSpecial[m_nImmediate & 0x3F]();
}

void CMA_MIPSIV::SPECIAL2()
{
	m_pOpSpecial2[m_nImmediate & 0x3F]();
}

void CMA_MIPSIV::REGIMM()
{
	m_pOpRegImm[m_nRT]();
}

//////////////////////////////////////////////////
//General Opcodes
//////////////////////////////////////////////////

//02
void CMA_MIPSIV::J()
{
	m_pB->PushAddr(&m_pCtx->m_State.nPC);
	m_pB->AndImm(0xF0000000);
	m_pB->AddImm((m_nOpcode & 0x03FFFFFF) << 2);
	m_pB->PullAddr(&m_pCtx->m_State.nDelayedJumpAddr);

	m_pB->SetDelayedJumpCheck();
}

//03
void CMA_MIPSIV::JAL()
{
	//64-bit addresses?

	//Save the address in RA
	m_pB->PushAddr(&m_pCtx->m_State.nPC);
	m_pB->AddImm(4);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[31].nV[0]);

	m_pB->PushAddr(&m_pCtx->m_State.nPC);
	m_pB->AndImm(0xF0000000);
	m_pB->AddImm((m_nOpcode & 0x03FFFFFF) << 2);
	m_pB->PullAddr(&m_pCtx->m_State.nDelayedJumpAddr);

	m_pB->SetDelayedJumpCheck();
}

//04
void CMA_MIPSIV::BEQ()
{
	CCodeGen::Begin(m_pB);
	{
		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));

		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

		CCodeGen::Cmp64(CCodeGen::CONDITION_EQ);

		BranchEx(true);
	}
	CCodeGen::End();
}

//05
void CMA_MIPSIV::BNE()
{
	CCodeGen::Begin(m_pB);
	{
		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));

		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

		CCodeGen::Cmp64(CCodeGen::CONDITION_EQ);

		BranchEx(false);
	}
	CCodeGen::End();
}

//06
void CMA_MIPSIV::BLEZ()
{
/*
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->PushImm(0);
	m_pB->Cmp(JCC_CONDITION_LE);

	Branch(true);
*/
	//Check sign bit
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[1]);
	m_pB->AndImm(0x80000000);
	m_pB->PushImm(0x00000000);
	m_pB->Cmp(JCC_CONDITION_EQ);

	//Check if parts aren't equal to zero
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[1]);
	m_pB->PushImm(0x00000000);
	m_pB->Cmp(JCC_CONDITION_NE);

	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->PushImm(0x00000000);
	m_pB->Cmp(JCC_CONDITION_NE);

	m_pB->Or();
	m_pB->And();

	Branch(false);
}

//07
void CMA_MIPSIV::BGTZ()
{
	//Check sign bit
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[1]);
	m_pB->AndImm(0x80000000);
	m_pB->PushImm(0x00000000);
	m_pB->Cmp(JCC_CONDITION_EQ);

	//Check if parts aren't equal to zero
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[1]);
	m_pB->PushImm(0x00000000);
	m_pB->Cmp(JCC_CONDITION_NE);

	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->PushImm(0x00000000);
	m_pB->Cmp(JCC_CONDITION_NE);

	m_pB->Or();
	m_pB->And();

	Branch(true);
}

//08
void CMA_MIPSIV::ADDI()
{
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->AddImm((int16)m_nImmediate);
	SignExtendTop32(m_nRT);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
}

//09
void CMA_MIPSIV::ADDIU()
{
	CCodeGen::Begin(m_pB);
	{
		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		CCodeGen::PushCst((int16)m_nImmediate);
		CCodeGen::Add();
		CCodeGen::SeX();
		CCodeGen::PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
		CCodeGen::PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	}
	CCodeGen::End();
}

//0A
void CMA_MIPSIV::SLTI()
{
	//TODO: Complete 64-bit comparaison
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->PushImm((int16)m_nImmediate);
	m_pB->Cmp(JCC_CONDITION_LT);
	SignExtendTop32(m_nRT);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
}

//0B
void CMA_MIPSIV::SLTIU()
{
	//TODO: Complete 64-bit comparaison
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->PushImm((int16)m_nImmediate);
	m_pB->Cmp(JCC_CONDITION_BL);
	SignExtendTop32(m_nRT);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
}

//0C
void CMA_MIPSIV::ANDI()
{
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->AndImm(m_nImmediate);
	SignExtendTop32(m_nRT);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
}

//0D
void CMA_MIPSIV::ORI()
{
	CCodeGen::Begin(m_pB);
	{
		//Lower 32-bits
		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		CCodeGen::PushCst(m_nImmediate);
		CCodeGen::Or();
		CCodeGen::PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));

		//Higher 32-bits (only if registers are different)
		if(m_nRS != m_nRT)
		{
			CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));
			CCodeGen::PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
		}
	}
	CCodeGen::End();
}

//0E
void CMA_MIPSIV::XORI()
{
	//Lower 32-bits
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->PushImm(m_nImmediate);
	m_pB->Xor();
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);

	//Higher 32-bits
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[1]);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[1]);
}

//0F
void CMA_MIPSIV::LUI()
{
	CCodeGen::Begin(m_pB);
	{
		CCodeGen::PushCst(m_nImmediate << 16);
		CCodeGen::PushCst((m_nImmediate & 0x8000) ? 0xFFFFFFFF : 0x00000000);
		CCodeGen::PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
		CCodeGen::PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	}
	CCodeGen::End();
}

//10
void CMA_MIPSIV::COP0()
{
	if(m_pCtx->m_pCOP[0] != NULL)
	{
		m_pCtx->m_pCOP[0]->CompileInstruction(m_nAddress, m_pB, m_pCtx, false);
	}
	else
	{
		Illegal();
	}
}

//11
void CMA_MIPSIV::COP1()
{
	if(m_pCtx->m_pCOP[1] != NULL)
	{
		m_pCtx->m_pCOP[1]->CompileInstruction(m_nAddress, m_pB, m_pCtx, false);
	}
	else
	{
		Illegal();
	}
}

//12
void CMA_MIPSIV::COP2()
{
	if(m_pCtx->m_pCOP[2] != NULL)
	{
		m_pCtx->m_pCOP[2]->CompileInstruction(m_nAddress, m_pB, m_pCtx, false);
	}
	else
	{
		Illegal();
	}
}

//14
void CMA_MIPSIV::BEQL()
{
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->Cmp(JCC_CONDITION_EQ);

	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[1]);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[1]);
	m_pB->Cmp(JCC_CONDITION_EQ);

	m_pB->And();

	BranchLikely(true);
}

//15
void CMA_MIPSIV::BNEL()
{
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->Cmp(JCC_CONDITION_EQ);

	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[1]);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[1]);
	m_pB->Cmp(JCC_CONDITION_EQ);

	m_pB->And();

	BranchLikely(false);
}

//16
void CMA_MIPSIV::BLEZL()
{
	//Check sign bit
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[1]);
	m_pB->AndImm(0x80000000);
	m_pB->PushImm(0x00000000);
	m_pB->Cmp(JCC_CONDITION_EQ);

	//Check if parts aren't equal to zero
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[1]);
	m_pB->PushImm(0x00000000);
	m_pB->Cmp(JCC_CONDITION_NE);

	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->PushImm(0x00000000);
	m_pB->Cmp(JCC_CONDITION_NE);

	m_pB->Or();
	m_pB->And();

	BranchLikely(false);
}

//17
void CMA_MIPSIV::BGTZL()
{
	//Check sign bit
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[1]);
	m_pB->AndImm(0x80000000);
	m_pB->PushImm(0x00000000);
	m_pB->Cmp(JCC_CONDITION_EQ);

	//Check if parts aren't equal to zero
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[1]);
	m_pB->PushImm(0x00000000);
	m_pB->Cmp(JCC_CONDITION_NE);

	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->PushImm(0x00000000);
	m_pB->Cmp(JCC_CONDITION_NE);

	m_pB->Or();
	m_pB->And();

	BranchLikely(true);
}

//19
void CMA_MIPSIV::DADDIU()
{
	//First 32-bits half
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->PushImm((int16)m_nImmediate);
	m_pB->AddC();
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);

	//2nd 32-bits half
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[1]);
	m_pB->PushImm((m_nImmediate & 0x8000) == 0 ? 0x00000000 : 0xFFFFFFFF);
	m_pB->Add();
	m_pB->Add();
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[1]);
}

//1A
void CMA_MIPSIV::LDL()
{
	ComputeMemAccessAddr();

	m_pB->PushValueAt(0);
	m_pB->AndImm(0x00000007);
	m_pB->PushValueAt(1);
	m_pB->AndImm(~0x00000007);
	//2 - BYTE EA
	//1 - OFFSET
	//0 - EA

	//Get the lower word
	m_pB->PushValueAt(0);
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::GetWordProxy, 2, true);
	//3 - BYTE EA
	//2 - OFFSET
	//1 - EA
	//0 - LOWORD

	//Get the high word
	m_pB->PushValueAt(1);
	m_pB->AddImm(4);
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::GetWordProxy, 2, true);
	//4 - BYTE EA
	//3 - OFFSET
	//2 - EA
	//1 - LOWORD
	//0 - HIWORD

	//Shift the 64-bits word left
	m_pB->PushValueAt(3);
	m_pB->Lookup(m_nLDLShift);
	m_pB->Sll64();
	//4 - BYTE EA
	//3 - OFFSET
	//2 - EA
	//1 - LOWORD SHIFTED
	//0 - HIWORD SHIFTED

	////////////////////////
	//Higher 32-bits
	////////////////////////

	//Get the mask for this register
	m_pB->PushValueAt(3);
	m_pB->Lookup(m_nLDLMaskHi);
	//5 - BYTE EA
	//4 - OFFSET
	//3 - EA
	//2 - LOWORD SHIFTED
	//1 - HIWORD SHIFTED
	//0 - HIMASK

	//Obtain the value of the register, mask, or with the value, sign extend, save
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[1]);
	m_pB->And();
	m_pB->Or();
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[1]);
	//3 - BYTE EA
	//2 - OFFSET
	//1 - EA
	//0 - LOWORD SHIFTED

	////////////////////////
	//Lower 32-bits
	////////////////////////

	//Get the mask for this register
	m_pB->PushValueAt(2);
	m_pB->Lookup(m_nLDLMaskLo);
	//4 - BYTE EA
	//3 - OFFSET
	//2 - EA
	//1 - LOWORD SHIFTED
	//0 - LOMASK

	//Obtain the value of the register, mask, or with the value and save
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->And();
	m_pB->Or();
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	//2 - BYTE EA
	//1 - OFFSET
	//0 - EA

	//Done, free the stack
	m_pB->Free(3);
}

//1B
void CMA_MIPSIV::LDR()
{
	ComputeMemAccessAddr();

	m_pB->PushValueAt(0);
	m_pB->AndImm(0x00000007);
	m_pB->PushValueAt(1);
	m_pB->AndImm(~0x00000007);
	//2 - BYTE EA
	//1 - OFFSET
	//0 - EA

	//Get the lower word
	m_pB->PushValueAt(0);
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::GetWordProxy, 2, true);
	//3 - BYTE EA
	//2 - OFFSET
	//1 - EA
	//0 - LOWORD

	//Get the high word
	m_pB->PushValueAt(1);
	m_pB->AddImm(4);
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::GetWordProxy, 2, true);
	//4 - BYTE EA
	//3 - OFFSET
	//2 - EA
	//1 - LOWORD
	//0 - HIWORD

	//Shift the 64-bits word right
	m_pB->PushValueAt(3);
	m_pB->Lookup(m_nLDRShift);
	m_pB->Srl64();
	//4 - BYTE EA
	//3 - OFFSET
	//2 - EA
	//1 - LOWORD SHIFTED
	//0 - HIWORD SHIFTED

	////////////////////////
	//Higher 32-bits
	////////////////////////

	//Get the mask for this register
	m_pB->PushValueAt(3);
	m_pB->Lookup(m_nLDRMaskHi);
	//5 - BYTE EA
	//4 - OFFSET
	//3 - EA
	//2 - LOWORD SHIFTED
	//1 - HIWORD SHIFTED
	//0 - HIMASK

	//Obtain the value of the register, mask, or with the value, sign extend, save
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[1]);
	m_pB->And();
	m_pB->Or();
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[1]);
	//3 - BYTE EA
	//2 - OFFSET
	//1 - EA
	//0 - LOWORD SHIFTED

	////////////////////////
	//Lower 32-bits
	////////////////////////

	//Get the mask for this register
	m_pB->PushValueAt(2);
	m_pB->Lookup(m_nLDRMaskLo);
	//4 - BYTE EA
	//3 - OFFSET
	//2 - EA
	//1 - LOWORD SHIFTED
	//0 - LOMASK

	//Obtain the value of the register, mask, or with the value and save
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->And();
	m_pB->Or();
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	//2 - BYTE EA
	//1 - OFFSET
	//0 - EA

	//Done, free the stack
	m_pB->Free(3);
}

//20
void CMA_MIPSIV::LB()
{
	ComputeMemAccessAddr();

	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::GetByteProxy, 2, true);
	m_pB->SeX8();
	SignExtendTop32(m_nRT);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
}

//21
void CMA_MIPSIV::LH()
{
	ComputeMemAccessAddr();

	//Load the word
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::GetHalfProxy, 2, true);
	m_pB->SeX16();
	SignExtendTop32(m_nRT);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
}

//22
void CMA_MIPSIV::LWL()
{
	ComputeMemAccessAddr();

	m_pB->PushValueAt(0);
	m_pB->AndImm(0x00000003);
	m_pB->PushValueAt(1);
	m_pB->AndImm(~0x00000003);
	//2 - BYTE EA
	//1 - OFFSET
	//0 - EA

	//Get the word
	m_pB->PushValueAt(0);
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::GetWordProxy, 2, true);
	//3 - BYTE EA
	//2 - OFFSET
	//1 - EA
	//0 - WORD

	//Shift the word left
	m_pB->PushValueAt(2);
	m_pB->Lookup(m_nLWLShift);
	m_pB->Sll();
	//3 - BYTE EA
	//2 - OFFSET
	//1 - EA
	//0 - WORD SHIFTED

	//Get the mask for this register
	m_pB->PushValueAt(2);
	m_pB->Lookup(m_nLWLMask);
	//4 - BYTE EA
	//3 - OFFSET
	//2 - EA
	//1 - WORD SHIFTED
	//0 - MASK

	//Obtain the value of the register, mask, or with the value, sign extend, save
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->And();
	m_pB->Or();
	SignExtendTop32(m_nRT);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	//2 - BYTE EA
	//1 - OFFSET
	//0 - EA

	//Done, free the stack
	m_pB->Free(3);
}

//23
void CMA_MIPSIV::LW()
{
	ComputeMemAccessAddr();

	//Load the word
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::GetWordProxy, 2, true);
	SignExtendTop32(m_nRT);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);

}

//24
void CMA_MIPSIV::LBU()
{
	ComputeMemAccessAddr();

	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::GetByteProxy, 2, true);
	SignExtendTop32(m_nRT);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
}

//25
void CMA_MIPSIV::LHU()
{
	ComputeMemAccessAddr();
	
	//Get the half
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::GetHalfProxy, 2, true);
	SignExtendTop32(m_nRT);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
}

//26
void CMA_MIPSIV::LWR()
{
	ComputeMemAccessAddr();

	m_pB->PushValueAt(0);
	m_pB->AndImm(0x00000003);
	m_pB->PushValueAt(1);
	m_pB->AndImm(~0x00000003);
	//2 - BYTE EA
	//1 - OFFSET
	//0 - EA

	//Get the word
	m_pB->PushValueAt(0);
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::GetWordProxy, 2, true);
	//3 - BYTE EA
	//2 - OFFSET
	//1 - EA
	//0 - WORD

	//Shift the word right
	m_pB->PushValueAt(2);
	m_pB->Lookup(m_nLWRShift);
	m_pB->Srl();
	//3 - BYTE EA
	//2 - OFFSET
	//1 - EA
	//0 - WORD SHIFTED

	//Get the mask for this register
	m_pB->PushValueAt(2);
	m_pB->Lookup(m_nLWRMask);
	//4 - BYTE EA
	//3 - OFFSET
	//2 - EA
	//1 - WORD SHIFTED
	//0 - MASK

	//Obtain the value of the register, mask, or with the value, sign extend, save
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->And();
	m_pB->Or();
	SignExtendTop32(m_nRT);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	//2 - BYTE EA
	//1 - OFFSET
	//0 - EA

	//Done, free the stack
	m_pB->Free(3);
}

//27
void CMA_MIPSIV::LWU()
{
	ComputeMemAccessAddr();

	//Load the word
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::GetWordProxy, 2, true);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);

	//Zero extend the value
	m_pB->PushImm(0);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[1]);
}

//28
void CMA_MIPSIV::SB()
{
	ComputeMemAccessAddr();

	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::SetByteProxy, 3, false);
}

//29
void CMA_MIPSIV::SH()
{
	ComputeMemAccessAddr();

	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::SetHalfProxy, 3, false);
}

//2A
void CMA_MIPSIV::SWL()
{
	ComputeMemAccessAddr();

	m_pB->PushValueAt(0);
	m_pB->AndImm(0x00000003);
	m_pB->PushValueAt(1);
	m_pB->AndImm(~0x00000003);
	//2 - BYTE EA
	//1 - OFFSET
	//0 - EA

	//Get the reg value
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	//3 - BYTE EA
	//2 - OFFSET
	//1 - EA
	//0 - REG

	//Get shift the register right
	m_pB->PushValueAt(2);
	m_pB->Lookup(m_nSWLShift);
	m_pB->Srl();
	//3 - BYTE EA
	//2 - OFFSET
	//1 - EA
	//0 - REG SHIFTED

	//Get the mask for this register
	m_pB->PushValueAt(2);
	m_pB->Lookup(m_nSWLMask);
	//4 - BYTE EA
	//3 - OFFSET
	//2 - EA
	//1 - REG SHIFTED
	//0 - MASK

	//Obtain the value of memory, mask, or with the value and save
	m_pB->PushValueAt(2);
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::GetWordProxy, 2, true);
	//5 - BYTE EA
	//4 - OFFSET
	//3 - EA
	//2 - REG SHIFTED
	//1 - MASK
	//0 - WORD

	m_pB->And();
	m_pB->Or();
	//3 - BYTE EA
	//2 - OFFSET
	//1 - EA
	//0 - WORD VALUE

	m_pB->PushValueAt(1);
	m_pB->Swap();
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::SetWordProxy, 3, false);
	//2 - BYTE EA
	//1 - OFFSET
	//0 - EA

	//Done, free the stack
	m_pB->Free(3);
}

//2B
void CMA_MIPSIV::SW()
{
	ComputeMemAccessAddr();

	//Write the words
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::SetWordProxy, 3, false);
}

//2C
void CMA_MIPSIV::SDL()
{
	ComputeMemAccessAddr();

	m_pB->PushValueAt(0);
	m_pB->AndImm(0x00000007);
	m_pB->PushValueAt(1);
	m_pB->AndImm(~0x00000007);
	//2 - BYTE EA
	//1 - OFFSET
	//0 - EA

	//Get the lo/hi reg values
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[1]);
	//4 - BYTE EA
	//3 - OFFSET
	//2 - EA
	//1 - LOREG
	//0 - HIREG

	//Get shift the register right
	m_pB->PushValueAt(3);
	m_pB->Lookup(m_nSDLShift);
	m_pB->Srl64();
	//4 - BYTE EA
	//3 - OFFSET
	//2 - EA
	//1 - LOREG SHIFTED
	//0 - HIREG SHIFTED

	////////////////////////
	//Higher 32-bits
	////////////////////////

	//Get the mask for this register
	m_pB->PushValueAt(3);
	m_pB->Lookup(m_nSDLMaskHi);
	//5 - BYTE EA
	//4 - OFFSET
	//3 - EA
	//2 - LOREG SHIFTED
	//1 - HIREG SHIFTED
	//0 - HIMASK

	//Obtain the value of memory, mask, or with the value and save
	m_pB->PushValueAt(3);
	m_pB->AddImm(4);
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::GetWordProxy, 2, true);
	//6 - BYTE EA
	//5 - OFFSET
	//4 - EA
	//3 - LOREG SHIFTED
	//2 - HIREG SHIFTED
	//1 - HIMASK
	//0 - HIWORD

	m_pB->And();
	m_pB->Or();
	//4 - BYTE EA
	//3 - OFFSET
	//2 - EA
	//1 - LOREG SHIFTED
	//0 - HIWORD VALUE

	m_pB->PushValueAt(2);
	m_pB->AddImm(4);
	m_pB->Swap();
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::SetWordProxy, 3, false);
	//3 - BYTE EA
	//2 - OFFSET
	//1 - EA
	//0 - LOREG SHIFTED

	////////////////////////
	//Lower 32-bits
	////////////////////////

	//Get the mask for this register
	m_pB->PushValueAt(2);
	m_pB->Lookup(m_nSDLMaskLo);
	//4 - BYTE EA
	//3 - OFFSET
	//2 - EA
	//1 - LOREG SHIFTED
	//0 - LOMASK

	//Obtain the value of memory, mask, or with the value and save
	m_pB->PushValueAt(2);
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::GetWordProxy, 2, true);
	//5 - BYTE EA
	//4 - OFFSET
	//3 - EA
	//2 - LOREG SHIFTED
	//1 - LOMASK
	//0 - LOWORD

	m_pB->And();
	m_pB->Or();
	//3 - BYTE EA
	//2 - OFFSET
	//1 - EA
	//0 - LOWORD VALUE

	m_pB->PushValueAt(1);
	m_pB->Swap();
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::SetWordProxy, 3, false);
	//2 - BYTE EA
	//1 - OFFSET
	//0 - EA

	//Done, free the stack
	m_pB->Free(3);
}

//2D
void CMA_MIPSIV::SDR()
{
	ComputeMemAccessAddr();

	m_pB->PushValueAt(0);
	m_pB->AndImm(0x00000007);
	m_pB->PushValueAt(1);
	m_pB->AndImm(~0x00000007);
	//2 - BYTE EA
	//1 - OFFSET
	//0 - EA

	//Get the lo/hi reg values
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[1]);
	//4 - BYTE EA
	//3 - OFFSET
	//2 - EA
	//1 - LOREG
	//0 - HIREG

	//Get shift the register left
	m_pB->PushValueAt(3);
	m_pB->Lookup(m_nSDRShift);
	m_pB->Sll64();
	//4 - BYTE EA
	//3 - OFFSET
	//2 - EA
	//1 - LOREG SHIFTED
	//0 - HIREG SHIFTED

	////////////////////////
	//Higher 32-bits
	////////////////////////

	//Get the mask for this register
	m_pB->PushValueAt(3);
	m_pB->Lookup(m_nSDRMaskHi);
	//5 - BYTE EA
	//4 - OFFSET
	//3 - EA
	//2 - LOREG SHIFTED
	//1 - HIREG SHIFTED
	//0 - HIMASK

	//Obtain the value of memory, mask, or with the value and save
	m_pB->PushValueAt(3);
	m_pB->AddImm(4);
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::GetWordProxy, 2, true);
	//6 - BYTE EA
	//5 - OFFSET
	//4 - EA
	//3 - LOREG SHIFTED
	//2 - HIREG SHIFTED
	//1 - HIMASK
	//0 - HIWORD

	m_pB->And();
	m_pB->Or();
	//4 - BYTE EA
	//3 - OFFSET
	//2 - EA
	//1 - LOREG SHIFTED
	//0 - HIWORD VALUE

	m_pB->PushValueAt(2);
	m_pB->AddImm(4);
	m_pB->Swap();
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::SetWordProxy, 3, false);
	//3 - BYTE EA
	//2 - OFFSET
	//1 - EA
	//0 - LOREG SHIFTED

	////////////////////////
	//Lower 32-bits
	////////////////////////

	//Get the mask for this register
	m_pB->PushValueAt(2);
	m_pB->Lookup(m_nSDRMaskLo);
	//4 - BYTE EA
	//3 - OFFSET
	//2 - EA
	//1 - LOREG SHIFTED
	//0 - LOMASK

	//Obtain the value of memory, mask, or with the value and save
	m_pB->PushValueAt(2);
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::GetWordProxy, 2, true);
	//5 - BYTE EA
	//4 - OFFSET
	//3 - EA
	//2 - LOREG SHIFTED
	//1 - LOMASK
	//0 - LOWORD

	m_pB->And();
	m_pB->Or();
	//3 - BYTE EA
	//2 - OFFSET
	//1 - EA
	//0 - LOWORD VALUE

	m_pB->PushValueAt(1);
	m_pB->Swap();
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::SetWordProxy, 3, false);
	//2 - BYTE EA
	//1 - OFFSET
	//0 - EA

	//Done, free the stack
	m_pB->Free(3);
}

//2E
void CMA_MIPSIV::SWR()
{
	ComputeMemAccessAddr();

	m_pB->PushValueAt(0);
	m_pB->AndImm(0x00000003);
	m_pB->PushValueAt(1);
	m_pB->AndImm(~0x00000003);
	//2 - BYTE EA
	//1 - OFFSET
	//0 - EA

	//Get the reg value
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	//3 - BYTE EA
	//2 - OFFSET
	//1 - EA
	//0 - REG

	//Get shift the register left
	m_pB->PushValueAt(2);
	m_pB->Lookup(m_nSWRShift);
	m_pB->Sll();
	//3 - BYTE EA
	//2 - OFFSET
	//1 - EA
	//0 - REG SHIFTED

	//Get the mask for this register
	m_pB->PushValueAt(2);
	m_pB->Lookup(m_nSWRMask);
	//4 - BYTE EA
	//3 - OFFSET
	//2 - EA
	//1 - REG SHIFTED
	//0 - MASK

	//Obtain the value of memory, mask, or with the value and save
	m_pB->PushValueAt(2);
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::GetWordProxy, 2, true);
	//5 - BYTE EA
	//4 - OFFSET
	//3 - EA
	//2 - REG SHIFTED
	//1 - MASK
	//0 - WORD

	m_pB->And();
	m_pB->Or();
	//3 - BYTE EA
	//2 - OFFSET
	//1 - EA
	//0 - WORD VALUE

	m_pB->PushValueAt(1);
	m_pB->Swap();
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::SetWordProxy, 3, false);
	//2 - BYTE EA
	//1 - OFFSET
	//0 - EA

	//Done, free the stack
	m_pB->Free(3);
}

//2F
void CMA_MIPSIV::CACHE()
{
	//No cache in our system. Nothing to do.
}

//31
void CMA_MIPSIV::LWC1()
{
	if(m_pCtx->m_pCOP[1] != NULL)
	{
		m_pCtx->m_pCOP[1]->CompileInstruction(m_nAddress, m_pB, m_pCtx, false);
	}
	else
	{
		Illegal();
	}
}

//36
void CMA_MIPSIV::LDC2()
{
	if(m_pCtx->m_pCOP[2] != NULL)
	{
		m_pCtx->m_pCOP[2]->CompileInstruction(m_nAddress, m_pB, m_pCtx, false);
	}
	else
	{
		Illegal();
	}
}

//37
void CMA_MIPSIV::LD()
{
	ComputeMemAccessAddr();

	//Load the word
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::GetWordProxy, 1, true);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->AddImm(4);

	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::GetWordProxy, 2, true);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[1]);
}

//39
void CMA_MIPSIV::SWC1()
{
	if(m_pCtx->m_pCOP[1] != NULL)
	{
		m_pCtx->m_pCOP[1]->CompileInstruction(m_nAddress, m_pB, m_pCtx, false);
	}
	else
	{
		Illegal();
	}
}

//3E
void CMA_MIPSIV::SDC2()
{
	if(m_pCtx->m_pCOP[2] != NULL)
	{
		m_pCtx->m_pCOP[2]->CompileInstruction(m_nAddress, m_pB, m_pCtx, false);
	}
	else
	{
		Illegal();
	}
}

//3F
void CMA_MIPSIV::SD()
{
	ComputeMemAccessAddr();

	//Write the words
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::SetWordProxy, 2, false);
	m_pB->AddImm(4);

	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[1]);
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::SetWordProxy, 3, false);
}

//////////////////////////////////////////////////
//Special Opcodes
//////////////////////////////////////////////////

//00
void CMA_MIPSIV::SLL()
{
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->SllImm(m_nSA);
	SignExtendTop32(m_nRD);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[0]);
}

//02
void CMA_MIPSIV::SRL()
{
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->SrlImm(m_nSA);
	SignExtendTop32(m_nRD);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[0]);
}

//03
void CMA_MIPSIV::SRA()
{
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->SraImm(m_nSA);
	SignExtendTop32(m_nRD);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[0]);
}

//04
void CMA_MIPSIV::SLLV()
{
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->Sll();
	SignExtendTop32(m_nRD);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[0]);
}

//06
void CMA_MIPSIV::SRLV()
{
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->Srl();
	SignExtendTop32(m_nRD);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[0]);
}

//07
void CMA_MIPSIV::SRAV()
{
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->Sra();
	SignExtendTop32(m_nRD);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[0]);
}

//08
void CMA_MIPSIV::JR()
{
	//TODO: 64-bits addresses

	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->PullAddr(&m_pCtx->m_State.nDelayedJumpAddr);

	m_pB->SetDelayedJumpCheck();
}

//09
void CMA_MIPSIV::JALR()
{
	//TODO: 64-bits addresses

	//Set the jump address
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->PullAddr(&m_pCtx->m_State.nDelayedJumpAddr);

	//Save address in register
	m_pB->PushAddr(&m_pCtx->m_State.nPC);
	m_pB->AddImm(4);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[0]);

	m_pB->SetDelayedJumpCheck();
}

//0A
void CMA_MIPSIV::MOVZ()
{
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->PushImm(0);
	m_pB->Cmp(JCC_CONDITION_EQ);

	m_pB->BeginJcc(true);
	{
		m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
		m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[0]);

		m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[1]);
		m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[1]);
	}
	m_pB->EndJcc();
}

//0B
void CMA_MIPSIV::MOVN()
{
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->PushImm(0);
	m_pB->Cmp(JCC_CONDITION_EQ);

	m_pB->BeginJcc(false);
	{
		m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
		m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[0]);

		m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[1]);
		m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[1]);
	}
	m_pB->EndJcc();
}

//0C
void CMA_MIPSIV::SYSCALL()
{
	m_pB->Call(m_pCtx->m_pSysCallHandler, 0, false);
	m_pB->SetProgramCounterChanged();
}

//0F
void CMA_MIPSIV::SYNC()
{
	//NOP
}

//10
void CMA_MIPSIV::MFHI()
{
	m_pB->PushAddr(&m_pCtx->m_State.nHI[0]);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[0]);

	m_pB->PushAddr(&m_pCtx->m_State.nHI[1]);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[1]);
}

//11
void CMA_MIPSIV::MTHI()
{
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->PullAddr(&m_pCtx->m_State.nHI[0]);

	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[1]);
	m_pB->PullAddr(&m_pCtx->m_State.nHI[1]);
}

//12
void CMA_MIPSIV::MFLO()
{
	m_pB->PushAddr(&m_pCtx->m_State.nLO[0]);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[0]);

	m_pB->PushAddr(&m_pCtx->m_State.nLO[1]);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[1]);
}

//13
void CMA_MIPSIV::MTLO()
{
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->PullAddr(&m_pCtx->m_State.nLO[0]);

	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[1]);
	m_pB->PullAddr(&m_pCtx->m_State.nLO[1]);
}

//14
void CMA_MIPSIV::DSLLV()
{
	CCodeGen::Begin(m_pB);
	{
		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));

		CCodeGen::Shl64();

		CCodeGen::PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
		CCodeGen::PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
	}
	CCodeGen::End();
}

//16
void CMA_MIPSIV::DSRLV()
{
	CCodeGen::Begin(m_pB);
	{
		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));

		CCodeGen::Srl64();

		CCodeGen::PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
		CCodeGen::PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
	}
	CCodeGen::End();
}

//18
void CMA_MIPSIV::MULT()
{
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->MultS();

	m_pB->SeX32();
	m_pB->PullAddr(&m_pCtx->m_State.nLO[1]);
	m_pB->PullAddr(&m_pCtx->m_State.nLO[0]);

	m_pB->SeX32();
	m_pB->PullAddr(&m_pCtx->m_State.nHI[1]);
	m_pB->PullAddr(&m_pCtx->m_State.nHI[0]);

	if(m_nRD != 0)
	{
		//Wierd EE MIPS spinoff...
		m_pB->PushAddr(&m_pCtx->m_State.nLO[0]);
		SignExtendTop32(m_nRD);
		m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[0]);
	}
}

//19
void CMA_MIPSIV::MULTU()
{
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->Mult();

	m_pB->SeX32();
	m_pB->PullAddr(&m_pCtx->m_State.nLO[1]);
	m_pB->PullAddr(&m_pCtx->m_State.nLO[0]);

	m_pB->SeX32();
	m_pB->PullAddr(&m_pCtx->m_State.nHI[1]);
	m_pB->PullAddr(&m_pCtx->m_State.nHI[0]);

	if(m_nRD != 0)
	{
		//Wierd EE MIPS spinoff...
		m_pB->PushAddr(&m_pCtx->m_State.nLO[0]);
		SignExtendTop32(m_nRD);
		m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[0]);
	}
}

//1A
void CMA_MIPSIV::DIV()
{
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->DivS();

	m_pB->SeX32();
	m_pB->PullAddr(&m_pCtx->m_State.nLO[1]);
	m_pB->PullAddr(&m_pCtx->m_State.nLO[0]);

	m_pB->SeX32();
	m_pB->PullAddr(&m_pCtx->m_State.nHI[1]);
	m_pB->PullAddr(&m_pCtx->m_State.nHI[0]);
}

//1B
void CMA_MIPSIV::DIVU()
{
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->Div();

	m_pB->SeX32();
	m_pB->PullAddr(&m_pCtx->m_State.nLO[1]);
	m_pB->PullAddr(&m_pCtx->m_State.nLO[0]);

	m_pB->SeX32();
	m_pB->PullAddr(&m_pCtx->m_State.nHI[1]);
	m_pB->PullAddr(&m_pCtx->m_State.nHI[0]);
}

//20
void CMA_MIPSIV::ADD()
{
	CCodeGen::Begin(m_pB);
	{
		CCodeGen::PushVar(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
		CCodeGen::PushVar(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
		CCodeGen::Add();
		CCodeGen::SeX();
		CCodeGen::PullVar(&m_pCtx->m_State.nGPR[m_nRD].nV[1]);
		CCodeGen::PullVar(&m_pCtx->m_State.nGPR[m_nRD].nV[0]);
	}
	CCodeGen::End();
}

//21
void CMA_MIPSIV::ADDU()
{
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->Add();
	SignExtendTop32(m_nRD);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[0]);
}

//23
void CMA_MIPSIV::SUBU()
{
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->Sub();
	SignExtendTop32(m_nRD);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[0]);
}

//24
void CMA_MIPSIV::AND()
{
	CCodeGen::Begin(m_pB);
	{
		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));

		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

		CCodeGen::And64();

		CCodeGen::PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
		CCodeGen::PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
	}
	CCodeGen::End();
}

//25
void CMA_MIPSIV::OR()
{
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->Or();
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[0]);

	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[1]);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[1]);
	m_pB->Or();
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[1]);
}

//26
void CMA_MIPSIV::XOR()
{
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->Xor();
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[0]);

	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[1]);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[1]);
	m_pB->Xor();
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[1]);
}

//27
void CMA_MIPSIV::NOR()
{
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->Or();
	m_pB->Not();
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[0]);

	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[1]);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[1]);
	m_pB->Or();
	m_pB->Not();
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[1]);
}

//2A
void CMA_MIPSIV::SLT()
{
	CCodeGen::Begin(m_pB);
	{
		CCodeGen::PushVar(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
		CCodeGen::PushVar(&m_pCtx->m_State.nGPR[m_nRS].nV[1]);

		CCodeGen::PushVar(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
		CCodeGen::PushVar(&m_pCtx->m_State.nGPR[m_nRT].nV[1]);

		CCodeGen::Cmp64(CCodeGen::CONDITION_LT);

		CCodeGen::PullVar(&m_pCtx->m_State.nGPR[m_nRD].nV[0]);

		//Clear higher 32-bits
		CCodeGen::PushCst(0);
		CCodeGen::PullVar(&m_pCtx->m_State.nGPR[m_nRD].nV[1]);
	}
	CCodeGen::End();
}

//2B
void CMA_MIPSIV::SLTU()
{
	CCodeGen::Begin(m_pB);
	{
		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));

		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

		CCodeGen::Cmp64(CCodeGen::CONDITION_BL);

		CCodeGen::PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));

		//Clear higher 32-bits
		CCodeGen::PushCst(0);
		CCodeGen::PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
	}
	CCodeGen::End();
}

//2D
void CMA_MIPSIV::DADDU()
{
	CCodeGen::Begin(m_pB);
	{
		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));

		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

		CCodeGen::Add64();

		CCodeGen::PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
		CCodeGen::PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
	}
	CCodeGen::End();
}

//2F
void CMA_MIPSIV::DSUBU()
{
	//First 32-bits half
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->SubC();
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[0]);

	//2nd 32-bits half
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[1]);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[1]);
	m_pB->Sub();
	m_pB->Swap();
	m_pB->Sub();
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[1]);
}

//38
void CMA_MIPSIV::DSLL()
{
	//1st 32-bits half
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->SllImm(m_nSA);

	//2nd 32-bits half
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[1]);
	m_pB->SllImm(m_nSA);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->SrlImm(32 - m_nSA);
	m_pB->Or();

	//Write the result
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[1]);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[0]);
}

//3A
void CMA_MIPSIV::DSRL()
{
	CCodeGen::Begin(m_pB);
	{
		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

		CCodeGen::Srl64(m_nSA);

		CCodeGen::PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
		CCodeGen::PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
	}
	CCodeGen::End();
}

//3B
void CMA_MIPSIV::DSRA()
{
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[1]);
	m_pB->PushImm(m_nSA);

	m_pB->Sra64();

	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[1]);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[0]);
}

//3C
void CMA_MIPSIV::DSLL32()
{
	CCodeGen::Begin(m_pB);
	{
		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

		CCodeGen::Shl64(m_nSA + 0x20);

		CCodeGen::PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
		CCodeGen::PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
	}
	CCodeGen::End();
}

//3E
void CMA_MIPSIV::DSRL32()
{
	CCodeGen::Begin(m_pB);
	{
		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
		CCodeGen::PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

		CCodeGen::Srl64(m_nSA + 32);

		CCodeGen::PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
		CCodeGen::PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
	}
	CCodeGen::End();
}

//3F
void CMA_MIPSIV::DSRA32()
{
	//1st 32-bits
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[1]);
	m_pB->SraImm(m_nSA);

	//2nd 32-bits
	m_pB->SeX32();

	//Write the result
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[1]);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[0]);
}

//////////////////////////////////////////////////
//Special2 Opcodes
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//RegImm Opcodes
//////////////////////////////////////////////////

//00
void CMA_MIPSIV::BLTZ()
{
	m_pB->PushImm(0);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[1]);
	m_pB->AndImm(0x80000000);

	m_pB->Cmp(JCC_CONDITION_EQ);

	Branch(false);
}

//01
void CMA_MIPSIV::BGEZ()
{
	m_pB->PushImm(0);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[1]);
	m_pB->AndImm(0x80000000);

	m_pB->Cmp(JCC_CONDITION_EQ);

	Branch(true);
}

//02
void CMA_MIPSIV::BLTZL()
{
	m_pB->PushImm(0);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[1]);
	m_pB->AndImm(0x80000000);

	m_pB->Cmp(JCC_CONDITION_EQ);

	BranchLikely(false);
}

//03
void CMA_MIPSIV::BGEZL()
{
	m_pB->PushImm(0);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[1]);
	m_pB->AndImm(0x80000000);

	m_pB->Cmp(JCC_CONDITION_EQ);

	BranchLikely(true);
}

//////////////////////////////////////////////////
//Opcode Tables
//////////////////////////////////////////////////

void (*CMA_MIPSIV::m_pOpGeneral[0x40])() =
{
	//0x00
	SPECIAL,		REGIMM,			J,				JAL,			BEQ,			BNE,			BLEZ,			BGTZ,
	//0x08
	ADDI,			ADDIU,			SLTI,			SLTIU,			ANDI,			ORI,			XORI,			LUI,
	//0x10
	COP0,			COP1,			COP2,			Illegal,		BEQL,			BNEL,			BLEZL,			BGTZL,
	//0x18
	Illegal,		DADDIU,			LDL,			LDR,			SPECIAL2,		Illegal,		Illegal,		Illegal,
	//0x20
	LB,				LH,				LWL,			LW,				LBU,			LHU,			LWR,			LWU,
	//0x28
	SB,				SH,				SWL,			SW,				SDL,			SDR,			SWR,			CACHE,
	//0x30
	Illegal,		LWC1,			Illegal,		Illegal,		Illegal,		Illegal,		LDC2,			LD,
	//0x38
	Illegal,		SWC1,			Illegal,		Illegal,		Illegal,		Illegal,		SDC2,			SD,
};

void (*CMA_MIPSIV::m_pOpSpecial[0x40])() = 
{
	//0x00
	SLL,			Illegal,		SRL,			SRA,			SLLV,			Illegal,		SRLV,			SRAV,
	//0x08
	JR,				JALR,			MOVZ,			MOVN,			SYSCALL,		Illegal,		Illegal,		SYNC,
	//0x10
	MFHI,			MTHI,			MFLO,			MTLO,			DSLLV,			Illegal,		DSRLV,			Illegal,
	//0x18
	MULT,			MULTU,			DIV,			DIVU,			Illegal,		Illegal,		Illegal,		Illegal,
	//0x20
	ADD,			ADDU,			Illegal,		SUBU,			AND,			OR,				XOR,			NOR,
	//0x28
	Illegal,		Illegal,		SLT,			SLTU,			Illegal,		DADDU,			Illegal,		DSUBU,
	//0x30
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x38
	DSLL,			Illegal,		DSRL,			DSRA,			DSLL32,			Illegal,		DSRL32,			DSRA32,
};

void (*CMA_MIPSIV::m_pOpSpecial2[0x40])() = 
{
	//0x00
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x08
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x10
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x18
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x20
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x28
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x30
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x38
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
};

void (*CMA_MIPSIV::m_pOpRegImm[0x20])() = 
{
	//0x00
	BLTZ,			BGEZ,			BLTZL,			BGEZL,			Illegal,		Illegal,		Illegal,		Illegal,
	//0x08
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x10
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x18
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
};
