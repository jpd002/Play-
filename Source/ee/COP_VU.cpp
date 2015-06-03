#include <assert.h>
#include "COP_VU.h"
#include "VUShared.h"
#include "../Log.h"
#include "../MIPS.h"
#include "../MemoryUtils.h"
#include "../Ps2Const.h"
#include "offsetof_def.h"

#define LOG_NAME				("vpu")

CCOP_VU::CCOP_VU(MIPS_REGSIZE nRegSize) 
: CMIPSCoprocessor(nRegSize)
, m_nFT(0)
, m_nFS(0)
, m_nFD(0)
, m_nDest(0)
, m_nFTF(0)
, m_nFSF(0)
, m_nBc(0)
, m_nIT(0)
, m_nIS(0)
, m_nID(0)
, m_nImm5(0)
, m_nImm15(0)
{
	SetupReflectionTables();
}

void CCOP_VU::CompileInstruction(uint32 nAddress, CMipsJitter* codeGen, CMIPS* pCtx)
{
	SetupQuickVariables(nAddress, codeGen, pCtx);

	m_nDest			= (uint8)((m_nOpcode >> 21) & 0x0F);
	
	m_nFSF			= ((m_nDest >> 0) & 0x03);
	m_nFTF			= ((m_nDest >> 2) & 0x03);

	m_nFT			= (uint8)((m_nOpcode >> 16) & 0x1F);
	m_nFS			= (uint8)((m_nOpcode >> 11) & 0x1F);
	m_nFD			= (uint8)((m_nOpcode >>  6) & 0x1F);

	m_nBc			= (uint8)((m_nOpcode >>  0) & 0x03);

	m_nIT			= m_nFT;
	m_nIS			= m_nFS;
	m_nID			= m_nFD;
	m_nImm5			= m_nID;
	m_nImm15		= (uint16)((m_nOpcode >> 6) & 0x7FFF);

	switch((m_nOpcode >> 26) & 0x3F)
	{
	case 0x12:
		//COP2
		((this)->*(m_pOpCop2[(m_nOpcode >> 21) & 0x1F]))();
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

	m_codeGen->PushCtx();
	m_codeGen->PushIdx(1);
	m_codeGen->Call(reinterpret_cast<void*>(&MemoryUtils_GetQuadProxy), 2, Jitter::CJitter::RETURN_VALUE_128);
	m_codeGen->MD_PullRel(offsetof(CMIPS, m_State.nCOP2[m_nFT]));

	m_codeGen->PullTop();
}

//3E
void CCOP_VU::SQC2()
{
	ComputeMemAccessAddr();

	m_codeGen->PushCtx();
	m_codeGen->MD_PushRel(offsetof(CMIPS, m_State.nCOP2[m_nFT]));
	m_codeGen->PushIdx(2);
	m_codeGen->Call(reinterpret_cast<void*>(&MemoryUtils_SetQuadProxy), 3, Jitter::CJitter::RETURN_VALUE_NONE);

	m_codeGen->PullTop();
}

//////////////////////////////////////////////////
//COP2 Instructions
//////////////////////////////////////////////////

//01
void CCOP_VU::QMFC2()
{
	for(unsigned int i = 0; i < 4; i++)
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2[m_nFS].nV[i]));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nFT].nV[i]));
	}
}

//02
void CCOP_VU::CFC2()
{
	if(m_nFS < 16)
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[m_nFS]));
		m_codeGen->PushCst(0xFFFF);
		m_codeGen->And();
	}
	else
	{
		switch(m_nFS)
		{
		case 18:	//Clipping flag
			m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2CF));
			break;
		case 16:	//STATUS
			CLog::GetInstance().Print(LOG_NAME, "Warning: Reading contents of STATUS flag through CFC2.\r\n");
			m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[0].nV[0]));
			break;
		case 20:	//R
			CLog::GetInstance().Print(LOG_NAME, "Warning: Reading contents of R register through CFC2.\r\n");
			m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[0].nV[0]));
			break;
		case 17:	//MAC flag
#ifdef _DEBUG
			CLog::GetInstance().Print(LOG_NAME, "Warning: Reading contents of MAC flag through CFC2.\r\n");
#endif
		case 26:	//TPC
		case 28:	//FBRST
		case 29:	//VPU-STAT
			m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[0].nV[0]));
			break;
		case 21:
			m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2I));
			break;
		case 22:
			m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2Q));
			break;
		default:
			assert(0);
			break;
		}
	}

	m_codeGen->PushTop();
	m_codeGen->SignExt();
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nFT].nV[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nFT].nV[0]));
}

//05
void CCOP_VU::QMTC2()
{
	if(m_nFS == 0) return;

	for(unsigned int i = 0; i < 4; i++)
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nFT].nV[i]));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2[m_nFS].nV[i]));
	}
}

//06
void CCOP_VU::CTC2()
{
	if(m_nFS == 0)
	{
		//Moving stuff in VI0 register? (probably synchronizing with VU0 micro subroutine execution)
	}
	else if((m_nFS > 0) && (m_nFS < 16))
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nFT].nV[0]));
		m_codeGen->PushCst(0xFFFF);
		m_codeGen->And();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[m_nFS]));
	}
	else
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nFT].nV[0]));

		switch(m_nFS)
		{
		case 16:
			//STATUS - Not implemented
#ifdef _DEBUG
			CLog::GetInstance().Print(LOG_NAME, "Warning: Writing contents of STATUS flag through CTC2.\r\n");
#endif
			m_codeGen->PullTop();
			break;
		case 17:
			//MAC - Not implemented
#ifdef _DEBUG
			CLog::GetInstance().Print(LOG_NAME, "Warning: Writing contents of MAC flag through CTC2.\r\n");
#endif
			m_codeGen->PullTop();
			break;
		case 18:
			//Clipping flag
			m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2CF));
			break;
		case 21:
			m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2I));
			break;
		case 22:
			m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2Q));
			break;
		case 27:
			//CMSAR0
			m_codeGen->PullRel(offsetof(CMIPS, m_State.cmsar0));
			break;
		case 28:
			//FBRST - Don't care
			m_codeGen->PullTop();
			break;
		case 31:
			//CMSAR1 - Not implemented (this should start vu1 like vcallmsr starts vu0.)
#ifdef _DEBUG
			CLog::GetInstance().Print(LOG_NAME, "Warning: Writing CMSAR1 through CTC2 (start vu1.)\r\n");
#endif
			m_codeGen->PullTop();
			break;
		default:
			throw std::runtime_error("Not implemented.");
			break;
		}
	}
}

//10-1F
void CCOP_VU::V()
{
	((this)->*(m_pOpVector[(m_nOpcode & 0x3F)]))();
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
	VUShared::ADDbc(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc, 0);
}

//04
//05
//06
//07
void CCOP_VU::VSUBbc()
{
	VUShared::SUBbc(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc, 0);
}

//08
//09
//0A
//0B
void CCOP_VU::VMADDbc()
{
	VUShared::MADDbc(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc, 0);
}

//0C
//0D
//0E
//0F
void CCOP_VU::VMSUBbc()
{
	VUShared::MSUBbc(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc);
}

//10
//11
//12
//13
void CCOP_VU::VMAXbc()
{
	VUShared::MAXbc(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc);
}

//14
//15
//16
//17
void CCOP_VU::VMINIbc()
{
	VUShared::MINIbc(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc);
}

//18
//19
//1A
//1B
void CCOP_VU::VMULbc()
{
	VUShared::MULbc(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, m_nBc, 0);
}

//1C
void CCOP_VU::VMULq()
{
	VUShared::MULq(m_codeGen, m_nDest, m_nFD, m_nFS, m_nAddress);
}

//1E
void CCOP_VU::VMULi()
{
	VUShared::MULi(m_codeGen, m_nDest, m_nFD, m_nFS);
}

//1F
void CCOP_VU::VMINIi()
{
	VUShared::MINIi(m_codeGen, m_nDest, m_nFD, m_nFS);
}

//20
void CCOP_VU::VADDq()
{
	VUShared::ADDq(m_codeGen, m_nDest, m_nFD, m_nFS);
}

//21
void CCOP_VU::VMADDq()
{
	VUShared::MADDq(m_codeGen, m_nDest, m_nFD, m_nFS, 0);
}

//23
void CCOP_VU::VMADDi()
{
	VUShared::MADDi(m_codeGen, m_nDest, m_nFD, m_nFS, 0);
}

//24
void CCOP_VU::VSUBq()
{
	VUShared::SUBq(m_codeGen, m_nDest, m_nFD, m_nFS);
}

//25
void CCOP_VU::VMSUBq()
{
	VUShared::MSUBq(m_codeGen, m_nDest, m_nFD, m_nFS);
}

//26
void CCOP_VU::VSUBi()
{
	VUShared::SUBi(m_codeGen, m_nDest, m_nFD, m_nFS, 0);
}

//27
void CCOP_VU::VMSUBi()
{
	VUShared::MSUBi(m_codeGen, m_nDest, m_nFD, m_nFS);
}

//28
void CCOP_VU::VADD()
{
	VUShared::ADD(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, 0);
}

//29
void CCOP_VU::VMADD()
{
	VUShared::MADD(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, 0);
}

//2A
void CCOP_VU::VMUL()
{
	VUShared::MUL(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT);
}

//2B
void CCOP_VU::VMAX()
{
	VUShared::MAX(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT);
}

//2C
void CCOP_VU::VSUB()
{
	VUShared::SUB(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, 0);
}

//2D
void CCOP_VU::VMSUB()
{
	VUShared::MSUB(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT, 0);
}

//2E
void CCOP_VU::VOPMSUB()
{
	VUShared::OPMSUB(m_codeGen, m_nFD, m_nFS, m_nFT, 0);
}

//2F
void CCOP_VU::VMINI()
{
	VUShared::MINI(m_codeGen, m_nDest, m_nFD, m_nFS, m_nFT);
}

//30
void CCOP_VU::VIADD()
{
	VUShared::IADD(m_codeGen, m_nID, m_nIS, m_nIT);
}

//31
void CCOP_VU::VISUB()
{
	VUShared::ISUB(m_codeGen, m_nID, m_nIS, m_nIT);
}

//32
void CCOP_VU::VIADDI()
{
	VUShared::IADDI(m_codeGen, m_nIT, m_nIS, m_nImm5);
}

//34
void CCOP_VU::VIAND()
{
	VUShared::IAND(m_codeGen, m_nID, m_nIS, m_nIT);
}

//35
void CCOP_VU::VIOR()
{
	VUShared::IOR(m_codeGen, m_nID, m_nIS, m_nIT);
}

//38
void CCOP_VU::VCALLMS()
{
	m_codeGen->PushCst(1);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.callMsEnabled));

	m_codeGen->PushCst(static_cast<uint32>(m_nImm15) * 8);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.callMsAddr));

	m_codeGen->PushCst(MIPS_EXCEPTION_CALLMS);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nHasException));
}

//39
void CCOP_VU::VCALLMSR()
{
	m_codeGen->PushCst(1);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.callMsEnabled));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.cmsar0));
	m_codeGen->Shl(3);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.callMsAddr));

	m_codeGen->PushCst(MIPS_EXCEPTION_CALLMS);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nHasException));
}

//3C
void CCOP_VU::VX0()
{
	((this)->*(m_pOpVx0[(m_nOpcode >> 6) & 0x1F]))();
}

//3D
void CCOP_VU::VX1()
{
	((this)->*(m_pOpVx1[(m_nOpcode >> 6) & 0x1F]))();
}

//3E
void CCOP_VU::VX2()
{
	((this)->*(m_pOpVx2[(m_nOpcode >> 6) & 0x1F]))();
}

//3F
void CCOP_VU::VX3()
{
	((this)->*(m_pOpVx3[(m_nOpcode >> 6) & 0x1F]))();
}

//////////////////////////////////////////////////
//Vx Common Instructions
//////////////////////////////////////////////////

//
void CCOP_VU::VADDAbc()
{
	VUShared::ADDAbc(m_codeGen, m_nDest, m_nFS, m_nFT, m_nBc);
}

//
void CCOP_VU::VSUBAbc()
{
	VUShared::SUBAbc(m_codeGen, m_nDest, m_nFS, m_nFT, m_nBc);
}

//
void CCOP_VU::VMADDAbc()
{
	VUShared::MADDAbc(m_codeGen, m_nDest, m_nFS, m_nFT, m_nBc, 0);
}

//
void CCOP_VU::VMSUBAbc()
{
	VUShared::MSUBAbc(m_codeGen, m_nDest, m_nFS, m_nFT, m_nBc, 0);
}

//
void CCOP_VU::VMULAbc()
{
	VUShared::MULAbc(m_codeGen, m_nDest, m_nFS, m_nFT, m_nBc, 0);
}

//////////////////////////////////////////////////
//V0 Instructions
//////////////////////////////////////////////////

//04
void CCOP_VU::VITOF0()
{
	VUShared::ITOF0(m_codeGen, m_nDest, m_nFT, m_nFS);
}

//05
void CCOP_VU::VFTOI0()
{
	VUShared::FTOI0(m_codeGen, m_nDest, m_nFT, m_nFS);
}

//07
void CCOP_VU::VMULAq()
{
	VUShared::MULAq(m_codeGen, m_nDest, m_nFS);
}

//0A
void CCOP_VU::VADDA()
{
	VUShared::ADDA(m_codeGen, m_nDest, m_nFS, m_nFT);
}

//0B
void CCOP_VU::VSUBA()
{
	VUShared::SUBA(m_codeGen, m_nDest, m_nFS, m_nFT);
}

//0C
void CCOP_VU::VMOVE()
{
	VUShared::MOVE(m_codeGen, m_nDest, m_nFT, m_nFS);
}

//0D
void CCOP_VU::VLQI()
{
	VUShared::LQI(m_codeGen, m_nDest, m_nIT, m_nIS, PS2::VUMEM0ADDR);
}

//0E
void CCOP_VU::VDIV()
{
	VUShared::DIV(m_codeGen, m_nFS, m_nFSF, m_nFT, m_nFTF, 0);
}

//0F
void CCOP_VU::VMTIR()
{
	VUShared::MTIR(m_codeGen, m_nIT, m_nIS, m_nFSF);
}

//10
void CCOP_VU::VRNEXT()
{
	VUShared::RNEXT(m_codeGen, m_nDest, m_nFT);
}

//////////////////////////////////////////////////
//V1 Instructions
//////////////////////////////////////////////////

//04
void CCOP_VU::VITOF4()
{
	VUShared::ITOF4(m_codeGen, m_nDest, m_nFT, m_nFS);
}

//05
void CCOP_VU::VFTOI4()
{
	VUShared::FTOI4(m_codeGen, m_nDest, m_nFT, m_nFS);
}

//07
void CCOP_VU::VABS()
{
	VUShared::ABS(m_codeGen, m_nDest, m_nFT, m_nFS);
}

//0A
void CCOP_VU::VMADDA()
{
	VUShared::MADDA(m_codeGen, m_nDest, m_nFS, m_nFT, 0);
}

//0B
void CCOP_VU::VMSUBA()
{
	VUShared::MSUBA(m_codeGen, m_nDest, m_nFS, m_nFT, 0);
}

//0C
void CCOP_VU::VMR32()
{
	VUShared::MR32(m_codeGen, m_nDest, m_nFT, m_nFS);
}

//0D
void CCOP_VU::VSQI()
{
	VUShared::SQI(m_codeGen, m_nDest, m_nIS, m_nIT, PS2::VUMEM0ADDR);
}

//0E
void CCOP_VU::VSQRT()
{
	VUShared::SQRT(m_codeGen, m_nFT, m_nFTF, 0);
}

//0F
void CCOP_VU::VMFIR()
{
	VUShared::MFIR(m_codeGen, m_nDest, m_nIT, m_nIS);
}

//10
void CCOP_VU::VRGET()
{
	VUShared::RGET(m_codeGen, m_nDest, m_nFT);
}

//////////////////////////////////////////////////
//V2 Instructions
//////////////////////////////////////////////////

//04
void CCOP_VU::VITOF12()
{
	VUShared::ITOF12(m_codeGen, m_nDest, m_nFT, m_nFS);
}

//05
void CCOP_VU::VFTOI12()
{
	VUShared::FTOI12(m_codeGen, m_nDest, m_nFT, m_nFS);
}

//07
void CCOP_VU::VMULAi()
{
	VUShared::MULAi(m_codeGen, m_nDest, m_nFS);
}

//0A
void CCOP_VU::VMULA()
{
	VUShared::MULA(m_codeGen, m_nDest, m_nFS, m_nFT);
}

//0B
void CCOP_VU::VOPMULA()
{
	VUShared::OPMULA(m_codeGen, m_nFS, m_nFT);
}

//0E
void CCOP_VU::VRSQRT()
{
	VUShared::RSQRT(m_codeGen, m_nFS, m_nFSF, m_nFT, m_nFTF, 0);
}

//0F
void CCOP_VU::VILWR()
{
	VUShared::ILWR(m_codeGen, m_nDest, m_nIT, m_nIS, PS2::VUMEM0ADDR);
}

//10
void CCOP_VU::VRINIT()
{
	VUShared::RINIT(m_codeGen, m_nFS, m_nFSF);
}

//////////////////////////////////////////////////
//V3 Instructions
//////////////////////////////////////////////////

//04
void CCOP_VU::VITOF15()
{
	VUShared::ITOF15(m_codeGen, m_nDest, m_nFT, m_nFS);
}

//05
void CCOP_VU::VFTOI15()
{
	VUShared::FTOI15(m_codeGen, m_nDest, m_nFT, m_nFS);
}

//07
void CCOP_VU::VCLIP()
{
	VUShared::CLIP(m_codeGen, m_nFS, m_nFT);
}

//08
void CCOP_VU::VMADDAi()
{
	VUShared::MADDAi(m_codeGen, m_nDest, m_nFS, 0);
}

//09
void CCOP_VU::VMSUBAi()
{
	VUShared::MSUBAi(m_codeGen, m_nDest, m_nFS, 0);
}

//0B
void CCOP_VU::VNOP()
{
	//Nothing to do
}

//0D
void CCOP_VU::VSQD()
{
	VUShared::SQD(m_codeGen, m_nDest, m_nIS, m_nIT, PS2::VUMEM0ADDR);
}

//0E
void CCOP_VU::VWAITQ()
{
	VUShared::WAITQ(m_codeGen);
}

//0F
void CCOP_VU::VISWR()
{
	VUShared::ISWR(m_codeGen, m_nDest, m_nIT, m_nIS, PS2::VUMEM0ADDR);
}

//10
void CCOP_VU::VRXOR()
{
	VUShared::RXOR(m_codeGen, m_nFS, m_nFSF);
}

//////////////////////////////////////////////////
//Opcode Tables
//////////////////////////////////////////////////

CCOP_VU::InstructionFuncConstant CCOP_VU::m_pOpCop2[0x20] =
{
	//0x00
	&CCOP_VU::Illegal,			&CCOP_VU::QMFC2,			&CCOP_VU::CFC2,			&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::QMTC2,			&CCOP_VU::CTC2,			&CCOP_VU::Illegal,
	//0x08
	&CCOP_VU::Illegal,			&CCOP_VU::Illegal,			&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,			&CCOP_VU::Illegal,		&CCOP_VU::Illegal,
	//0x10
	&CCOP_VU::V,				&CCOP_VU::V,				&CCOP_VU::V,			&CCOP_VU::V,			&CCOP_VU::V,			&CCOP_VU::V,				&CCOP_VU::V,			&CCOP_VU::V,
	//0x18
	&CCOP_VU::V,				&CCOP_VU::V,				&CCOP_VU::V,			&CCOP_VU::V,			&CCOP_VU::V,			&CCOP_VU::V,				&CCOP_VU::V,			&CCOP_VU::V,
};

CCOP_VU::InstructionFuncConstant CCOP_VU::m_pOpVector[0x40] =
{
	//0x00
	&CCOP_VU::VADDbc,		&CCOP_VU::VADDbc,		&CCOP_VU::VADDbc,		&CCOP_VU::VADDbc,		&CCOP_VU::VSUBbc,		&CCOP_VU::VSUBbc,		&CCOP_VU::VSUBbc,		&CCOP_VU::VSUBbc,
	//0x08
	&CCOP_VU::VMADDbc,		&CCOP_VU::VMADDbc,		&CCOP_VU::VMADDbc,		&CCOP_VU::VMADDbc,		&CCOP_VU::VMSUBbc,		&CCOP_VU::VMSUBbc,		&CCOP_VU::VMSUBbc,		&CCOP_VU::VMSUBbc,
	//0x10
	&CCOP_VU::VMAXbc,		&CCOP_VU::VMAXbc,		&CCOP_VU::VMAXbc,		&CCOP_VU::VMAXbc,		&CCOP_VU::VMINIbc,		&CCOP_VU::VMINIbc,		&CCOP_VU::VMINIbc,		&CCOP_VU::VMINIbc,
	//0x18
	&CCOP_VU::VMULbc,		&CCOP_VU::VMULbc,		&CCOP_VU::VMULbc,		&CCOP_VU::VMULbc,		&CCOP_VU::VMULq,		&CCOP_VU::Illegal,		&CCOP_VU::VMULi,		&CCOP_VU::VMINIi,
	//0x20
	&CCOP_VU::VADDq,		&CCOP_VU::VMADDq,		&CCOP_VU::Illegal,		&CCOP_VU::VMADDi,		&CCOP_VU::VSUBq,		&CCOP_VU::VMSUBq,		&CCOP_VU::VSUBi,		&CCOP_VU::VMSUBi,
	//0x28
	&CCOP_VU::VADD,			&CCOP_VU::VMADD,		&CCOP_VU::VMUL,			&CCOP_VU::VMAX,			&CCOP_VU::VSUB,			&CCOP_VU::VMSUB,		&CCOP_VU::VOPMSUB,		&CCOP_VU::VMINI,
	//0x30
	&CCOP_VU::VIADD,		&CCOP_VU::VISUB,		&CCOP_VU::VIADDI,		&CCOP_VU::Illegal,		&CCOP_VU::VIAND,		&CCOP_VU::VIOR,			&CCOP_VU::Illegal,		&CCOP_VU::Illegal,
	//0x38
	&CCOP_VU::VCALLMS,		&CCOP_VU::VCALLMSR,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::VX0,			&CCOP_VU::VX1,			&CCOP_VU::VX2,			&CCOP_VU::VX3,
};

CCOP_VU::InstructionFuncConstant CCOP_VU::m_pOpVx0[0x20] =
{
	//0x00
	&CCOP_VU::VADDAbc,		&CCOP_VU::VSUBAbc,		&CCOP_VU::VMADDAbc,		&CCOP_VU::VMSUBAbc,		&CCOP_VU::VITOF0,		&CCOP_VU::VFTOI0,		&CCOP_VU::VMULAbc,		&CCOP_VU::VMULAq,
	//0x08
	&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::VADDA,		&CCOP_VU::VSUBA,		&CCOP_VU::VMOVE,		&CCOP_VU::VLQI,			&CCOP_VU::VDIV,			&CCOP_VU::VMTIR,
	//0x10
	&CCOP_VU::VRNEXT,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,
	//0x18
	&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,
};

CCOP_VU::InstructionFuncConstant CCOP_VU::m_pOpVx1[0x20] =
{
	//0x00
	&CCOP_VU::VADDAbc,		&CCOP_VU::VSUBAbc,		&CCOP_VU::VMADDAbc,		&CCOP_VU::VMSUBAbc,		&CCOP_VU::VITOF4,		&CCOP_VU::VFTOI4,		&CCOP_VU::VMULAbc,		&CCOP_VU::VABS,
	//0x08
	&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::VMADDA,		&CCOP_VU::VMSUBA,		&CCOP_VU::VMR32,		&CCOP_VU::VSQI,			&CCOP_VU::VSQRT,		&CCOP_VU::VMFIR,
	//0x10
	&CCOP_VU::VRGET,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,
	//0x18
	&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,
};

CCOP_VU::InstructionFuncConstant CCOP_VU::m_pOpVx2[0x20] =
{
	//0x00
	&CCOP_VU::VADDAbc,		&CCOP_VU::VSUBAbc,		&CCOP_VU::VMADDAbc,		&CCOP_VU::VMSUBAbc,		&CCOP_VU::VITOF12,		&CCOP_VU::VFTOI12,		&CCOP_VU::VMULAbc,		&CCOP_VU::VMULAi,
	//0x08
	&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::VMULA,		&CCOP_VU::VOPMULA,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::VRSQRT,		&CCOP_VU::VILWR,
	//0x10
	&CCOP_VU::VRINIT,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,
	//0x18
	&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,
};

CCOP_VU::InstructionFuncConstant CCOP_VU::m_pOpVx3[0x20] =
{
	//0x00
	&CCOP_VU::VADDAbc,		&CCOP_VU::VSUBAbc,		&CCOP_VU::VMADDAbc,		&CCOP_VU::VMSUBAbc,		&CCOP_VU::VITOF15,		&CCOP_VU::VFTOI15,		&CCOP_VU::VMULAbc,		&CCOP_VU::VCLIP,
	//0x08
	&CCOP_VU::VMADDAi,		&CCOP_VU::VMSUBAi,		&CCOP_VU::Illegal,		&CCOP_VU::VNOP,			&CCOP_VU::Illegal,		&CCOP_VU::VSQD,			&CCOP_VU::VWAITQ,		&CCOP_VU::VISWR,
	//0x10
	&CCOP_VU::VRXOR,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,
	//0x18
	&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,		&CCOP_VU::Illegal,
};
