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

CCOP_SCU::CCOP_SCU(MIPS_REGSIZE nRegSize) :
CMIPSCoprocessor(nRegSize),
m_nRT(0),
m_nRD(0)
{

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
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP0[m_nRD]));

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
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP0[m_nRD]));
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

//////////////////////////////////////////////////
//Coprocessor Specific Opcodes
//////////////////////////////////////////////////

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
	&CCOP_SCU::MFC0,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::MTC0,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,
	//0x08
	&CCOP_SCU::BC0,   		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,
	//0x10
	&CCOP_SCU::C0,			&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,
	//0x18
	&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,
};

CCOP_SCU::InstructionFuncConstant CCOP_SCU::m_pOpBC0[0x20] = 
{
	//0x00
	&CCOP_SCU::BC0F,		&CCOP_SCU::BC0T,  		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,
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
	&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,		&CCOP_SCU::Illegal,
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
    case 0x08:
        switch((nOpcode >> 16) & 0x1F)
        {
        case 0x00:
            strcpy(sText, "BC0F");
            break;
        case 0x01:
            strcpy(sText, "BC0T");
            break;
        default:
            strcpy(sText, "???");
            break;
        }
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
	uint8 nRT = static_cast<uint8>((nOpcode >> 16) & 0x1F);
	uint8 nRD = static_cast<uint8>((nOpcode >> 11) & 0x1F);
    uint16 nImm = static_cast<uint16>(nOpcode);
	switch((nOpcode >> 21) & 0x1F)
	{
	case 0x00:
	case 0x04:
		sprintf(sText, "%s, %s", CMIPS::m_sGPRName[nRT], m_sRegName[nRD]);
		break;
    case 0x08:
        switch((nOpcode >> 16) & 0x1F)
        {
        case 0x00:
        case 0x01:
            sprintf(sText, "0x%0.8X", nAddress + CMIPS::GetBranch(nImm) + 4);
            break;
        default:
            strcpy(sText, "");
            break;
        }
        break;
	default:
		strcpy(sText, "");
		break;
	}
}

uint32 CCOP_SCU::GetEffectiveAddress(uint32 nAddress, uint32 nOpcode)
{
    if(((nOpcode >> 21) & 0x1F) == 0x08)
    {
        switch((nOpcode >> 16) & 0x1F)
        {
        case 0x00:
        case 0x01:
	        return (nAddress + CMIPS::GetBranch(static_cast<uint16>(nOpcode)) + 4);
            break;
        default:
            return 0;
            break;
        }
    }
    return 0;
}

bool CCOP_SCU::IsBranch(uint32 nOpcode)
{
    if(((nOpcode >> 21) & 0x1F) == 0x08)
    {
        switch((nOpcode >> 16) & 0x1F)
        {
        case 0x00:
        case 0x01:
            return true;
            break;
        default:
            return false;
            break;
        }
    }
    return false;
}
