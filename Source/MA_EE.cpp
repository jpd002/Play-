#include <stddef.h>
#include "MA_EE.h"
#include "CodeGen_VUI128.h"
#include "MIPS.h"
#include "MipsCodeGen.h"
#include "PS2OS.h"
#include "offsetof_def.h"

using namespace CodeGen;
using namespace std::tr1;

//CCacheBlock::CVUI128::PushImm(0xA3A2A1A0, 0xA7A6A5A4, 0xABAAA9A8, 0xAFAEADAC);
//CCacheBlock::CVUI128::PushImm(0xB3B2B1B0, 0xB7B6B5B4, 0xBBBAB9B8, 0xBFBEBDBC);

CMA_EE g_MAEE;

CMA_EE::CMA_EE() :
CMA_MIPSIV(MIPS_REGSIZE_64)
{
	m_pOpGeneral[0x1E] = LQ;
	m_pOpGeneral[0x1F] = SQ;

	//OS special instructions
	m_pOpSpecial[0x1D] = REEXCPT;

	m_pOpRegImm[0x18] = MTSAB;
	m_pOpRegImm[0x19] = MTSAH;

	m_pOpSpecial2[0x00] = MADD;
	m_pOpSpecial2[0x04] = PLZCW;
	m_pOpSpecial2[0x08] = MMI0;
	m_pOpSpecial2[0x09] = MMI2;
	m_pOpSpecial2[0x10] = MFHI1;
	m_pOpSpecial2[0x11] = MTHI1;
	m_pOpSpecial2[0x12] = MFLO1;
	m_pOpSpecial2[0x13] = MTLO1;
	m_pOpSpecial2[0x18] = MULT1;
	m_pOpSpecial2[0x19] = MULTU1;
	m_pOpSpecial2[0x1A] = DIV1;
	m_pOpSpecial2[0x1B] = DIVU1;
	m_pOpSpecial2[0x28] = MMI1;
	m_pOpSpecial2[0x29] = MMI3;
	m_pOpSpecial2[0x34] = PSLLH;
	m_pOpSpecial2[0x36] = PSRLH;
	m_pOpSpecial2[0x37] = PSRAH;

	SetupReflectionTables();
}

CMA_EE::~CMA_EE()
{

}

void CMA_EE::PushVector(unsigned int nReg)
{
	CVUI128::Push(&m_pCtx->m_State.nGPR[nReg]);
}

void CMA_EE::PullVector(unsigned int nReg)
{
	CVUI128::Pull(&m_pCtx->m_State.nGPR[nReg]);
}

//////////////////////////////////////////////////
//General Opcodes
//////////////////////////////////////////////////

//1E
void CMA_EE::LQ()
{
    ComputeMemAccessAddrEx();

    for(unsigned int i = 0; i < 4; i++)
    {
	    m_codeGen->PushRef(m_pCtx);
	    m_codeGen->PushIdx(1);
	    m_codeGen->Call(reinterpret_cast<void*>(&CCacheBlock::GetWordProxy), 2, true);

        m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[i]));

        if(i != 3)
        {
            m_codeGen->PushCst(4);
            m_codeGen->Add();
        }
    }

    m_codeGen->PullTop();
}

//1F
void CMA_EE::SQ()
{
	ComputeMemAccessAddrEx();

	for(unsigned int i = 0; i < 4; i++)
	{
		m_codeGen->PushRef(m_pCtx);
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[i]));
		m_codeGen->PushIdx(2);
		m_codeGen->Call(reinterpret_cast<void*>(&CCacheBlock::SetWordProxy), 3, false);

		if(i != 3)
		{
			m_codeGen->PushCst(4);
			m_codeGen->Add();
		}
	}

	m_codeGen->PullTop();
}

//////////////////////////////////////////////////
//RegImm Opcodes
//////////////////////////////////////////////////

//18
void CMA_EE::MTSAB()
{
	m_pB->PushImm(m_nImmediate & 0x0F);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);

	m_pB->AndImm(0x0F);
	m_pB->Xor();
	m_pB->SllImm(0x03);

	m_pB->PullAddr(&m_pCtx->m_State.nSA);
}

//19
void CMA_EE::MTSAH()
{
	m_pB->PushImm(m_nImmediate & 0x07);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);

	m_pB->AndImm(0x07);
	m_pB->Xor();
	m_pB->SllImm(0x04);

	m_pB->PullAddr(&m_pCtx->m_State.nSA);
}

//////////////////////////////////////////////////
//Special Opcodes
//////////////////////////////////////////////////

void CMA_EE::REEXCPT()
{
//	m_pB->Call(CPS2OS::ExceptionReentry, 0, false);
}

//////////////////////////////////////////////////
//MMI Opcodes
//////////////////////////////////////////////////

//00
void CMA_EE::MADD()
{
	//Multiply
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->MultS();

	//Add LO
	m_pB->PushAddr(&m_pCtx->m_State.nLO[0]);
	m_pB->AddC();

	//Save LO/RD
	m_pB->SeX32();
	m_pB->PullAddr(&m_pCtx->m_State.nLO[1]);

	if(m_nRD != 0)
	{
		m_pB->PushTop();
		SignExtendTop32(m_nRD);
		m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[0]);
	}

	m_pB->PullAddr(&m_pCtx->m_State.nLO[0]);

	//Add HI
	m_pB->PushAddr(&m_pCtx->m_State.nHI[0]);
	m_pB->Add();
	m_pB->Add();

	//Save HI
	m_pB->SeX32();
	m_pB->PullAddr(&m_pCtx->m_State.nHI[1]);
	m_pB->PullAddr(&m_pCtx->m_State.nHI[0]);
}

//04
void CMA_EE::PLZCW()
{
	CCodeGen::Begin(m_pB);
	{
		for(unsigned int i = 0; i < 2; i++)
		{
			CCodeGen::PushVar(&m_pCtx->m_State.nGPR[m_nRS].nV[i]);
			CCodeGen::Lzc();
			CCodeGen::PullVar(&m_pCtx->m_State.nGPR[m_nRD].nV[i]);
		}
	}
	CCodeGen::End();
}

//08
void CMA_EE::MMI0()
{
	m_pOpMmi0[(m_nOpcode >> 6) & 0x1F]();
}

//09
void CMA_EE::MMI2()
{
	m_pOpMmi2[(m_nOpcode >> 6) & 0x1F]();
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
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->PullAddr(&m_pCtx->m_State.nHI1[0]);

	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[1]);
	m_pB->PullAddr(&m_pCtx->m_State.nHI1[1]);
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
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->PullAddr(&m_pCtx->m_State.nLO1[0]);

	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[1]);
	m_pB->PullAddr(&m_pCtx->m_State.nLO1[1]);
}

//18
void CMA_EE::MULT1()
{
    Template_Mult32()(bind(&CCodeGen::MultS, m_codeGen), 1);
}

//19
void CMA_EE::MULTU1()
{
    Template_Mult32()(bind(&CCodeGen::Mult, m_codeGen), 1);
}

//1A
void CMA_EE::DIV1()
{
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->DivS();

	m_pB->SeX32();
	m_pB->PullAddr(&m_pCtx->m_State.nLO1[1]);
	m_pB->PullAddr(&m_pCtx->m_State.nLO1[0]);

	m_pB->SeX32();
	m_pB->PullAddr(&m_pCtx->m_State.nHI1[1]);
	m_pB->PullAddr(&m_pCtx->m_State.nHI1[0]);
}

//1B
void CMA_EE::DIVU1()
{
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[0]);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[0]);
	m_pB->Div();

	m_pB->SeX32();
	m_pB->PullAddr(&m_pCtx->m_State.nLO1[1]);
	m_pB->PullAddr(&m_pCtx->m_State.nLO1[0]);

	m_pB->SeX32();
	m_pB->PullAddr(&m_pCtx->m_State.nHI1[1]);
	m_pB->PullAddr(&m_pCtx->m_State.nHI1[0]);
}

//28
void CMA_EE::MMI1()
{
	m_pOpMmi1[(m_nOpcode >> 6) & 0x1F]();
}

//29
void CMA_EE::MMI3()
{
	m_pOpMmi3[(m_nOpcode >> 6) & 0x1F]();
}

//34
void CMA_EE::PSLLH()
{
	CCodeGen::Begin(m_pB);
	{
		PushVector(m_nRT);
		CVUI128::SllH(m_nSA);
		PullVector(m_nRD);
	}
	CCodeGen::End();
}

//36
void CMA_EE::PSRLH()
{
	CCodeGen::Begin(m_pB);
	{
		PushVector(m_nRT);
		CVUI128::SrlH(m_nSA);
		PullVector(m_nRD);
	}
	CCodeGen::End();
}

//37
void CMA_EE::PSRAH()
{
	CCodeGen::Begin(m_pB);
	{
		PushVector(m_nRT);
		CVUI128::SraH(m_nSA);
		PullVector(m_nRD);
	}
	CCodeGen::End();
}

//////////////////////////////////////////////////
//MMI0 Opcodes
//////////////////////////////////////////////////

//01
void CMA_EE::PSUBW()
{
	CCodeGen::Begin(m_pB);
	{
		PushVector(m_nRS);
		PushVector(m_nRT);
		CVUI128::SubW();
		PullVector(m_nRD);
	}
	CCodeGen::End();
}

//04
void CMA_EE::PADDH()
{
	CCodeGen::Begin(m_pB);
	{
		PushVector(m_nRS);
		PushVector(m_nRT);
		CVUI128::AddH();
		PullVector(m_nRD);
	}
	CCodeGen::End();
}

//06
void CMA_EE::PCGTH()
{
	CCodeGen::Begin(m_pB);
	{
		PushVector(m_nRS);
		PushVector(m_nRT);
		CVUI128::CmpGtH();
		PullVector(m_nRD);
	}
	CCodeGen::End();
}

//07
void CMA_EE::PMAXH()
{
	CCodeGen::Begin(m_pB);
	{
		PushVector(m_nRS);
		PushVector(m_nRT);
		CVUI128::MaxH();
		PullVector(m_nRD);
	}
	CCodeGen::End();
}

//09
void CMA_EE::PSUBB()
{
	CCodeGen::Begin(m_pB);
	{
		PushVector(m_nRS);
		PushVector(m_nRT);
		CVUI128::SubB();
		PullVector(m_nRD);
	}
	CCodeGen::End();
}

//10
void CMA_EE::PADDSW()
{
	CCodeGen::Begin(m_pB);
	{
		PushVector(m_nRS);
		PushVector(m_nRT);
		CVUI128::AddWSS();
		PullVector(m_nRD);
	}
	CCodeGen::End();
}

//12
void CMA_EE::PEXTLW()
{
	CCodeGen::Begin(m_pB);
	{
		PushVector(m_nRS);
		PushVector(m_nRT);
		CVUI128::UnpackLowerWD();
		PullVector(m_nRD);
	}
	CCodeGen::End();
}

//16
void CMA_EE::PEXTLH()
{
	CCodeGen::Begin(m_pB);
	{
		PushVector(m_nRS);
		PushVector(m_nRT);
		CVUI128::UnpackLowerHW();
		PullVector(m_nRD);
	}
	CCodeGen::End();
}

//17
void CMA_EE::PPACH()
{
	CCodeGen::Begin(m_pB);
	{
		PushVector(m_nRS);
		PushVector(m_nRT);
		CVUI128::PackWH();
		PullVector(m_nRD);
	}
	CCodeGen::End();
}

//1A
void CMA_EE::PEXTLB()
{
	CCodeGen::Begin(m_pB);
	{
		PushVector(m_nRS);
		PushVector(m_nRT);
		CVUI128::UnpackLowerBH();
		PullVector(m_nRD);
	}
	CCodeGen::End();
}

//1B
void CMA_EE::PPACB()
{
	CCodeGen::Begin(m_pB);
	{
		PushVector(m_nRS);
		PushVector(m_nRT);
		CVUI128::PackHB();
		PullVector(m_nRD);
	}
	CCodeGen::End();
}

//1E
void CMA_EE::PEXT5()
{
	unsigned int i;

	CCodeGen::Begin(m_pB);
	{
		for(i = 0; i < 4; i++)
		{
			CCodeGen::PushVar(&m_pCtx->m_State.nGPR[m_nRT].nV[i]);
			CCodeGen::PushCst(0x001F);
			CCodeGen::And();
			m_codeGen->Shl(3);

			CCodeGen::PushVar(&m_pCtx->m_State.nGPR[m_nRT].nV[i]);
			CCodeGen::PushCst(0x03E0);
			CCodeGen::And();
			m_codeGen->Shl(6);
			m_codeGen->Or();

			CCodeGen::PushVar(&m_pCtx->m_State.nGPR[m_nRT].nV[i]);
			CCodeGen::PushCst(0x7C00);
			CCodeGen::And();
			m_codeGen->Shl(9);
			m_codeGen->Or();

			CCodeGen::PushVar(&m_pCtx->m_State.nGPR[m_nRT].nV[i]);
			CCodeGen::PushCst(0x8000);
			CCodeGen::And();
			m_codeGen->Shl(16);
			m_codeGen->Or();

			CCodeGen::PullVar(&m_pCtx->m_State.nGPR[m_nRD].nV[i]);
		}
	}
	CCodeGen::End();
}

//////////////////////////////////////////////////
//MMI1 Opcodes
//////////////////////////////////////////////////

//02
void CMA_EE::PCEQW()
{
	CCodeGen::Begin(m_pB);
	{
		PushVector(m_nRS);
		PushVector(m_nRT);
		CVUI128::CmpEqW();
		PullVector(m_nRD);
	}
	CCodeGen::End();
}

//07
void CMA_EE::PMINH()
{
	CCodeGen::Begin(m_pB);
	{
		PushVector(m_nRS);
		PushVector(m_nRT);
		CVUI128::MinH();
		PullVector(m_nRD);
	}
	CCodeGen::End();
}

//10
void CMA_EE::PADDUW()
{
	CCodeGen::Begin(m_pB);
	{
		PushVector(m_nRS);
		PushVector(m_nRT);
		CVUI128::AddWUS();
		PullVector(m_nRD);
	}
	CCodeGen::End();
}

//12
void CMA_EE::PEXTUW()
{
	CCodeGen::Begin(m_pB);
	{
		PushVector(m_nRS);
		PushVector(m_nRT);
		CVUI128::UnpackUpperWD();
		PullVector(m_nRD);
	}
	CCodeGen::End();
}

//1A
void CMA_EE::PEXTUB()
{
	CCodeGen::Begin(m_pB);
	{
		PushVector(m_nRS);
		PushVector(m_nRT);
		CVUI128::UnpackUpperBH();
		PullVector(m_nRD);
	}
	CCodeGen::End();
}

//1B
void CMA_EE::QFSRV()
{
	CCodeGen::Begin(m_pB);
	{
		PushVector(m_nRS);
		PushVector(m_nRT);
		CVUI128::SrlVQw(&m_pCtx->m_State.nSA);
		PullVector(m_nRD);
	}
	CCodeGen::End();
}

//////////////////////////////////////////////////
//MMI2 Opcodes
//////////////////////////////////////////////////

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
	CCodeGen::Begin(m_pB);
	{
		CVUI128::Push(&m_pCtx->m_State.nGPR[m_nRS]);
		CVUI128::Push(&m_pCtx->m_State.nGPR[m_nRT]);
		CVUI128::And();
		CVUI128::Pull(&m_pCtx->m_State.nGPR[m_nRD]);
	}
	CCodeGen::End();
}

//13
void CMA_EE::PXOR()
{
	CCodeGen::Begin(m_pB);
	{
		CVUI128::Push(&m_pCtx->m_State.nGPR[m_nRS]);
		CVUI128::Push(&m_pCtx->m_State.nGPR[m_nRT]);
		CVUI128::Xor();
		CVUI128::Pull(&m_pCtx->m_State.nGPR[m_nRD]);
	}
	CCodeGen::End();
}

//////////////////////////////////////////////////
//MMI3 Opcodes
//////////////////////////////////////////////////

//0E
void CMA_EE::PCPYUD()
{
	//A
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[2]);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRS].nV[3]);
	
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[1]);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[0]);

	//B
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[2]);
	m_pB->PushAddr(&m_pCtx->m_State.nGPR[m_nRT].nV[3]);
	
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[3]);
	m_pB->PullAddr(&m_pCtx->m_State.nGPR[m_nRD].nV[2]);	
}

//12
void CMA_EE::POR()
{
	CCodeGen::Begin(m_pB);
	{
		CVUI128::Push(&m_pCtx->m_State.nGPR[m_nRS]);
		CVUI128::Push(&m_pCtx->m_State.nGPR[m_nRT]);
		CVUI128::Or();
		CVUI128::Pull(&m_pCtx->m_State.nGPR[m_nRD]);
	}
	CCodeGen::End();
}

//13
void CMA_EE::PNOR()
{
	CCodeGen::Begin(m_pB);
	{
		CVUI128::Push(&m_pCtx->m_State.nGPR[m_nRS]);
		CVUI128::Push(&m_pCtx->m_State.nGPR[m_nRT]);
		CVUI128::Or();
		CVUI128::Not();
		CVUI128::Pull(&m_pCtx->m_State.nGPR[m_nRD]);
	}
	CCodeGen::End();
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

//////////////////////////////////////////////////
//Opcode Tables
//////////////////////////////////////////////////

void (*CMA_EE::m_pOpMmi0[0x20])() = 
{
	//0x00
	Illegal,		PSUBW,			Illegal,		Illegal,		PADDH,			Illegal,		PCGTH,			PMAXH,
	//0x08
	Illegal,		PSUBB,			Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x10
	PADDSW,			Illegal,		PEXTLW,			Illegal,		Illegal,		Illegal,		PEXTLH,			PPACH,
	//0x18
	Illegal,		Illegal,		PEXTLB,			PPACB,			Illegal,		Illegal,		PEXT5,			Illegal,
};

void (*CMA_EE::m_pOpMmi1[0x20])() = 
{
	//0x00
	Illegal,		Illegal,		PCEQW,			Illegal,		Illegal,		Illegal,		Illegal,		PMINH,
	//0x08
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x10
	PADDUW,			Illegal,		PEXTUW,			Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x18
	Illegal,		Illegal,		PEXTUB,			QFSRV,			Illegal,		Illegal,		Illegal,		Illegal,
};

void (*CMA_EE::m_pOpMmi2[0x20])() = 
{
	//0x00
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x08
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		PCPYLD,			Illegal,
	//0x10
	Illegal,		Illegal,		PAND,			PXOR,			Illegal,		Illegal,		Illegal,		Illegal,
	//0x18
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
};

void (*CMA_EE::m_pOpMmi3[0x20])() = 
{
	//0x00
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x08
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		PCPYUD,			Illegal,
	//0x10
	Illegal,		Illegal,		POR,			PNOR,			Illegal,		Illegal,		Illegal,		Illegal,
	//0x18
	Illegal,		Illegal,		Illegal,		PCPYH,			Illegal,		Illegal,		Illegal,		Illegal,
};
