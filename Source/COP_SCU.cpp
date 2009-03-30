#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include "COP_SCU.h"
#include "MIPS.h"
#include "CodeGen.h"
#include "offsetof_def.h"

char*		CCOP_SCU::m_sRegName[] = 
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
	"*RESERVED*",
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

CCOP_SCU::CCOP_SCU(MIPS_REGSIZE nRegSize) :
CMIPSCoprocessor(nRegSize),
m_nRT(0),
m_nRD(0)
{

}

void CCOP_SCU::CompileInstruction(uint32 nAddress, CCodeGen* codeGen, CMIPS* pCtx)
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
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP0[m_nRD]));
    m_codeGen->SeX();
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
}

//04
void CCOP_SCU::MTC0()
{
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP0[m_nRD]));
}

//10
void CCOP_SCU::CO()
{
	((this)->*(m_pOpCO[(m_nOpcode & 0x3F)]))();
}

//////////////////////////////////////////////////
//Coprocessor Specific Opcodes
//////////////////////////////////////////////////

//18
void CCOP_SCU::ERET()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP0[STATUS]));
	m_codeGen->PushCst(0x04);
	m_codeGen->And();
	
	m_codeGen->PushCst(0x00);
	m_codeGen->Cmp(CCodeGen::CONDITION_EQ);

	m_codeGen->BeginIfElse(false);
	{
		//ERL bit was set
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP0[ERROREPC]));
//		m_codeGen->PullRel(offsetof(CMIPS, m_State.nPC));
        m_codeGen->PullRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));

		//Clear ERL bit
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP0[STATUS]));
		m_codeGen->PushCst(~0x04);
		m_codeGen->And();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP0[STATUS]));
	}
	m_codeGen->BeginIfElseAlt();
	{
		//EXL bit wasn't set, we assume ERL was (unsafe)
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP0[EPC]));
//		m_codeGen->PullRel(offsetof(CMIPS, m_State.nPC));
        m_codeGen->PullRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));

		//Clear EXL bit
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP0[STATUS]));
		m_codeGen->PushCst(~0x02);
		m_codeGen->And();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP0[STATUS]));
	}
	m_codeGen->EndIf();
}

//38
void CCOP_SCU::EI()
{
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP0[STATUS]));
	//Should check for pending interrupts here
    m_codeGen->PushCst(0x00010001);
    m_codeGen->Or();
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP0[STATUS]));
}

//39
void CCOP_SCU::DI()
{
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP0[STATUS]));
    m_codeGen->PushCst(~0x00010001);
    m_codeGen->And();
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP0[STATUS]));
}

//////////////////////////////////////////////////
//Opcode Tables
//////////////////////////////////////////////////

CCOP_SCU::InstructionFuncConstant CCOP_SCU::m_pOpGeneral[0x20] = 
{
	//0x00
	&MFC0,			&Illegal,		&Illegal,		&Illegal,		&MTC0,			&Illegal,		&Illegal,		&Illegal,
	//0x08
	&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,
	//0x10
	&CO,			&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,
	//0x18
	&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,
};

CCOP_SCU::InstructionFuncConstant CCOP_SCU::m_pOpCO[0x40] =
{
	//0x00
	&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,
	//0x08
	&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,
	//0x10
	&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,
	//0x18
	&ERET,			&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,
	//0x20
	&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,
	//0x28
	&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,
	//0x30
	&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,
	//0x38
	&EI,			&DI,			&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,		&Illegal,
};

void CCOP_SCU::GetInstruction(uint32 nOpcode, char* sText)
{
	switch((nOpcode >> 21) & 0x1F)
	{
	case 0x00:
		strcpy(sText, "MFC0");
		break;
	case 0x04:
		strcpy(sText, "MTC0");
		break;
	case 0x10:
		switch(nOpcode & 0x3F)
		{
		case 0x02:
			strcpy(sText, "TLBWI");
			break;
		case 0x18:
			strcpy(sText, "ERET");
			break;
		case 0x38:
			strcpy(sText, "EI");
			break;
		case 0x39:
			strcpy(sText, "DI");
			break;
		default:
			strcpy(sText, "???");
			break;
		}
		break;
	default:
		strcpy(sText, "???");
		break;
	}
}

void CCOP_SCU::GetArguments(uint32 nAddress, uint32 nOpcode, char* sText)
{
	unsigned char nRD, nRT;
	nRT = (unsigned char)((nOpcode >> 16) & 0x1F);
	nRD = (unsigned char)((nOpcode >> 11) & 0x1F);
	switch((nOpcode >> 21) & 0x1F)
	{
	case 0x00:
	case 0x04:
		sprintf(sText, "%s, %s", CMIPS::m_sGPRName[nRT], m_sRegName[nRD]);
		break;
	default:
		strcpy(sText, "");
		break;
	}
}

uint32 CCOP_SCU::GetEffectiveAddress(uint32 nAddress, uint32 nData)
{
	return false;
}

bool CCOP_SCU::IsBranch(uint32 nData)
{
	return 0;
}
