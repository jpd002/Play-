#include <assert.h>
#include "COP_VU.h"
#include "VUShared.h"
#include "MIPS.h"
#include "CodeGen.h"

CCOP_VU			g_COPVU(MIPS_REGSIZE_64);

uint8			CCOP_VU::m_nFT		= 0;
uint8			CCOP_VU::m_nFS		= 0;
uint8			CCOP_VU::m_nFD		= 0;

uint8			CCOP_VU::m_nDest	= 0;
uint8			CCOP_VU::m_nFTF		= 0;
uint8			CCOP_VU::m_nFSF		= 0;
uint8			CCOP_VU::m_nBc		= 0;

CCOP_VU::CCOP_VU(MIPS_REGSIZE nRegSize) :
CMIPSCoprocessor(nRegSize)
{
	SetupReflectionTables();
}

void CCOP_VU::CompileInstruction(uint32 nAddress, CCacheBlock* pBlock, CMIPS* pCtx, bool nParent)
{
	if(nParent)
	{
		SetupQuickVariables(nAddress, pBlock, pCtx);
	}

	m_nDest			= (uint8)((m_nOpcode >> 21) & 0x0F);
	
	m_nFSF			= ((m_nDest >> 0) & 0x03);
	m_nFTF			= ((m_nDest >> 2) & 0x03);

	m_nFT			= (uint8)((m_nOpcode >> 16) & 0x1F);
	m_nFS			= (uint8)((m_nOpcode >> 11) & 0x1F);
	m_nFD			= (uint8)((m_nOpcode >>  6) & 0x1F);

	m_nBc			= (uint8)((m_nOpcode >>  0) & 0x03);

	switch((m_nOpcode >> 26) & 0x3F)
	{
	case 0x12:
		//COP2
		m_pOpCop2[(m_nOpcode >> 21) & 0x1F]();
		break;
	case 0x36:
		//LQC2
		LQC2();
		break;
	case 0x3E:
		//SQC2
		SQC2();
		break;
	default:
		Illegal();
		break;
	}
}

//////////////////////////////////////////////////
//General Instructions
//////////////////////////////////////////////////

//36
void CCOP_VU::LQC2()
{
	ComputeMemAccessAddr();

	//Load the word
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::GetWordProxy, 1, true);
	m_pB->PullAddr(&m_pCtx->m_State.nCOP2[m_nFT].nV0);
	m_pB->AddImm(4);

	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::GetWordProxy, 1, true);
	m_pB->PullAddr(&m_pCtx->m_State.nCOP2[m_nFT].nV1);
	m_pB->AddImm(4);

	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::GetWordProxy, 1, true);
	m_pB->PullAddr(&m_pCtx->m_State.nCOP2[m_nFT].nV2);
	m_pB->AddImm(4);

	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::GetWordProxy, 2, true);
	m_pB->PullAddr(&m_pCtx->m_State.nCOP2[m_nFT].nV3);
}

//3E
void CCOP_VU::SQC2()
{
	ComputeMemAccessAddr();

	//Write the words
	m_pB->PushAddr(&m_pCtx->m_State.nCOP2[m_nFT].nV0);
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::SetWordProxy, 2, false);
	m_pB->AddImm(4);

	m_pB->PushAddr(&m_pCtx->m_State.nCOP2[m_nFT].nV1);
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::SetWordProxy, 2, false);
	m_pB->AddImm(4);

	m_pB->PushAddr(&m_pCtx->m_State.nCOP2[m_nFT].nV2);
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::SetWordProxy, 2, false);
	m_pB->AddImm(4);

	m_pB->PushAddr(&m_pCtx->m_State.nCOP2[m_nFT].nV3);
	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::SetWordProxy, 3, false);	
}

//////////////////////////////////////////////////
//COP2 Instructions
//////////////////////////////////////////////////

//01
void CCOP_VU::QMFC2()
{
	unsigned int i;

	CCodeGen::Begin(m_pB);
	{
		for(i = 0; i < 4; i++)
		{
			CCodeGen::PushVar(&m_pCtx->m_State.nCOP2[m_nFS].nV[i]);
			CCodeGen::PullVar(&m_pCtx->m_State.nGPR[m_nFT].nV[i]);
		}
	}
	CCodeGen::End();
}

//02
void CCOP_VU::CFC2()
{
	CCodeGen::Begin(m_pB);
	{
		if(m_nFS < 16)
		{
			CCodeGen::PushVar(&m_pCtx->m_State.nCOP2VI[m_nFS]);
			CCodeGen::PushCst(0xFFFF);
			CCodeGen::And();
		}
		else
		{
			switch(m_nFS)
			{
			case 16:	//STATUS
			case 17:	//MAC flag
			case 18:	//Clipping flag
			case 20:	//R
			case 26:	//TPC
			case 28:	//FBRST
			case 29:	//VPU-STAT
				CCodeGen::PushVar(&m_pCtx->m_State.nGPR[0].nV[0]);
				break;
			case 21:
				CCodeGen::PushVar(&m_pCtx->m_State.nCOP2I);
				break;
			case 22:
				CCodeGen::PushVar(&m_pCtx->m_State.nCOP2Q);
				break;
			default:
				assert(0);
				break;
			}
		}

		CCodeGen::SeX();
		CCodeGen::PullVar(&m_pCtx->m_State.nGPR[m_nFT].nV[1]);
		CCodeGen::PullVar(&m_pCtx->m_State.nGPR[m_nFT].nV[0]);
	}
	CCodeGen::End();
}

//05
void CCOP_VU::QMTC2()
{
	unsigned int i;

	CCodeGen::Begin(m_pB);
	{
		for(i = 0; i < 4; i++)
		{
			CCodeGen::PushVar(&m_pCtx->m_State.nGPR[m_nFT].nV[i]);
			CCodeGen::PullVar(&m_pCtx->m_State.nCOP2[m_nFS].nV[i]);
		}
	}
	CCodeGen::End();
}

//06
void CCOP_VU::CTC2()
{

}

//10-1F
void CCOP_VU::V()
{
	m_pOpVector[(m_nOpcode & 0x3F)]();
}

//////////////////////////////////////////////////
//Vector Instructions
//////////////////////////////////////////////////

//00
//01
//02
//03
void CCOP_VU::VADDbc()
{
	VUShared::ADDbc(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc);
}

//04
void CCOP_VU::VSUBbc()
{
	VUShared::SUBbc(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc);
}

//0B
void CCOP_VU::VMADDbc()
{
	VUShared::MADDbc(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc);
}

//10
void CCOP_VU::VMAXbc()
{
	VUShared::MAXbc(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc);
}

//14
void CCOP_VU::VMINIbc()
{
	VUShared::MINIbc(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc);
}

//18
//1B
void CCOP_VU::VMULbc()
{
	VUShared::MULbc(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc);
}

//1C
void CCOP_VU::VMULq()
{
	VUShared::MULq(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS);
}

//20
void CCOP_VU::VADDq()
{
	VUShared::ADDq(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS);
}

//28
void CCOP_VU::VADD()
{
	VUShared::ADD(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS, m_nFT);
}

//2A
void CCOP_VU::VMUL()
{
	VUShared::MUL(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS, m_nFT);
}

//2B
void CCOP_VU::VMAX()
{
	VUShared::MAX(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS, m_nFT);
}

//2C
void CCOP_VU::VSUB()
{
	VUShared::SUB(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS, m_nFT);
}

//2E
void CCOP_VU::VOPMSUB()
{
	VUShared::OPMSUB(m_pB, m_pCtx, m_nFD, m_nFS, m_nFT);
}

//2F
void CCOP_VU::VMINI()
{
	VUShared::MINI(m_pB, m_pCtx, m_nDest, m_nFD, m_nFS, m_nFT);
}

//3C
void CCOP_VU::VX0()
{
	m_pOpVx0[(m_nOpcode >> 6) & 0x1F]();
}

//3D
void CCOP_VU::VX1()
{
	m_pOpVx1[(m_nOpcode >> 6) & 0x1F]();
}

//3E
void CCOP_VU::VX2()
{
	m_pOpVx2[(m_nOpcode >> 6) & 0x1F]();
}

//3F
void CCOP_VU::VX3()
{
	m_pOpVx3[(m_nOpcode >> 6) & 0x1F]();
}

//////////////////////////////////////////////////
//Vx Common Instructions
//////////////////////////////////////////////////

//
void CCOP_VU::VMULAbc()
{
	VUShared::MULAbc(m_pB, m_pCtx, m_nDest, m_nFS, m_nFT, m_nBc);
}

//
void CCOP_VU::VMADDAbc()
{
	VUShared::MADDAbc(m_pB, m_pCtx, m_nDest, m_nFS, m_nFT, m_nBc);
}

//////////////////////////////////////////////////
//V0 Instructions
//////////////////////////////////////////////////

//04
void CCOP_VU::VITOF0()
{
	VUShared::ITOF0(m_pB, m_pCtx, m_nDest, m_nFT, m_nFS);
}

//05
void CCOP_VU::VFTOI0()
{
	VUShared::FTOI0(m_pB, m_pCtx, m_nDest, m_nFT, m_nFS);
}

//0C
void CCOP_VU::VMOVE()
{
	VUShared::MOVE(m_pB, m_pCtx, m_nDest, m_nFT, m_nFS);
}

//0E
void CCOP_VU::VDIV()
{
	VUShared::DIV(m_pB, m_pCtx, m_nFS, m_nFSF, m_nFT, m_nFTF);
}

//////////////////////////////////////////////////
//V1 Instructions
//////////////////////////////////////////////////

//05
void CCOP_VU::VFTOI4()
{
	VUShared::FTOI4(m_pB, m_pCtx, m_nDest, m_nFT, m_nFS);
}

//0C
void CCOP_VU::VMR32()
{
	VUShared::MR32(m_pB, m_pCtx, m_nDest, m_nFT, m_nFS);
}

//0E
void CCOP_VU::VSQRT()
{
	VUShared::SQRT(m_pB, m_pCtx, m_nFT, m_nFTF);
}

//////////////////////////////////////////////////
//V2 Instructions
//////////////////////////////////////////////////

//0B
void CCOP_VU::VOPMULA()
{
	VUShared::OPMULA(m_pB, m_pCtx, m_nFS, m_nFT);
}

//0E
void CCOP_VU::VRSQRT()
{
	VUShared::RSQRT(m_pB, m_pCtx, m_nFS, m_nFSF, m_nFT, m_nFTF);
}

//////////////////////////////////////////////////
//V3 Instructions
//////////////////////////////////////////////////

//0B
void CCOP_VU::VNOP()
{
	//Nothing to do
}

//0E
void CCOP_VU::VWAITQ()
{
	//Nothing to do
}

//////////////////////////////////////////////////
//Opcode Tables
//////////////////////////////////////////////////

void (*CCOP_VU::m_pOpCop2[0x20])() =
{
	//0x00
	Illegal,		QMFC2,			CFC2,			Illegal,		Illegal,		QMTC2,			CTC2,			Illegal,
	//0x08
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x10
	V,				V,				V,				V,				V,				V,				V,				V,
	//0x18
	V,				V,				V,				V,				V,				V,				V,				V,
};

void (*CCOP_VU::m_pOpVector[0x40])() =
{
	//0x00
	VADDbc,			VADDbc,			VADDbc,			VADDbc,			VSUBbc,			Illegal,		Illegal,		Illegal,
	//0x08
	Illegal,		Illegal,		VMADDbc,		VMADDbc,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x10
	VMAXbc,			Illegal,		Illegal,		Illegal,		VMINIbc,		Illegal,		Illegal,		Illegal,
	//0x18
	VMULbc,			Illegal,		Illegal,		VMULbc,			VMULq,			Illegal,		Illegal,		Illegal,
	//0x20
	VADDq,			Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x28
	VADD,			Illegal,		VMUL,			VMAX,			VSUB,			Illegal,		VOPMSUB,		VMINI,
	//0x30
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x38
	Illegal,		Illegal,		Illegal,		Illegal,		VX0,			VX1,			VX2,			VX3,
};

void (*CCOP_VU::m_pOpVx0[0x20])() =
{
	//0x00
	Illegal,		Illegal,		Illegal,		Illegal,		VITOF0,			VFTOI0,			VMULAbc,		Illegal,
	//0x08
	Illegal,		Illegal,		Illegal,		Illegal,		VMOVE,			Illegal,		VDIV,			Illegal,
	//0x10
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x18
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
};

void (*CCOP_VU::m_pOpVx1[0x20])() =
{
	//0x00
	Illegal,		Illegal,		VMADDAbc,		Illegal,		Illegal,		VFTOI4,			VMULAbc,		Illegal,
	//0x08
	Illegal,		Illegal,		Illegal,		Illegal,		VMR32,			Illegal,		VSQRT,			Illegal,
	//0x10
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x18
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
};

void (*CCOP_VU::m_pOpVx2[0x20])() =
{
	//0x00
	Illegal,		Illegal,		VMADDAbc,		Illegal,		Illegal,		Illegal,		VMULAbc,		Illegal,
	//0x08
	Illegal,		Illegal,		Illegal,		VOPMULA,		Illegal,		Illegal,		VRSQRT,			Illegal,
	//0x10
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x18
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
};

void (*CCOP_VU::m_pOpVx3[0x20])() =
{
	//0x00
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x08
	Illegal,		Illegal,		Illegal,		VNOP,			Illegal,		Illegal,		VWAITQ,			Illegal,
	//0x10
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x18
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
};
