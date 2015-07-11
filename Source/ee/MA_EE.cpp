#include <stddef.h>
#include "../MIPS.h"
#include "../MemoryUtils.h"
#include "MA_EE.h"
#include "offsetof_def.h"

CMA_EE::CMA_EE() :
CMA_MIPSIV(MIPS_REGSIZE_64)
{
	m_pOpGeneral[0x1E] = std::bind(&CMA_EE::LQ, this);
	m_pOpGeneral[0x1F] = std::bind(&CMA_EE::SQ, this);

	m_pOpRegImm[0x18] = std::bind(&CMA_EE::MTSAB, this);
	m_pOpRegImm[0x19] = std::bind(&CMA_EE::MTSAH, this);

	m_pOpSpecial[0x28] = std::bind(&CMA_EE::MFSA, this);
	m_pOpSpecial[0x29] = std::bind(&CMA_EE::MTSA, this);

	m_pOpSpecial2[0x00] = std::bind(&CMA_EE::MADD, this);
	m_pOpSpecial2[0x01] = std::bind(&CMA_EE::MADDU, this);
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
	m_pOpSpecial2[0x21] = std::bind(&CMA_EE::MADDU1, this);
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
	if(m_nRT == 0) return;

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
//Special Opcodes
//////////////////////////////////////////////////

//28
void CMA_EE::MFSA()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nSA));
	m_codeGen->Srl(3);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

//29
void CMA_EE::MTSA()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushCst(0xF);
	m_codeGen->And();
	m_codeGen->Shl(3);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nSA));
}

//////////////////////////////////////////////////
//MMI Opcodes
//////////////////////////////////////////////////

//00
void CMA_EE::MADD()
{
	Generic_MADD(0, true);
}

//01
void CMA_EE::MADDU()
{
	Generic_MADD(0, false);
}

//04
void CMA_EE::PLZCW()
{
	if(m_nRD == 0) return;

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
	if(m_nRD == 0) return;

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
	if(m_nRD == 0) return;

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
	Template_Mult32(true, 1);
}

//19
void CMA_EE::MULTU1()
{
	Template_Mult32(false, 1);
}

//1A
void CMA_EE::DIV1()
{
	Template_Div32(true, 1);
}

//1B
void CMA_EE::DIVU1()
{
	Template_Div32(false, 1);
}

//20
void CMA_EE::MADD1()
{
	Generic_MADD(1, true);
}

//21
void CMA_EE::MADDU1()
{
	Generic_MADD(1, false);
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
	if(m_nRD == 0) return;

	PushVector(m_nRT);
	m_codeGen->MD_SllH(m_nSA);
	PullVector(m_nRD);
}

//36
void CMA_EE::PSRLH()
{
	if(m_nRD == 0) return;

	PushVector(m_nRT);
	m_codeGen->MD_SrlH(m_nSA);
	PullVector(m_nRD);
}

//37
void CMA_EE::PSRAH()
{
	if(m_nRD == 0) return;

	PushVector(m_nRT);
	m_codeGen->MD_SraH(m_nSA);
	PullVector(m_nRD);
}

//3C
void CMA_EE::PSLLW()
{
	if(m_nRD == 0) return;

	PushVector(m_nRT);
	m_codeGen->MD_SllW(m_nSA);
	PullVector(m_nRD);
}

//3E
void CMA_EE::PSRLW()
{
	if(m_nRD == 0) return;

	PushVector(m_nRT);
	m_codeGen->MD_SrlW(m_nSA);
	PullVector(m_nRD);
}

//3F
void CMA_EE::PSRAW()
{
	if(m_nRD == 0) return;

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
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_AddW();
	PullVector(m_nRD);
}

//01
void CMA_EE::PSUBW()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_SubW();
	PullVector(m_nRD);
}

//02
void CMA_EE::PCGTW()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_CmpGtW();
	PullVector(m_nRD);
}

//03
void CMA_EE::PMAXW()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_MaxW();
	PullVector(m_nRD);
}

//04
void CMA_EE::PADDH()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_AddH();
	PullVector(m_nRD);
}

//05
void CMA_EE::PSUBH()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_SubH();
	PullVector(m_nRD);
}

//06
void CMA_EE::PCGTH()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_CmpGtH();
	PullVector(m_nRD);
}

//07
void CMA_EE::PMAXH()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_MaxH();
	PullVector(m_nRD);
}

//08
void CMA_EE::PADDB()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_AddB();
	PullVector(m_nRD);
}

//09
void CMA_EE::PSUBB()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_SubB();
	PullVector(m_nRD);
}

//0A
void CMA_EE::PCGTB()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_CmpGtB();
	PullVector(m_nRD);
}

//10
void CMA_EE::PADDSW()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_AddWSS();
	PullVector(m_nRD);
}

//11
void CMA_EE::PSUBSW()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_SubWSS();
	PullVector(m_nRD);
}

//12
void CMA_EE::PEXTLW()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_UnpackLowerWD();
	PullVector(m_nRD);
}

//13
void CMA_EE::PPACW()
{
	if(m_nRD == 0) return;

	//RS = A
	//RT = B
	//RD = A2 A0 B2 B0

	//The order of operations here is quite important because RS, RT and RD can be equal
	//- Elements 1 and 3 in the destination register are unused in the source registers so, it's safe to write them first
	//- Element 2 in the destination register isn't used after the previous operations, so it's safe to override
	//- Only element 0 remains which is filled with element 0 from source, which is safe even if registers are the same

	//1
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[2]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));

	//3
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[2]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[3]));

	//2
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[2]));

	//0
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

//14
void CMA_EE::PADDSH()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_AddHSS();
	PullVector(m_nRD);
}

//15
void CMA_EE::PSUBSH()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_SubHSS();
	PullVector(m_nRD);
}

//16
void CMA_EE::PEXTLH()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_UnpackLowerHW();
	PullVector(m_nRD);
}

//17
void CMA_EE::PPACH()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_PackWH();
	PullVector(m_nRD);
}

//1A
void CMA_EE::PEXTLB()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_UnpackLowerBH();
	PullVector(m_nRD);
}

//1B
void CMA_EE::PPACB()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_PackHB();
	PullVector(m_nRD);
}

//1E
void CMA_EE::PEXT5()
{
	if(m_nRD == 0) return;

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

//01
void CMA_EE::PABSW()
{
	if(m_nRD == 0) return;

	//RD = (RT != 0x80000000) ? |RT| : 0x7FFFFFFF;

	for(unsigned int i = 0; i < 4; i++)
	{
		auto srcOffset = offsetof(CMIPS, m_State.nGPR[m_nRT].nV[i]);
		auto dstOffset = offsetof(CMIPS, m_State.nGPR[m_nRD].nV[i]);

		m_codeGen->PushRel(srcOffset);
		m_codeGen->PushCst(0x80000000);

		m_codeGen->BeginIf(Jitter::CONDITION_EQ);
		{
			m_codeGen->PushCst(0x7FFFFFFF);
			m_codeGen->PullRel(dstOffset);
		}
		m_codeGen->Else();
		{
			m_codeGen->PushRel(srcOffset);
			m_codeGen->PushCst(0);

			m_codeGen->BeginIf(Jitter::CONDITION_LT);
			{
				m_codeGen->PushCst(0);
				m_codeGen->PushRel(srcOffset);
				m_codeGen->Sub();
				m_codeGen->PullRel(dstOffset);
			}
			m_codeGen->Else();
			{
				m_codeGen->PushRel(srcOffset);
				m_codeGen->PullRel(dstOffset);
			}
			m_codeGen->EndIf();
		}
		m_codeGen->EndIf();
	}
}

//02
void CMA_EE::PCEQW()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_CmpEqW();
	PullVector(m_nRD);
}

//03
void CMA_EE::PMINW()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_MinW();
	PullVector(m_nRD);
}

//06
void CMA_EE::PCEQH()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_CmpEqH();
	PullVector(m_nRD);
}

//07
void CMA_EE::PMINH()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_MinH();
	PullVector(m_nRD);
}

//0A
void CMA_EE::PCEQB()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_CmpEqB();
	PullVector(m_nRD);
}

//10
void CMA_EE::PADDUW()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_AddWUS();
	PullVector(m_nRD);
}

//12
void CMA_EE::PEXTUW()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_UnpackUpperWD();
	PullVector(m_nRD);
}

//15
void CMA_EE::PSUBUH()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_SubHUS();
	PullVector(m_nRD);
}

//16
void CMA_EE::PEXTUH()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_UnpackUpperHW();
	PullVector(m_nRD);
}

//18
void CMA_EE::PADDUB()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_AddBUS();
	PullVector(m_nRD);
}

//19
void CMA_EE::PSUBUB()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_SubBUS();
	PullVector(m_nRD);
}

//1A
void CMA_EE::PEXTUB()
{
	if(m_nRD == 0) return;

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

//02
void CMA_EE::PSLLVW()
{
	Generic_PSxxV([this] () { m_codeGen->Shl(); });
}

//03
void CMA_EE::PSRLVW()
{
	Generic_PSxxV([this] () { m_codeGen->Srl(); });
}

//08
void CMA_EE::PMFHI()
{
	if(m_nRD == 0) return;

	for(unsigned int i = 0; i < 4; i++)
	{
		m_codeGen->PushRel(GetHiOffset(i));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[i]));
	}
}

//09
void CMA_EE::PMFLO()
{
	if(m_nRD == 0) return;

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
		Template_Div32(true, i, i * 2);
	}
}

//0E
void CMA_EE::PCPYLD()
{
	if(m_nRD == 0) return;

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

//10
void CMA_EE::PMADDH()
{
	static const size_t offsets[8] =
	{
		offsetof(CMIPS, m_State.nLO[0]),
		offsetof(CMIPS, m_State.nLO[1]),
		offsetof(CMIPS, m_State.nHI[0]),
		offsetof(CMIPS, m_State.nHI[1]),
		offsetof(CMIPS, m_State.nLO1[0]),
		offsetof(CMIPS, m_State.nLO1[1]),
		offsetof(CMIPS, m_State.nHI1[0]),
		offsetof(CMIPS, m_State.nHI1[1])
	};

	for(unsigned int i = 0; i < 4; i++)
	{
		//Lower 16-bits
		{
			m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[i]));
			m_codeGen->SignExt16();

			m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[i]));
			m_codeGen->SignExt16();

			m_codeGen->MultS();
			m_codeGen->ExtLow64();

			m_codeGen->PushRel(offsets[(i * 2) + 0]);
			m_codeGen->Add();
			m_codeGen->PullRel(offsets[(i * 2) + 0]);
		}

		//Higher 16-bits
		{
			m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[i]));
			m_codeGen->Sra(16);

			m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[i]));
			m_codeGen->Sra(16);

			m_codeGen->MultS();
			m_codeGen->ExtLow64();

			m_codeGen->PushRel(offsets[(i * 2) + 1]);
			m_codeGen->Add();
			m_codeGen->PullRel(offsets[(i * 2) + 1]);
		}
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

//12
void CMA_EE::PAND()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_And();
	PullVector(m_nRD);
}

//13
void CMA_EE::PXOR()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_Xor();
	PullVector(m_nRD);
}

//1B
void CMA_EE::PREVH()
{
	if(m_nRD == 0) return;

	for(unsigned int i = 0; i < 2; i++)
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[(i * 2) + 0]));
		m_codeGen->Shl(16);
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[(i * 2) + 0]));
		m_codeGen->Srl(16);
		m_codeGen->Or();

		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[(i * 2) + 1]));
		m_codeGen->Shl(16);
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[(i * 2) + 1]));
		m_codeGen->Srl(16);
		m_codeGen->Or();

		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[(i * 2) + 0]));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[(i * 2) + 1]));
	}
}

//1C
void CMA_EE::PMULTH()
{
	static const size_t offsets[8] =
	{
		offsetof(CMIPS, m_State.nLO[0]),
		offsetof(CMIPS, m_State.nLO[1]),
		offsetof(CMIPS, m_State.nHI[0]),
		offsetof(CMIPS, m_State.nHI[1]),
		offsetof(CMIPS, m_State.nLO1[0]),
		offsetof(CMIPS, m_State.nLO1[1]),
		offsetof(CMIPS, m_State.nHI1[0]),
		offsetof(CMIPS, m_State.nHI1[1])
	};

	for(unsigned int i = 0; i < 4; i++)
	{
		//Lower 16-bits
		{
			m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[i]));
			m_codeGen->SignExt16();

			m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[i]));
			m_codeGen->SignExt16();

			m_codeGen->MultS();
			m_codeGen->ExtLow64();

			m_codeGen->PullRel(offsets[(i * 2) + 0]);
		}

		//Higher 16-bits
		{
			m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[i]));
			m_codeGen->Sra(16);

			m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[i]));
			m_codeGen->Sra(16);

			m_codeGen->MultS();
			m_codeGen->ExtLow64();

			m_codeGen->PullRel(offsets[(i * 2) + 1]);
		}
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
	if(m_nRD == 0) return;

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
	if(m_nRD == 0) return;

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

//03
void CMA_EE::PSRAVW()
{
	Generic_PSxxV([this] () { m_codeGen->Sra(); });
}

//08
void CMA_EE::PMTHI()
{
	for(unsigned int i = 0; i < 4; i++)
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[i]));
		m_codeGen->PullRel(GetHiOffset(i));
	}
}

//09
void CMA_EE::PMTLO()
{
	for(unsigned int i = 0; i < 4; i++)
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[i]));
		m_codeGen->PullRel(GetLoOffset(i));
	}
}

//0A
void CMA_EE::PINTEH()
{
	if(m_nRD == 0) return;

	for(unsigned int i = 0; i < 4; i++)
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[i]));
		m_codeGen->Shl(16);
		m_codeGen->PushCst(0xFFFF0000);
		m_codeGen->And();

		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[i]));
		m_codeGen->PushCst(0xFFFF);
		m_codeGen->And();

		m_codeGen->Or();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[i]));
	}
}

//0E
void CMA_EE::PCPYUD()
{
	if(m_nRD == 0) return;

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
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_Or();
	PullVector(m_nRD);
}

//13
void CMA_EE::PNOR()
{
	if(m_nRD == 0) return;

	PushVector(m_nRS);
	PushVector(m_nRT);
	m_codeGen->MD_Or();
	m_codeGen->MD_Not();
	PullVector(m_nRD);
}

//1A
void CMA_EE::PEXCH()
{
	if(m_nRD == 0) return;

	for(unsigned int i = 0; i < 4; i += 2)
	{
		//Compute first value
		//t0 = rt[0] >> 16;
		//t1 = (rt[1] & 0xFFFF0000)
		//t2 = t0 | t1
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[i + 0]));
		m_codeGen->Srl(16);
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[i + 1]));
		m_codeGen->PushCst(0xFFFF0000);
		m_codeGen->And();
		m_codeGen->Or();

		//Compute second value
		//t0 = rt[1] << 16;
		//t1 = (r1[0] & 0x0000FFFF)
		//t2 = t0 | t1
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[i + 1]));
		m_codeGen->Shl(16);
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[i + 0]));
		m_codeGen->PushCst(0x0000FFFF);
		m_codeGen->And();
		m_codeGen->Or();

		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[i + 0]));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[i + 1]));
	}
}

//1B
void CMA_EE::PCPYH()
{
	if(m_nRD == 0) return;

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
	if(m_nRD == 0) return;

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

//00
void CMA_EE::PMFHL_LW()
{
	if(m_nRD == 0) return;

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nLO[0]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nHI[0]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nLO1[0]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[2]));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nHI1[0]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[3]));
}

//01
void CMA_EE::PMFHL_UW()
{
	if(m_nRD == 0) return;

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nLO[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nHI[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nLO1[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[2]));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nHI1[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[3]));
}

//03
void CMA_EE::PMFHL_LH()
{
	if(m_nRD == 0) return;

	static const size_t offsets[8] =
	{
		offsetof(CMIPS, m_State.nLO[0]),
		offsetof(CMIPS, m_State.nLO[1]),
		offsetof(CMIPS, m_State.nHI[0]),
		offsetof(CMIPS, m_State.nHI[1]),
		offsetof(CMIPS, m_State.nLO1[0]),
		offsetof(CMIPS, m_State.nLO1[1]),
		offsetof(CMIPS, m_State.nHI1[0]),
		offsetof(CMIPS, m_State.nHI1[1])
	};

	for(unsigned int i = 0; i < 4; i++)
	{
		m_codeGen->PushRel(offsets[(i * 2) + 0]);
		m_codeGen->PushCst(0xFFFF);
		m_codeGen->And();
	
		m_codeGen->PushRel(offsets[(i * 2) + 1]);
		m_codeGen->Shl(16);

		m_codeGen->Or();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[i]));
	}
}

//04
void CMA_EE::PMFHL_SH()
{
	if(m_nRD == 0) return;

	static const auto emitClamp =
		[] (CMipsJitter* codeGen, size_t offsetToClamp)
		{
			codeGen->PushRel(offsetToClamp);
			codeGen->PushCst(0x7FFF);
			codeGen->BeginIf(Jitter::CONDITION_GT);
			{
				codeGen->PushCst(0x7FFF);
				codeGen->PullRel(offsetToClamp);
			}
			codeGen->EndIf();

			codeGen->PushRel(offsetToClamp);
			codeGen->PushCst(0xFFFF8000);
			codeGen->BeginIf(Jitter::CONDITION_LT);
			{
				codeGen->PushCst(0x8000);
				codeGen->PullRel(offsetToClamp);
			}
			codeGen->EndIf();
		};

	static const size_t offsets[8] =
	{
		offsetof(CMIPS, m_State.nLO[0]),
		offsetof(CMIPS, m_State.nLO[1]),
		offsetof(CMIPS, m_State.nHI[0]),
		offsetof(CMIPS, m_State.nHI[1]),
		offsetof(CMIPS, m_State.nLO1[0]),
		offsetof(CMIPS, m_State.nLO1[1]),
		offsetof(CMIPS, m_State.nHI1[0]),
		offsetof(CMIPS, m_State.nHI1[1])
	};

	static const size_t tempOffset = offsetof(CMIPS, m_State.nCOP2T);

	for(unsigned int i = 0; i < 4; i++)
	{
		size_t dstOffset = offsetof(CMIPS, m_State.nGPR[m_nRD].nV[i]);

		//Lo part
		{
			m_codeGen->PushRel(offsets[(i * 2) + 0]);
			m_codeGen->PullRel(tempOffset);

			emitClamp(m_codeGen, tempOffset);

			m_codeGen->PushRel(tempOffset);
			m_codeGen->PullRel(dstOffset);
		}

		//Hi part
		{
			m_codeGen->PushRel(offsets[(i * 2) + 1]);
			m_codeGen->PullRel(tempOffset);
			emitClamp(m_codeGen, tempOffset);
		}

		//Merge
		{
			m_codeGen->PushRel(dstOffset);
			m_codeGen->PushRel(tempOffset);
			m_codeGen->Shl(16);

			m_codeGen->Or();
			m_codeGen->PullRel(dstOffset);
		}
	}
}

//////////////////////////////////////////////////
//Generic Stuff
//////////////////////////////////////////////////

void CMA_EE::Generic_MADD(unsigned int unit, bool isSigned)
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
	if(isSigned)
	{
		m_codeGen->MultS();
	}
	else
	{
		m_codeGen->Mult();
	}

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

void CMA_EE::Generic_PSxxV(const TemplateOperationFunctionType& function)
{
	if(m_nRD == 0) return;

	for(unsigned int i = 0; i < 2; i++)
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[i * 2]));
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[i * 2]));
		function();

		m_codeGen->PushTop();
		m_codeGen->SignExt();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[(i * 2) + 1]));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[(i * 2) + 0]));
	}
}

//////////////////////////////////////////////////
//Opcode Tables
//////////////////////////////////////////////////

CMA_EE::InstructionFuncConstant CMA_EE::m_pOpMmi0[0x20] = 
{
	//0x00
	&CMA_EE::PADDW,			&CMA_EE::PSUBW,			&CMA_EE::PCGTW,			&CMA_EE::PMAXW,			&CMA_EE::PADDH,			&CMA_EE::PSUBH,			&CMA_EE::PCGTH,			&CMA_EE::PMAXH,
	//0x08
	&CMA_EE::PADDB,			&CMA_EE::PSUBB,			&CMA_EE::PCGTB,			&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,
	//0x10
	&CMA_EE::PADDSW,		&CMA_EE::PSUBSW,		&CMA_EE::PEXTLW,		&CMA_EE::PPACW,			&CMA_EE::PADDSH,		&CMA_EE::PSUBSH,		&CMA_EE::PEXTLH,		&CMA_EE::PPACH,
	//0x18
	&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::PEXTLB,		&CMA_EE::PPACB,			&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::PEXT5,			&CMA_EE::Illegal,
};

CMA_EE::InstructionFuncConstant CMA_EE::m_pOpMmi1[0x20] = 
{
	//0x00
	&CMA_EE::Illegal,		&CMA_EE::PABSW,			&CMA_EE::PCEQW,			&CMA_EE::PMINW,			&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::PCEQH,			&CMA_EE::PMINH,
	//0x08
	&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::PCEQB,			&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,
	//0x10
	&CMA_EE::PADDUW,		&CMA_EE::Illegal,		&CMA_EE::PEXTUW,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::PSUBUH,		&CMA_EE::PEXTUH,		&CMA_EE::Illegal,
	//0x18
	&CMA_EE::PADDUB,		&CMA_EE::PSUBUB,		&CMA_EE::PEXTUB,		&CMA_EE::QFSRV,			&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,
};

CMA_EE::InstructionFuncConstant CMA_EE::m_pOpMmi2[0x20] = 
{
	//0x00
	&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::PSLLVW,		&CMA_EE::PSRLVW,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,
	//0x08
	&CMA_EE::PMFHI,			&CMA_EE::PMFLO,			&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::PMULTW,		&CMA_EE::PDIVW,			&CMA_EE::PCPYLD,		&CMA_EE::Illegal,
	//0x10
	&CMA_EE::PMADDH,		&CMA_EE::Illegal,		&CMA_EE::PAND,			&CMA_EE::PXOR,			&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,
	//0x18
	&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::PREVH,			&CMA_EE::PMULTH,		&CMA_EE::Illegal,		&CMA_EE::PEXEW,			&CMA_EE::PROT3W,
};

CMA_EE::InstructionFuncConstant CMA_EE::m_pOpMmi3[0x20] = 
{
	//0x00
	&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::PSRAVW,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,
	//0x08
	&CMA_EE::PMTHI,			&CMA_EE::PMTLO,			&CMA_EE::PINTEH,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::PCPYUD,		&CMA_EE::Illegal,
	//0x10
	&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::POR,			&CMA_EE::PNOR,			&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,
	//0x18
	&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::PEXCH,			&CMA_EE::PCPYH,			&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::PEXCW,			&CMA_EE::Illegal,
};

CMA_EE::InstructionFuncConstant CMA_EE::m_pOpPmfhl[0x20] = 
{
	//0x00
	&CMA_EE::PMFHL_LW,		&CMA_EE::PMFHL_UW,		&CMA_EE::Illegal,		&CMA_EE::PMFHL_LH,		&CMA_EE::PMFHL_SH,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,
	//0x08
	&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,
	//0x10
	&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,
	//0x18
	&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,		&CMA_EE::Illegal,
};
