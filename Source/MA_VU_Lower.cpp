#include "MA_VU.h"
#include "MIPS.h"
#include "VIF.h"
#include "VUShared.h"
#include "CodeGen.h"
#include "CodeGen_FPU.h"

using namespace CodeGen;

uint32			CMA_VU::CLower::m_nOpcode;
uint8			CMA_VU::CLower::m_nImm5;
uint16			CMA_VU::CLower::m_nImm11;
uint16			CMA_VU::CLower::m_nImm15;
uint16			CMA_VU::CLower::m_nImm15S;
uint32			CMA_VU::CLower::m_nImm24;
uint8			CMA_VU::CLower::m_nIT;
uint8			CMA_VU::CLower::m_nIS;
uint8			CMA_VU::CLower::m_nID;
uint8			CMA_VU::CLower::m_nFSF;
uint8			CMA_VU::CLower::m_nFTF;
uint8			CMA_VU::CLower::m_nDest;

void CMA_VU::CLower::CompileInstruction(uint32 nAddress, CCacheBlock* pBlock, CMIPS* pCtx, bool nParent)
{
	uint32 nPrevOpcode, nNextOpcode;

	m_nOpcode	= pCtx->m_pMemoryMap->GetWord(nAddress);
	nPrevOpcode	= pCtx->m_pMemoryMap->GetWord(nAddress - 8);
	nNextOpcode = pCtx->m_pMemoryMap->GetWord(nAddress + 4);

	if(IsInstructionBranch(pCtx, nAddress - 8, nPrevOpcode))
	{
		m_pB->SetDelayedJumpCheck();
	}

	if(nNextOpcode & 0x80000000)
	{
//		LOI();
		return;
	}

	m_pB		= pBlock;
	m_pCtx		= pCtx;

	m_nDest		= (uint8 )((m_nOpcode >> 21) & 0x000F);

	m_nFSF		= ((m_nDest >> 0) & 0x03);
	m_nFTF		= ((m_nDest >> 2) & 0x03);

	m_nIT		= (uint8 )((m_nOpcode >> 16) & 0x001F);
	m_nIS		= (uint8 )((m_nOpcode >> 11) & 0x001F);
	m_nID		= (uint8 )((m_nOpcode >>  6) & 0x001F);
	m_nImm5		= m_nID;
	m_nImm11	= (uint16)((m_nOpcode >>  0) & 0x07FF);
	m_nImm15	= (uint16)((m_nOpcode & 0x7FF) | (m_nOpcode & 0x01E00000) >> 10);
	m_nImm15S	= m_nImm15 | (m_nImm15 & 0x4000 ? 0x8000 : 0x0000);
	m_nImm24	= m_nOpcode & 0x00FFFFFF;

	if(m_nOpcode != 0x8000033C)
	{
		m_pOpGeneral[m_nOpcode >> 25]();
	}
}

void CMA_VU::CLower::SetBranchAddress(bool nCondition, int32 nOffset)
{
	m_pB->PushImm(MIPS_INVALID_PC);
	m_pB->PullAddr(&m_pCtx->m_State.nDelayedJumpAddr);

	m_pB->BeginJcc(nCondition);
	{
		m_pB->PushAddr(&m_pCtx->m_State.nPC);
		m_pB->AddImm(nOffset);
		m_pB->PullAddr(&m_pCtx->m_State.nDelayedJumpAddr);
	}
	m_pB->EndJcc();
}

void CMA_VU::CLower::SetBranchAddressEx(bool nCondition, int32 nOffset)
{
	CCodeGen::BeginIfElse(nCondition);
	{
		CCodeGen::PushVar(&m_pCtx->m_State.nPC);
		CCodeGen::PushCst(nOffset);
		CCodeGen::Add();
		CCodeGen::PullVar(&m_pCtx->m_State.nDelayedJumpAddr);
	}
	CCodeGen::BeginIfElseAlt();
	{
		CCodeGen::PushCst(MIPS_INVALID_PC);
		CCodeGen::PullVar(&m_pCtx->m_State.nDelayedJumpAddr);
	}
	CCodeGen::EndIf();
}

uint32 CMA_VU::CLower::GetDestOffset(uint8 nDest)
{
	if(nDest & 0x0001) return 0xC;
	if(nDest & 0x0002) return 0x8;
	if(nDest & 0x0004) return 0x4;
	if(nDest & 0x0008) return 0x0;

	return 0;
}

//////////////////////////////////////////////////
//General Instructions
//////////////////////////////////////////////////

//00
void CMA_VU::CLower::LQ()
{
	unsigned int i;

	for(i = 0; i < 4; i++)
	{
		if(!VUShared::DestinationHasElement(m_nDest, i)) continue;

		m_pB->PushAddr(&m_pCtx->m_State.nCOP2VI[m_nIS]);
		m_pB->AddImm((uint32)VUShared::GetImm11Offset(m_nImm11));
		m_pB->SllImm(4);
		m_pB->AddImm(i * 4);

		m_pB->PushRef(m_pCtx);
		m_pB->Call(&CCacheBlock::GetWordProxy, 2, true);

		m_pB->PullAddr(VUShared::GetVectorElement(m_pCtx, m_nIT, i));
	}
}

//01
void CMA_VU::CLower::SQ()
{
	CCodeGen::Begin(m_pB);
	{
		for(unsigned int i = 0; i < 4; i++)
		{
			if(!VUShared::DestinationHasElement(m_nDest, i)) continue;

			///////////////////////////////
			// Push context

			CCodeGen::PushRef(m_pCtx);

			///////////////////////////////
			// Push value

			CCodeGen::PushVar(VUShared::GetVectorElement(m_pCtx, m_nIS, i));

			///////////////////////////////
			// Compute address

			CCodeGen::PushVar(&m_pCtx->m_State.nCOP2VI[m_nIT]);
			CCodeGen::PushCst((uint32)VUShared::GetImm11Offset(m_nImm11));
			CCodeGen::Add();
			CCodeGen::Shl(4);

			CCodeGen::PushCst(i * 4);
			CCodeGen::Add();

			///////////////////////////////
			// Call

			CCodeGen::Call(&CCacheBlock::SetWordProxy, 3, false);
		}

	}
	CCodeGen::End();
}

//04
void CMA_VU::CLower::ILW()
{
	m_pB->PushAddr(&m_pCtx->m_State.nCOP2VI[m_nIS]);
	m_pB->AddImm((uint32)VUShared::GetImm11Offset(m_nImm11));

	m_pB->SllImm(4);
	m_pB->AddImm(GetDestOffset(m_nDest));

	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::GetWordProxy, 2, true);
	m_pB->PullAddr(&m_pCtx->m_State.nCOP2VI[m_nIT]);
}

//05
void CMA_VU::CLower::ISW()
{
	m_pB->PushAddr(&m_pCtx->m_State.nCOP2VI[m_nIS]);
	m_pB->AddImm((uint32)VUShared::GetImm11Offset(m_nImm11));

	m_pB->SllImm(4);
	m_pB->AddImm(GetDestOffset(m_nDest));

	m_pB->PushAddr(&m_pCtx->m_State.nCOP2VI[m_nIT]);
	m_pB->AndImm(0xFFFF);

	m_pB->PushRef(m_pCtx);
	m_pB->Call(&CCacheBlock::SetWordProxy, 3, false);
}

//08
void CMA_VU::CLower::IADDIU()
{
	m_pB->PushAddr(&m_pCtx->m_State.nCOP2VI[m_nIS]);
	m_pB->AddImm((uint16)m_nImm15);
	m_pB->PullAddr(&m_pCtx->m_State.nCOP2VI[m_nIT]);
}

//09
void CMA_VU::CLower::ISUBIU()
{
	m_pB->PushAddr(&m_pCtx->m_State.nCOP2VI[m_nIS]);
	m_pB->SubImm((uint16)m_nImm15);
	m_pB->PullAddr(&m_pCtx->m_State.nCOP2VI[m_nIT]);
}

//11
void CMA_VU::CLower::FCSET()
{
	CCodeGen::Begin(m_pB);
	{
		CCodeGen::PushCst(m_nImm24);
		CCodeGen::PullVar(&m_pCtx->m_State.nCOP2CF);
	}
	CCodeGen::End();
}

//12
void CMA_VU::CLower::FCAND()
{
	CCodeGen::Begin(m_pB);
	{
		CCodeGen::PushVar(&m_pCtx->m_State.nCOP2CF);
		CCodeGen::PushCst(m_nImm24);
		CCodeGen::And();

		CCodeGen::PushCst(0);
		CCodeGen::Cmp(CCodeGen::CONDITION_EQ);

		CCodeGen::BeginIfElse(false);
		{
			CCodeGen::PushCst(1);
			CCodeGen::PullVar(&m_pCtx->m_State.nCOP2VI[1]);
		}
		CCodeGen::BeginIfElseAlt();
		{
			CCodeGen::PushCst(0);
			CCodeGen::PullVar(&m_pCtx->m_State.nCOP2VI[1]);
		}
		CCodeGen::EndIf();
	}
	CCodeGen::End();
}

//1A
void CMA_VU::CLower::FMAND()
{
	unsigned int i;

	//MAC flag temp
	m_pB->PushImm(0);

	//Check sign flags
	for(i = 0; i < 4; i++)
	{
		m_pB->PushAddr(&m_pCtx->m_State.nCOP2SF.nV[3 - i]);
		m_pB->PushImm(0);
		m_pB->Cmp(JCC_CONDITION_NE);
		m_pB->SllImm(4 + i);
		m_pB->Or();
	}

	m_pB->PushAddr(&m_pCtx->m_State.nCOP2VI[m_nIS]);
	m_pB->And();
	m_pB->PullAddr(&m_pCtx->m_State.nCOP2VI[m_nIT]);
}

//20
void CMA_VU::CLower::B()
{
	CCodeGen::Begin(m_pB);
	{
		CCodeGen::PushVar(&m_pCtx->m_State.nPC);
		CCodeGen::PushCst(VUShared::GetBranch(m_nImm11) + 4);
		CCodeGen::Add();
		CCodeGen::PullVar(&m_pCtx->m_State.nDelayedJumpAddr);
	}
	CCodeGen::End();
}

//28
void CMA_VU::CLower::IBEQ()
{
	m_pB->PushAddr(&m_pCtx->m_State.nCOP2VI[m_nIS]);
	m_pB->AndImm(0xFFFF);
	m_pB->PushAddr(&m_pCtx->m_State.nCOP2VI[m_nIT]);
	m_pB->AndImm(0xFFFF);

	m_pB->Cmp(JCC_CONDITION_EQ);

	SetBranchAddress(true, VUShared::GetBranch(m_nImm11) + 4);
}

//29
void CMA_VU::CLower::IBNE()
{
	m_pB->PushAddr(&m_pCtx->m_State.nCOP2VI[m_nIS]);
	m_pB->AndImm(0xFFFF);
	m_pB->PushAddr(&m_pCtx->m_State.nCOP2VI[m_nIT]);
	m_pB->AndImm(0xFFFF);

	m_pB->Cmp(JCC_CONDITION_EQ);

	SetBranchAddress(false, VUShared::GetBranch(m_nImm11) + 4);
}

//2C
void CMA_VU::CLower::IBLTZ()
{
	CCodeGen::Begin(m_pB);
	{
		CCodeGen::PushVar(&m_pCtx->m_State.nCOP2VI[m_nIS]);
		CCodeGen::PushCst(0x8000);
		CCodeGen::And();
		
		CCodeGen::PushCst(0);
		CCodeGen::Cmp(CCodeGen::CONDITION_EQ);

		SetBranchAddressEx(false, VUShared::GetBranch(m_nImm11) + 4);
	}
	CCodeGen::End();
}

//2F
void CMA_VU::CLower::IBGEZ()
{
	m_pB->PushImm(0);
	m_pB->PushAddr(&m_pCtx->m_State.nCOP2VI[m_nIS]);
	m_pB->AndImm(0x8000);

	m_pB->Cmp(JCC_CONDITION_EQ);

	SetBranchAddress(true, VUShared::GetBranch(m_nImm11) + 4);
}

//40
void CMA_VU::CLower::LOWEROP()
{
	m_pOpLower[m_nOpcode & 0x3F]();
}

//////////////////////////////////////////////////
//LowerOp Instructions
//////////////////////////////////////////////////

//30
void CMA_VU::CLower::IADD()
{
	m_pB->PushAddr(&m_pCtx->m_State.nCOP2VI[m_nIS]);
	m_pB->PushAddr(&m_pCtx->m_State.nCOP2VI[m_nIT]);
	m_pB->Add();
	m_pB->PullAddr(&m_pCtx->m_State.nCOP2VI[m_nID]);
}

//31
void CMA_VU::CLower::ISUB()
{
	CCodeGen::Begin(m_pB);
	{
		CCodeGen::PushVar(&m_pCtx->m_State.nCOP2VI[m_nIS]);
		CCodeGen::PushVar(&m_pCtx->m_State.nCOP2VI[m_nIT]);
		CCodeGen::Sub();
		CCodeGen::PullVar(&m_pCtx->m_State.nCOP2VI[m_nID]);
	}
	CCodeGen::End();
}

//32
void CMA_VU::CLower::IADDI()
{
	CCodeGen::Begin(m_pB);
	{
		CCodeGen::PushVar(&m_pCtx->m_State.nCOP2VI[m_nIS]);
		CCodeGen::PushCst(m_nImm5 | ((m_nImm5 & 0x10) != 0 ? 0xFFFFFFE0 : 0x0));
		CCodeGen::Add();
		CCodeGen::PullVar(&m_pCtx->m_State.nCOP2VI[m_nIT]);
	}
	CCodeGen::End();
}

//34
void CMA_VU::CLower::IAND()
{
	CCodeGen::Begin(m_pB);
	{
		CCodeGen::PushVar(&m_pCtx->m_State.nCOP2VI[m_nIS]);
		CCodeGen::PushVar(&m_pCtx->m_State.nCOP2VI[m_nIT]);
		CCodeGen::And();
		CCodeGen::PullVar(&m_pCtx->m_State.nCOP2VI[m_nID]);
	}
	CCodeGen::End();
}

//35
void CMA_VU::CLower::IOR()
{
	m_pB->PushAddr(&m_pCtx->m_State.nCOP2VI[m_nIS]);
	m_pB->PushAddr(&m_pCtx->m_State.nCOP2VI[m_nIT]);
	m_pB->Or();
	m_pB->PullAddr(&m_pCtx->m_State.nCOP2VI[m_nID]);
}

//3C
void CMA_VU::CLower::VECTOR0()
{
	m_pOpVector0[(m_nOpcode >> 6) & 0x1F]();
}

//3D
void CMA_VU::CLower::VECTOR1()
{
	m_pOpVector1[(m_nOpcode >> 6) & 0x1F]();
}

//3E
void CMA_VU::CLower::VECTOR2()
{
	m_pOpVector2[(m_nOpcode >> 6) & 0x1F]();
}

//3F
void CMA_VU::CLower::VECTOR3()
{
	m_pOpVector3[(m_nOpcode >> 6) & 0x1F]();
}

//////////////////////////////////////////////////
//Vector0 Instructions
//////////////////////////////////////////////////

//0C
void CMA_VU::CLower::MOVE()
{
	VUShared::MOVE(m_pB, m_pCtx, m_nDest, m_nIT, m_nIS);
}

//0D
void CMA_VU::CLower::LQI()
{
	unsigned int i;

	for(i = 0; i < 4; i++)
	{
		if(!VUShared::DestinationHasElement(m_nDest, i)) continue;

		m_pB->PushAddr(&m_pCtx->m_State.nCOP2VI[m_nIS]);
		m_pB->SllImm(4);
		m_pB->AddImm(i * 4);

		m_pB->PushRef(m_pCtx);
		m_pB->Call(&CCacheBlock::GetWordProxy, 2, true);

		m_pB->PullAddr(VUShared::GetVectorElement(m_pCtx, m_nIT, i));
	}

	m_pB->PushAddr(&m_pCtx->m_State.nCOP2VI[m_nIS]);
	m_pB->AddImm(1);
	m_pB->PullAddr(&m_pCtx->m_State.nCOP2VI[m_nIS]);
}

//0E
void CMA_VU::CLower::DIV()
{
	VUShared::DIV(m_pB, m_pCtx, m_nIS, m_nFSF, m_nIT, m_nFTF);
}

//0F
void CMA_VU::CLower::MTIR()
{
	m_pB->PushAddr(&m_pCtx->m_State.nCOP2[m_nIS].nV[m_nFSF]);
	m_pB->PullAddr(&m_pCtx->m_State.nCOP2VI[m_nIT]);
}

//19
void CMA_VU::CLower::MFP()
{
	CCodeGen::Begin(m_pB);
	{
		for(unsigned int i = 0; i < 4; i++)
		{
			if(!VUShared::DestinationHasElement(m_nDest, i)) continue;

			CCodeGen::PushVar(&m_pCtx->m_State.nCOP2P);
			CCodeGen::PullVar(&m_pCtx->m_State.nCOP2[m_nIT].nV[i]);
		}
	}
	CCodeGen::End();
}

//1A
void CMA_VU::CLower::XTOP()
{
	m_pB->PushAddr(CVIF::GetTop1Address());
	m_pB->PullAddr(&m_pCtx->m_State.nCOP2VI[m_nIT]);
}

//1B
void CMA_VU::CLower::XGKICK()
{
	m_pB->PushAddr(&m_pCtx->m_State.nCOP2VI[m_nIS]);
	m_pB->Call(CVIF::ProcessXGKICK, 1, false);
}

//////////////////////////////////////////////////
//Vector1 Instructions
//////////////////////////////////////////////////

//0C
void CMA_VU::CLower::MR32()
{
	VUShared::MR32(m_pB, m_pCtx, m_nDest, m_nIT, m_nIS);
}

//0D
void CMA_VU::CLower::SQI()
{
	unsigned int i;

	for(i = 0; i < 4; i++)
	{
		if(!VUShared::DestinationHasElement(m_nDest, i)) continue;

		m_pB->PushAddr(&m_pCtx->m_State.nCOP2VI[m_nIT]);
		m_pB->SllImm(4);
		m_pB->AddImm(i * 4);

		m_pB->PushAddr(VUShared::GetVectorElement(m_pCtx, m_nIS, i));

		m_pB->PushRef(m_pCtx);
		m_pB->Call(&CCacheBlock::SetWordProxy, 3, false);
	}

	m_pB->PushAddr(&m_pCtx->m_State.nCOP2VI[m_nIT]);
	m_pB->AddImm(1);
	m_pB->PullAddr(&m_pCtx->m_State.nCOP2VI[m_nIT]);
}

//0F
void CMA_VU::CLower::MFIR()
{
	CCodeGen::Begin(m_pB);
	{
		for(unsigned int i = 0; i < 4; i++)
		{
			if(!VUShared::DestinationHasElement(m_nDest, i)) continue;

			CCodeGen::PushVar(&m_pCtx->m_State.nCOP2VI[m_nIS]);
			CCodeGen::SeX16();
			CCodeGen::PullVar(VUShared::GetVectorElement(m_pCtx, m_nIT, i));
		}
	}
	CCodeGen::End();
}

//10
void CMA_VU::CLower::RGET()
{
	CCodeGen::Begin(m_pB);
	{
		for(unsigned int i = 0; i < 4; i++)
		{
			if(!VUShared::DestinationHasElement(m_nDest, i)) continue;

			CCodeGen::PushVar(&m_pCtx->m_State.nCOP2R);
			CCodeGen::PushCst(0x3F800000);
			CCodeGen::PullVar(VUShared::GetVectorElement(m_pCtx, m_nIT, i));
		}
	}
	CCodeGen::End();
}

//////////////////////////////////////////////////
//Vector2 Instructions
//////////////////////////////////////////////////

//10
void CMA_VU::CLower::RINIT()
{
	VUShared::RINIT(m_pB, m_pCtx, m_nIS, m_nFSF);
}

//1E
void CMA_VU::CLower::ERCPR()
{
	CCodeGen::Begin(m_pB);
	{
		CFPU::PushSingle(&m_pCtx->m_State.nCOP2[m_nIS].nV[m_nFSF]);
		CFPU::Rcpl();
		CFPU::PullSingle(&m_pCtx->m_State.nCOP2P);
	}
	CCodeGen::End();
}

//////////////////////////////////////////////////
//Vector3 Instructions
//////////////////////////////////////////////////

//0E
void CMA_VU::CLower::WAITQ()
{

}

//1C
void CMA_VU::CLower::ERLENG()
{
	CCodeGen::Begin(m_pB);
	{
		///////////////////////////////////////////////////
		//Raise all components to the power of 2

		CFPU::PushSingle(&m_pCtx->m_State.nCOP2[m_nIS].nV[0]);
		CFPU::Dup();
		CFPU::Mul();

		CFPU::PushSingle(&m_pCtx->m_State.nCOP2[m_nIS].nV[1]);
		CFPU::Dup();
		CFPU::Mul();

		CFPU::PushSingle(&m_pCtx->m_State.nCOP2[m_nIS].nV[2]);
		CFPU::Dup();
		CFPU::Mul();

		///////////////////////////////////////////////////
		//Sum all components

		CFPU::Add();
		CFPU::Add();

		///////////////////////////////////////////////////
		//Extract root, inverse

		CFPU::Sqrt();
		CFPU::Rcpl();

		CFPU::PullSingle(&m_pCtx->m_State.nCOP2P);
	}
	CCodeGen::End();
}

//////////////////////////////////////////////////
//Opcode Tables
//////////////////////////////////////////////////

void (*CMA_VU::CLower::m_pOpGeneral[0x80])() =
{
	//0x00
	LQ,				SQ,				Illegal,		Illegal,		ILW,			ISW,			Illegal,		Illegal,
	//0x08
	IADDIU,			ISUBIU,			Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x10
	Illegal,		FCSET,			FCAND,			Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x18
	Illegal,		Illegal,		FMAND,			Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x20
	B,				Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x28
	IBEQ,			IBNE,			Illegal,		Illegal,		IBLTZ,			Illegal,		Illegal,		IBGEZ,
	//0x30
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x38
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x40
	LOWEROP,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x48
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x50
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x58
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x60
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x68
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x70
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x78
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
};

void (*CMA_VU::CLower::m_pOpLower[0x40])() =
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
	IADD,			ISUB,			IADDI,			Illegal,		IAND,			IOR,			Illegal,		Illegal,
	//0x38
	Illegal,		Illegal,		Illegal,		Illegal,		VECTOR0,		VECTOR1,		VECTOR2,		VECTOR3,
};

void (*CMA_VU::CLower::m_pOpVector0[0x20])() =
{
	//0x00
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x08
	Illegal,		Illegal,		Illegal,		Illegal,		MOVE,			LQI,			DIV,			MTIR,
	//0x10
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x18
	Illegal,		MFP,			XTOP,			XGKICK,			Illegal,		Illegal,		Illegal,		Illegal,
};

void (*CMA_VU::CLower::m_pOpVector1[0x20])() =
{
	//0x00
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x08
	Illegal,		Illegal,		Illegal,		Illegal,		MR32,			SQI,			Illegal,		MFIR,
	//0x10
	RGET,			Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x18
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
};

void (*CMA_VU::CLower::m_pOpVector2[0x20])() =
{
	//0x00
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x08
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x10
	RINIT,			Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x18
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		ERCPR,			Illegal,
};

void (*CMA_VU::CLower::m_pOpVector3[0x20])() =
{
	//0x00
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x08
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		WAITQ,			Illegal,
	//0x10
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x18
	Illegal,		Illegal,		Illegal,		Illegal,		ERLENG,			Illegal,		Illegal,		Illegal,
};
