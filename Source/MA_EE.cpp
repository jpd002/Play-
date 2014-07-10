#include <stddef.h>
#include "MA_EE.h"
#include "MIPS.h"
#include "PS2OS.h"
#include "offsetof_def.h"
#include "MemoryUtils.h"

CMA_EE::CMA_EE() :
CMA_MIPSIV(MIPS_REGSIZE_64)
{
	m_pOpGeneral[0x1E] = std::bind(&CMA_EE::LQ, this);
	m_pOpGeneral[0x1F] = std::bind(&CMA_EE::SQ, this);

	m_pOpRegImm[0x18] = std::bind(&CMA_EE::MTSAB, this);
	m_pOpRegImm[0x19] = std::bind(&CMA_EE::MTSAH, this);

	m_pOpSpecial2[0x00] = std::bind(&CMA_EE::MADD, this);
	m_pOpSpecial2[0x04] = std::bind(&CMA_EE::PLZCW, this);
	m_pOpSpecial2[0x08] = std::bind(&CMA_EE::MMI0, this);
	m_pOpSpecial2[0x09] = std::bind(&CMA_EE::MMI2, this);
	m_pOpSpecial2[0x10] = std::bind(&CMA_EE::MFHI1, this);
	m_pOpSpecial2[0x11] = std::bind(&CMA_EE::MTHI1, this);
	m_pOpSpecial2[0x12] = std::bind(&CMA_EE::MFLO1, this);
	m_pOpSpecial2[0x13] = std::bind(&CMA_EE::MTLO1, this);
	m_pOpSpecial2[0x18] = std::bind(&CMA_EE::MULT1, this);
	m_pOpSpecial2[0x19] = std::bind(&CMA_EE::MULTU1, this);
	m_pOpSpecial2[0x1A] = std::bind(&CMA_EE::DIV1, this);
	m_pOpSpecial2[0x1B] = std::bind(&CMA_EE::DIVU1, this);
	m_pOpSpecial2[0x20] = std::bind(&CMA_EE::MADD1, this);
	m_pOpSpecial2[0x28] = std::bind(&CMA_EE::MMI1, this);
	m_pOpSpecial2[0x29] = std::bind(&CMA_EE::MMI3, this);
	m_pOpSpecial2[0x30] = std::bind(&CMA_EE::PMFHL, this);
	m_pOpSpecial2[0x34] = std::bind(&CMA_EE::PSLLH, this);
	m_pOpSpecial2[0x36] = std::bind(&CMA_EE::PSRLH, this);
	m_pOpSpecial2[0x37] = std::bind(&CMA_EE::PSRAH, this);
	m_pOpSpecial2[0x3C] = std::bind(&CMA_EE::PSLLW, this);
	m_pOpSpecial2[0x3E] = std::bind(&CMA_EE::PSRLW, this);
	m_pOpSpecial2[0x3F] = std::bind(&CMA_EE::PSRAW, this);

	SetupReflectionTables();
}

CMA_EE::~CMA_EE()
{

}

void CMA_EE::PushVector(unsigned int nReg)
{
	m_codeGen->MD_PushRel(offsetof(CMIPS, m_State.nGPR[nReg]));
}

void CMA_EE::PullVector(unsigned int nReg)
{
	m_codeGen->MD_PullRel(offsetof(CMIPS, m_State.nGPR[nReg]));
}

size_t CMA_EE::GetLoOffset(unsigned int index)
{
	switch(index)
	{
	case 0:
		return offsetof(CMIPS, m_State.nLO[0]);
		break;
	case 1:
		return offsetof(CMIPS, m_State.nLO[1]);
		break;
	case 2:
		return offsetof(CMIPS, m_State.nLO1[0]);
		break;
	case 3:
		return offsetof(CMIPS, m_State.nLO1[1]);
		break;
	default:
		throw std::exception();
		break;
	}
}

size_t CMA_EE::GetHiOffset(unsigned int index)
{
	switch(index)
	{
	case 0:
		return offsetof(CMIPS, m_State.nHI[0]);
		break;
	case 1:
		return offsetof(CMIPS, m_State.nHI[1]);
		break;
	case 2:
		return offsetof(CMIPS, m_State.nHI1[0]);
		break;
	case 3:
		return offsetof(CMIPS, m_State.nHI1[1]);
		break;
	default:
		throw std::exception();
		break;
	}
}

//////////////////////////////////////////////////
//General Opcodes
//////////////////////////////////////////////////

//1E
void CMA_EE::LQ()
{
	ComputeMemAccessAddr();

	m_codeGen->PushCtx();
	m_codeGen->PushIdx(1);
	m_codeGen->Call(reinterpret_cast<void*>(&MemoryUtils_GetQuadProxy), 2, Jitter::CJitter::RETURN_VALUE_128);
	m_codeGen->MD_PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT]));

	m_codeGen->PullTop();
}

//1F
void CMA_EE::SQ()
{
	ComputeMemAccessAddr();

	m_codeGen->PushCtx();
	m_codeGen->MD_PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT]));
	m_codeGen->PushIdx(2);
	m_codeGen->Call(reinterpret_cast<void*>(&MemoryUtils_SetQuadProxy), 3, Jitter::CJitter::RETURN_VALUE_NONE);

	m_codeGen->PullTop();
}

//////////////////////////////////////////////////
//RegImm Opcodes
//////////////////////////////////////////////////

//18
void CMA_EE::MTSAB()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushCst(0x0F);
	m_codeGen->And();
	m_codeGen->PushCst(m_nImmediate & 0x0F);
	m_codeGen->Xor();
	m_codeGen->Shl(0x03);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nSA));
}

//19
void CMA_EE::MTSAH()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushCst(0x07);
	m_codeGen->And();
	m_codeGen->PushCst(m_nImmediate & 0x07);
	m_codeGen->Xor();
	m_codeGen->Shl(0x04);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nSA));
}

//////////////////////////////////////////////////
//MMI Opcodes
//////////////////////////////////////////////////

//00
void CMA_EE::MADD()
{
	Generic_MADD(0);
}

//04
void CMA_EE::PLZCW()
{
	for(unsigned int i = 0; i < 2; i++)
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[i]));
		m_codeGen->Lzc();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[i]));
	}
}

//08
void CMA_EE::MMI0()
{
	((this)->*(m_pOpMmi0[(m_nOpcode >> 6) & 0x1F]))();
}

//09
void CMA_EE::MMI2()
{
	((this)->*(m_pOpMmi2[(m_nOpcode >> 6) & 0x1F]))();
}

//10
void CMA_EE::MFHI1()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nHI1[0]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nHI1[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
}

//11
void CMA_EE::MTHI1()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nHI1[0]));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nHI1[1]));
}

//12
void CMA_EE::MFLO1()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nLO1[0]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nLO1[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
}

//13
void CMA_EE::MTLO1()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nLO1[0]));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nLO1[1]));
}

//18
void CMA_EE::MULT1()
{
	Template_Mult32(std::bind(&Jitter::CJitter::MultS, m_codeGen), 1);
}

//19
void CMA_EE::MULTU1()
{
	Template_Mult32(std::bind(&Jitter::CJitter::Mult, m_codeGen), 1);
}

//1A
void CMA_EE::DIV1()
{
	Template_Div32(std::bind(&Jitter::CJitter::DivS, m_codeGen), 1);
}

//1B
void CMA_EE::DIVU1()
{
	Template_Div32(std::bind(&Jitter::CJitter::Div, m_codeGen), 1);
}

//20
void CMA_EE::MADD1()
{
	Generic_MADD(1);
}

//28
void CMA_EE::MMI1()
{
	((this)->*(m_pOpMmi1[(m_nOpcode >> 6) & 0x1F]))();
}

//29
void CMA_EE::MMI3()
{
	((this)->*(m_pOpMmi3[(m_nOpcode >> 6) & 0x1F]))();
}

//30
void CMA_EE::PMFHL()
{
	((this)->*(m_pOpPmfhl[(m_nOpcode >> 6) & 0x1F]))();
}

//34
void CMA_EE::PSLLH()
{
	PushVector(m_nRT);
	m_codeGen->MD_SllH(m_nSA);
	PullVector(m_nRD);
}

//36
void CMA_EE::PSRLH()
{
	PushVector(m_nRT);
	m_codeGen->MD_SrlH(m_nSA);
	PullVector(m_nRD);
}

//37
void CMA_EE::PSRAH()
{
	PushVector(m_nRT);
	m_codeGen->MD_SraH(m_nSA);
	PullVector(m_nRD);
}

//3C
void CMA_EE::PSLLW()
{
	PushVector(m_nRT);
	m_codeGen->MD_SllW(m_nSA);
	PullVector(m_nRD);
}

//3E
void CMA_EE::PSRLW()
{
	PushVector(m_nRT);
	m_codeGen->MD_SrlW(m_nSA);
	PullVector(m_nRD);
}

//3F
void CMA_EE::PSRAW()
{
	PushVector(m_nRT);
	m_codeGen->MD_SraW(m_nSA);
	PullVector(m_nRD);
}

//////////////////////////////////////////////////
//MMI0 Opcodes
//////////////////////////////////////////////////

//00
void CMA_EE::PADDW()
{
	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_AddW();
	PullVector(m_nRD);
}

//01
void CMA_EE::PSUBW()
{
	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_SubW();
	PullVector(m_nRD);
}

//03
void CMA_EE::PMAXW()
{
	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_MaxW();
	PullVector(m_nRD);
}

//04
void CMA_EE::PADDH()
{
	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_AddH();
	PullVector(m_nRD);
}

//06
void CMA_EE::PCGTH()
{
	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_CmpGtH();
	PullVector(m_nRD);
}

//07
void CMA_EE::PMAXH()
{
	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_MaxH();
	PullVector(m_nRD);
}

//08
void CMA_EE::PADDB()
{
	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_AddB();
	PullVector(m_nRD);
}

//09
void CMA_EE::PSUBB()
{
	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_SubB();
	PullVector(m_nRD);
}

//10
void CMA_EE::PADDSW()
{
	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_AddWSS();
	PullVector(m_nRD);
}

//12
void CMA_EE::PEXTLW()
{
	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_UnpackLowerWD();
	PullVector(m_nRD);
}

//13
void CMA_EE::PPACW()
{
	//RS = A
	//RT = B
	//RD = A2 A0 B2 B0
	assert(m_nRS != m_nRD);
	assert(m_nRT != m_nRD);

	//0
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));

	//1
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[2]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));

	//2
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[2]));

	//3
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[2]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[3]));
}

//16
void CMA_EE::PEXTLH()
{
	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_UnpackLowerHW();
	PullVector(m_nRD);
}

//17
void CMA_EE::PPACH()
{
	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_PackWH();
	PullVector(m_nRD);
}

//1A
void CMA_EE::PEXTLB()
{
	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_UnpackLowerBH();
	PullVector(m_nRD);
}

//1B
void CMA_EE::PPACB()
{
	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_PackHB();
	PullVector(m_nRD);
}

//1E
void CMA_EE::PEXT5()
{
	for(unsigned int i = 0; i < 4; i++)
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[i]));
		m_codeGen->PushCst(0x001F);
		m_codeGen->And();
		m_codeGen->Shl(3);

		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[i]));
		m_codeGen->PushCst(0x03E0);
		m_codeGen->And();
		m_codeGen->Shl(6);
		m_codeGen->Or();

		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[i]));
		m_codeGen->PushCst(0x7C00);
		m_codeGen->And();
		m_codeGen->Shl(9);
		m_codeGen->Or();

		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[i]));
		m_codeGen->PushCst(0x8000);
		m_codeGen->And();
		m_codeGen->Shl(16);
		m_codeGen->Or();

		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[i]));
	}
}

//////////////////////////////////////////////////
//MMI1 Opcodes
//////////////////////////////////////////////////

//02
void CMA_EE::PCEQW()
{
	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_CmpEqW();
	PullVector(m_nRD);
}

//03
void CMA_EE::PMINW()
{
	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_MinW();
	PullVector(m_nRD);
}

//06
void CMA_EE::PCEQH()
{
	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_CmpEqH();
	PullVector(m_nRD);
}

//07
void CMA_EE::PMINH()
{
	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_MinH();
	PullVector(m_nRD);
}

//0A
void CMA_EE::PCEQB()
{
	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_CmpEqB();
	PullVector(m_nRD);
}

//10
void CMA_EE::PADDUW()
{
	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_AddWUS();
	PullVector(m_nRD);
}

//12
void CMA_EE::PEXTUW()
{
	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_UnpackUpperWD();
	PullVector(m_nRD);
}

//18
void CMA_EE::PADDUB()
{
	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_AddBUS();
	PullVector(m_nRD);
}

//1A
void CMA_EE::PEXTUB()
{
	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_UnpackUpperBH();
	PullVector(m_nRD);
}

//1B
void CMA_EE::QFSRV()
{
	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nSA));
	m_codeGen->MD_Srl256();
	PullVector(m_nRD);
}

//////////////////////////////////////////////////
//MMI2 Opcodes
//////////////////////////////////////////////////

//08
void CMA_EE::PMFHI()
{
	for(unsigned int i = 0; i < 4; i++)
	{
		m_codeGen->PushRel(GetHiOffset(i));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[i]));
	}
}

//09
void CMA_EE::PMFLO()
{
	for(unsigned int i = 0; i < 4; i++)
	{
		m_codeGen->PushRel(GetLoOffset(i));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[i]));
	}
}

//0C
void CMA_EE::PMULTW()
{
	for(unsigned int i = 0; i < 2; i++)
	{
		unsigned int regOffset = i * 2;

		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[regOffset]));
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[regOffset]));
		m_codeGen->MultS();

		m_codeGen->PushTop();

		//LO
		m_codeGen->ExtLow64();
		{
			m_codeGen->PushTop();
			m_codeGen->SignExt();
			m_codeGen->PullRel(GetLoOffset(regOffset + 1));
		}
		m_codeGen->PullRel(GetLoOffset(regOffset + 0));

		//HI
		m_codeGen->ExtHigh64();
		{
			m_codeGen->PushTop();
			m_codeGen->SignExt();
			m_codeGen->PullRel(GetHiOffset(regOffset + 1));
		}
		m_codeGen->PullRel(GetHiOffset(regOffset + 0));
	}

	if(m_nRD != 0)
	{
		for(unsigned int i = 0; i < 2; i++)
		{
			unsigned int regOffset = i * 2;

			m_codeGen->PushRel(GetLoOffset(regOffset));
			m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[regOffset + 0]));

			m_codeGen->PushRel(GetHiOffset(regOffset));
			m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[regOffset + 1]));
		}
	}
}

//0D
void CMA_EE::PDIVW()
{
	for(unsigned int i = 0; i < 2; i++)
	{
		unsigned int regOffset = i * 2;

		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[regOffset]));
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[regOffset]));
		m_codeGen->DivS();

		m_codeGen->PushTop();

		//Quotient
		m_codeGen->ExtLow64();
		{
			m_codeGen->PushTop();
			m_codeGen->SignExt();
			m_codeGen->PullRel(GetLoOffset(regOffset + 1));
		}
		m_codeGen->PullRel(GetLoOffset(regOffset + 0));

		//Remainder
		m_codeGen->ExtHigh64();
		{
			m_codeGen->PushTop();
			m_codeGen->SignExt();
			m_codeGen->PullRel(GetHiOffset(regOffset + 1));
		}
		m_codeGen->PullRel(GetHiOffset(regOffset + 0));
	}
}

//0E
void CMA_EE::PCPYLD()
{
	//A0
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));
	
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[3]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[2]));

	//B0
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
	
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

//12
void CMA_EE::PAND()
{
	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_And();
	PullVector(m_nRD);
}

//13
void CMA_EE::PXOR()
{
	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_Xor();
	PullVector(m_nRD);
}

//1C
void CMA_EE::PMULTH()
{
	{
		//A0xB0
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
		m_codeGen->MultSHL();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nLO[0]));

		//A1xB1
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
		m_codeGen->MultSHH();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nLO[1]));
	}

	{
		//A2xB2
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
		m_codeGen->MultSHL();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nHI[0]));

		//A3xB3
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
		m_codeGen->MultSHH();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nHI[1]));
	}

	{
		//A4xB4
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[2]));
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[2]));
		m_codeGen->MultSHL();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nLO1[0]));

		//A5xB5
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[2]));
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[2]));
		m_codeGen->MultSHH();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nLO1[1]));
	}

	{
		//A6xB6
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[3]));
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[3]));
		m_codeGen->MultSHL();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nHI1[0]));

		//A7xB7
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[3]));
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[3]));
		m_codeGen->MultSHH();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nHI1[1]));
	}

	if(m_nRD != 0)
	{
		//Copy to RD
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nLO[0]));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));

		m_codeGen->PushRel(offsetof(CMIPS, m_State.nHI[0]));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));

		m_codeGen->PushRel(offsetof(CMIPS, m_State.nLO1[0]));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[2]));

		m_codeGen->PushRel(offsetof(CMIPS, m_State.nHI1[0]));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[3]));
	}
}

//1E
void CMA_EE::PEXEW()
{
	size_t offset[4];

	if(m_nRT == m_nRD)
	{
		offset[0] = offsetof(CMIPS, m_State.nGPR[m_nRT].nV[2]);
		offset[1] = offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]);
		offset[2] = offsetof(CMIPS, m_State.nCOP2T);
		offset[3] = offsetof(CMIPS, m_State.nGPR[m_nRT].nV[3]);

		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
		m_codeGen->PullRel(offset[2]);
	}
	else
	{
		offset[0] = offsetof(CMIPS, m_State.nGPR[m_nRT].nV[2]);
		offset[1] = offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]);
		offset[2] = offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]);
		offset[3] = offsetof(CMIPS, m_State.nGPR[m_nRT].nV[3]);
	}

	for(unsigned int i = 0; i < 4; i++)
	{
		m_codeGen->PushRel(offset[i]);
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[i]));
	}
}

//1F
void CMA_EE::PROT3W()
{
	size_t offset[4];

	if(m_nRT == m_nRD)
	{
		offset[0] = offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]);
		offset[1] = offsetof(CMIPS, m_State.nGPR[m_nRT].nV[2]);
		offset[2] = offsetof(CMIPS, m_State.nCOP2T);
		offset[3] = offsetof(CMIPS, m_State.nGPR[m_nRT].nV[3]);

		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
		m_codeGen->PullRel(offset[2]);
	}
	else
	{
		offset[0] = offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]);
		offset[1] = offsetof(CMIPS, m_State.nGPR[m_nRT].nV[2]);
		offset[2] = offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]);
		offset[3] = offsetof(CMIPS, m_State.nGPR[m_nRT].nV[3]);
	}

	for(unsigned int i = 0; i < 4; i++)
	{
		m_codeGen->PushRel(offset[i]);
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[i]));
	}
}

//////////////////////////////////////////////////
//MMI3 Opcodes
//////////////////////////////////////////////////

//0E
void CMA_EE::PCPYUD()
{
	//A
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[2]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[3]));
	
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));

	//B
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[2]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[3]));
	
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[3]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[2]));	
}

//12
void CMA_EE::POR()
{
	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_Or();
	PullVector(m_nRD);
}

//13
void CMA_EE::PNOR()
{
	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_Or();
	m_codeGen->MD_Not();
	PullVector(m_nRD);
}

//1B
void CMA_EE::PCPYH()
{
	for(unsigned int i = 0; i < 4; i += 2)
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[i]));
		m_codeGen->PushCst(0xFFFF);
		m_codeGen->And();
		m_codeGen->PushTop();
		m_codeGen->Shl(16);
		m_codeGen->Or();
		m_codeGen->PushTop();

		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[i + 0]));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[i + 1]));
	}
}

//1E
void CMA_EE::PEXCW()
{
	size_t offset[4];

	if(m_nRT == m_nRD)
	{
		offset[0] = offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]);
		offset[1] = offsetof(CMIPS, m_State.nGPR[m_nRT].nV[2]);
		offset[2] = offsetof(CMIPS, m_State.nCOP2T);
		offset[3] = offsetof(CMIPS, m_State.nGPR[m_nRT].nV[3]);

		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
		m_codeGen->PullRel(offset[2]);
	}
	else
	{
		offset[0] = offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]);
		offset[1] = offsetof(CMIPS, m_State.nGPR[m_nRT].nV[2]);
		offset[2] = offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]);
		offset[3] = offsetof(CMIPS, m_State.nGPR[m_nRT].nV[3]);
	}

	for(unsigned int i = 0; i < 4; i++)
	{
		m_codeGen->PushRel(offset[i]);
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[i]));
	}
}

//////////////////////////////////////////////////
//PMFHL
//////////////////////////////////////////////////

void CMA_EE::PMFHL_LW()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nLO[0]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nHI[0]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nLO1[0]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[2]));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nHI1[0]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[3]));
}

//////////////////////////////////////////////////
//Generic Stuff
//////////////////////////////////////////////////

void CMA_EE::Generic_MADD(unsigned int unit)
{
	//prod = (HI || LO) + (RS * RT)
	//LO = sex(prod[0])
	//HI = sex(prod[1])
	//RD = LO

	size_t lo[2];
	size_t hi[2];

	switch(unit)
	{
	case 0:
		lo[0] = offsetof(CMIPS, m_State.nLO[0]);
		lo[1] = offsetof(CMIPS, m_State.nLO[1]);
		hi[0] = offsetof(CMIPS, m_State.nHI[0]);
		hi[1] = offsetof(CMIPS, m_State.nHI[1]);
		break;
	case 1:
		lo[0] = offsetof(CMIPS, m_State.nLO1[0]);
		lo[1] = offsetof(CMIPS, m_State.nLO1[1]);
		hi[0] = offsetof(CMIPS, m_State.nHI1[0]);
		hi[1] = offsetof(CMIPS, m_State.nHI1[1]);
		break;
	default:
		throw std::runtime_error("Invalid unit number.");
		break;
	}

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->MultS();

	m_codeGen->PushRel(lo[0]);
	m_codeGen->PushRel(hi[0]);
	m_codeGen->MergeTo64();

	m_codeGen->Add64();

	m_codeGen->PushTop();
	m_codeGen->ExtHigh64();
	m_codeGen->PushTop();
	m_codeGen->SignExt();
	m_codeGen->PullRel(hi[1]);
	m_codeGen->PullRel(hi[0]);

	m_codeGen->ExtLow64();
	m_codeGen->PushTop();
	m_codeGen->SignExt();
	m_codeGen->PullRel(lo[1]);
	m_codeGen->PullRel(lo[0]);

	if(m_nRD != 0)
	{
		m_codeGen->PushRel(lo[0]);
		m_codeGen->PushRel(lo[1]);

		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
	}
}

//////////////////////////////////////////////////
//Opcode Tables
//////////////////////////////////////////////////

CMA_EE::InstructionFuncConstant CMA_EE::m_pOpMmi0[0x20] = 
{
	//0x00
	&CMA_EE::PADDW,			&CMA_EE::PSUBW,			&CMA_EE::Illegal,		&CMA_EE::PMAXW,			&CMA_EE::PADDH,			&CMA_EE::Illegal,		&CMA_EE::PCGTH,			&CMA_EE::PMAXH,
	//0x08
	&CMA_EE::PADDB,			&CMA_EE::PSUBB,			&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,
	//0x10
	&CMA_EE::PADDSW,		&CMA_EE::Illegal,		&CMA_EE::PEXTLW,		&CMA_EE::PPACW,			&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::PEXTLH,		&CMA_EE::PPACH,
	//0x18
	&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::PEXTLB,		&CMA_EE::PPACB,			&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::PEXT5,			&CMA_EE::Illegal,
};

CMA_EE::InstructionFuncConstant CMA_EE::m_pOpMmi1[0x20] = 
{
	//0x00
	&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::PCEQW,			&CMA_EE::PMINW,			&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::PCEQH,			&CMA_EE::PMINH,
	//0x08
	&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::PCEQB,			&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,
	//0x10
	&CMA_EE::PADDUW,		&CMA_EE::Illegal,		&CMA_EE::PEXTUW,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,
	//0x18
	&CMA_EE::PADDUB,		&CMA_EE::Illegal,		&CMA_EE::PEXTUB,		&CMA_EE::QFSRV,			&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,
};

CMA_EE::InstructionFuncConstant CMA_EE::m_pOpMmi2[0x20] = 
{
	//0x00
	&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,
	//0x08
	&CMA_EE::PMFHI,			&CMA_EE::PMFLO,			&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::PMULTW,		&CMA_EE::PDIVW,			&CMA_EE::PCPYLD,		&CMA_EE::Illegal,
	//0x10
	&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::PAND,			&CMA_EE::PXOR,			&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,
	//0x18
	&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::PMULTH,		&CMA_EE::Illegal,		&CMA_EE::PEXEW,			&CMA_EE::PROT3W,
};

CMA_EE::InstructionFuncConstant CMA_EE::m_pOpMmi3[0x20] = 
{
	//0x00
	&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,
	//0x08
	&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::PCPYUD,		&CMA_EE::Illegal,
	//0x10
	&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::POR,			&CMA_EE::PNOR,			&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,
	//0x18
	&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::PCPYH,			&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::PEXCW,			&CMA_EE::Illegal,
};

CMA_EE::InstructionFuncConstant CMA_EE::m_pOpPmfhl[0x20] = 
{
	//0x00
	&CMA_EE::PMFHL_LW,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,
	//0x08
	&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,
	//0x10
	&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,
	//0x18
	&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,
};
