#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include "COP_SCU.h"
#include "MIPS.h"
#include "Jitter.h"
#include "offsetof_def.h"

const char* CCOP_SCU::m_sRegName[] = 
{
	"Index",
	"Random",
	"EntryLo0",
	"EntryLo1",
	"Context",
	"PageMask",
	"Wired",
	"*RESERVED*",
	"BadVAddr",
	"Count",
	"EntryHi",
	"Compare",
	"Status",
	"Cause",
	"EPC",
	"PrevID",
	"Config",
	"LLAddr",
	"WatchLo",
	"WatchHi",
	"XContext",
	"CPCOND0",
	"*RESERVED*",
	"*RESERVED*",
	"*RESERVED*",
	"*RESERVED*",
	"PErr",
	"CacheErr",
	"TagLo",
	"TagHi",
	"ErrorEPC",
	"*RESERVED*"
};

CCOP_SCU::CCOP_SCU(MIPS_REGSIZE nRegSize)
: CMIPSCoprocessor(nRegSize)
, m_nRT(0)
, m_nRD(0)
{
	SetupReflectionTables();
}

void CCOP_SCU::CompileInstruction(uint32 nAddress, CMipsJitter* codeGen, CMIPS* pCtx)
{
	SetupQuickVariables(nAddress, codeGen, pCtx);

	m_nRT	= (uint8)((m_nOpcode >> 16) & 0x1F);
	m_nRD	= (uint8)((m_nOpcode >> 11) & 0x1F);

	((this)->*(m_pOpGeneral[(m_nOpcode >> 21) & 0x1F]))();
}

//////////////////////////////////////////////////
//General Opcodes
//////////////////////////////////////////////////

//00
void CCOP_SCU::MFC0()
{
	switch(m_nRD)
	{
	case 25:
		switch(m_nOpcode & 1)
		{
		case 0:
			//MFPS
			m_codeGen->PushRel(offsetof(CMIPS, m_State.cop0_pccr));
			break;
		case 1:
			//MFPC
			{
				uint32 reg = (m_nOpcode >> 1) & 1;
				m_codeGen->PushRel(offsetof(CMIPS, m_State.cop0_pcr[reg]));
			}
			break;
		default:
			assert(false);
			break;
		}
		break;
	default:
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP0[m_nRD]));
		break;
	}

	if(m_regSize == MIPS_REGSIZE_64)
	{
		m_codeGen->PushTop();
		m_codeGen->SignExt();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
	}
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
}

//04
void CCOP_SCU::MTC0()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));

	if(m_nRD == CCOP_SCU::STATUS)
	{
		//Keep the EXL bit
		//This is needed for Valkyrie Profile 2 which resets the EXL bit
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP0[m_nRD]));
		m_codeGen->PushCst(CMIPS::STATUS_EXL);
		m_codeGen->And();
		m_codeGen->Or();
	}

	switch(m_nRD)
	{
	case 25:
		switch(m_nOpcode & 1)
		{
		case 0:
			//MTPS
			//Mask out bits that stay 0
			m_codeGen->PushCst(0x800FFBFE);
			m_codeGen->And();
			m_codeGen->PullRel(offsetof(CMIPS, m_State.cop0_pccr));
			break;
		case 1:
			//MTPC
			{
				uint32 reg = (m_nOpcode >> 1) & 1;
				m_codeGen->PullRel(offsetof(CMIPS, m_State.cop0_pcr[reg]));
			}
			break;
		}
		break;
	default:
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP0[m_nRD]));
		break;
	}
}

//08
void CCOP_SCU::BC0()
{
	((this)->*(m_pOpBC0[m_nRT]))();
}

//10
void CCOP_SCU::C0()
{
	((this)->*(m_pOpC0[(m_nOpcode & 0x3F)]))();
}

//////////////////////////////////////////////////
//Branches
//////////////////////////////////////////////////

//00
void CCOP_SCU::BC0F()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP0[CPCOND0]));
	m_codeGen->PushCst(0);
	Branch(Jitter::CONDITION_EQ);
}

//01
void CCOP_SCU::BC0T()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP0[CPCOND0]));
	m_codeGen->PushCst(0);
	Branch(Jitter::CONDITION_NE);
}

//02
void CCOP_SCU::BC0FL()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP0[CPCOND0]));
	m_codeGen->PushCst(0);
	BranchLikely(Jitter::CONDITION_EQ);
}

//////////////////////////////////////////////////
//Coprocessor Specific Opcodes
//////////////////////////////////////////////////

//02
void CCOP_SCU::TLBWI()
{
	//TLB not supported for now
}

//18
void CCOP_SCU::ERET()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP0[STATUS]));
	m_codeGen->PushCst(CMIPS::STATUS_ERL);
	m_codeGen->And();
	
	m_codeGen->PushCst(0);
	m_codeGen->BeginIf(Jitter::CONDITION_NE);
	{
		//ERL bit was set
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP0[ERROREPC]));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));

		//Clear ERL bit
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP0[STATUS]));
		m_codeGen->PushCst(~CMIPS::STATUS_ERL);
		m_codeGen->And();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP0[STATUS]));
	}
	m_codeGen->Else();
	{
		//EXL bit was set
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP0[EPC]));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));

		//Clear EXL bit
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP0[STATUS]));
		m_codeGen->PushCst(~CMIPS::STATUS_EXL);
		m_codeGen->And();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP0[STATUS]));
	}
	m_codeGen->EndIf();

	//Force the main loop to do special processing
	m_codeGen->PushCst(MIPS_EXCEPTION_RETURNFROMEXCEPTION);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nHasException));
}

//38
void CCOP_SCU::EI()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP0[STATUS]));
	m_codeGen->PushCst(CMIPS::STATUS_EIE);
	m_codeGen->Or();
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP0[STATUS]));

	//Force the main loop to check for pending interrupts
	m_codeGen->PushCst(MIPS_EXCEPTION_CHECKPENDINGINT);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nHasException));
}

//39
void CCOP_SCU::DI()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP0[STATUS]));
	m_codeGen->PushCst(~CMIPS::STATUS_EIE);
	m_codeGen->And();
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP0[STATUS]));
}

//////////////////////////////////////////////////
//Opcode Tables
//////////////////////////////////////////////////

CCOP_SCU::InstructionFuncConstant CCOP_SCU::m_pOpGeneral[0x20] = 
{
	//0x00
	&CCOP_SCU::MFC0,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::MTC0,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,
	//0x08
	&CCOP_SCU::BC0,			&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,
	//0x10
	&CCOP_SCU::C0,			&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,
	//0x18
	&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,
};

CCOP_SCU::InstructionFuncConstant CCOP_SCU::m_pOpBC0[0x20] = 
{
	//0x00
	&CCOP_SCU::BC0F,		&CCOP_SCU::BC0T,		&CCOP_SCU::BC0FL,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,
	//0x08
	&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,
	//0x10
	&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,
	//0x18
	&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,
};

CCOP_SCU::InstructionFuncConstant CCOP_SCU::m_pOpC0[0x40] =
{
	//0x00
	&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::TLBWI,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,
	//0x08
	&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,
	//0x10
	&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,
	//0x18
	&CCOP_SCU::ERET,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,
	//0x20
	&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,
	//0x28
	&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,
	//0x30
	&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,
	//0x38
	&CCOP_SCU::EI,			&CCOP_SCU::DI,			&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,
};
