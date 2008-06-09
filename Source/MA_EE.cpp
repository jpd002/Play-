#include <stddef.h>
#include "MA_EE.h"
#include "MIPS.h"
#include "CodeGen.h"
#include "PS2OS.h"
#include "offsetof_def.h"
#include "MemoryUtils.h"

using namespace std;
using namespace std::tr1;

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
    m_pOpSpecial2[0x3F] = PSRAW;

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

//////////////////////////////////////////////////
//General Opcodes
//////////////////////////////////////////////////

//1E
void CMA_EE::LQ()
{
    ComputeMemAccessAddr();

    for(unsigned int i = 0; i < 4; i++)
    {
	    m_codeGen->PushRef(m_pCtx);
	    m_codeGen->PushIdx(1);
	    m_codeGen->Call(reinterpret_cast<void*>(&CMemoryUtils::GetWordProxy), 2, true);

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
	ComputeMemAccessAddr();

	for(unsigned int i = 0; i < 4; i++)
	{
		m_codeGen->PushRef(m_pCtx);
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[i]));
		m_codeGen->PushIdx(2);
		m_codeGen->Call(reinterpret_cast<void*>(&CMemoryUtils::SetWordProxy), 3, false);

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
    //prod = (HI || LO) + (RS * RT)
    //LO = sex(prod[0])
    //HI = sex(prod[1])
    //RD = LO

    size_t lo[2];
    size_t hi[2];

    unsigned int unit = 0;
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
        throw runtime_error("Invalid unit number.");
        break;
    }

    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
    m_codeGen->MultS();     //gives Stack(0) = LO, Stack(1) = HI
    m_codeGen->Swap();

    m_codeGen->PushRel(lo[0]);
    m_codeGen->PushRel(hi[0]);
    m_codeGen->Add64();

    m_codeGen->SeX();
    m_codeGen->PullRel(hi[1]);
    m_codeGen->PullRel(hi[0]);

    m_codeGen->SeX();
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
    Template_Div32()(bind(&CCodeGen::DivS, m_codeGen), 1);
}

//1B
void CMA_EE::DIVU1()
{
    Template_Div32()(bind(&CCodeGen::Div, m_codeGen), 1);
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

//01
void CMA_EE::PSUBW()
{
    PushVector(m_nRS);
    PushVector(m_nRT);
    m_codeGen->MD_SubW();
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

//07
void CMA_EE::PMINH()
{
	PushVector(m_nRS);
	PushVector(m_nRT);
    m_codeGen->MD_MinH();
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

//1F
void CMA_EE::PROT3W()
{
    //3
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[3]));
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[3]));

    //2
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[2]));

    //1
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[2]));
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));

    //0
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
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
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		PROT3W,
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
