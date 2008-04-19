#include <stddef.h>
#include "MA_VU.h"
#include "VIF.h"
#include "MIPS.h"
#include "VUShared.h"
#include "CodeGen.h"
#include "offsetof_def.h"
#include "MemoryUtils.h"

using namespace std;

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

void CMA_VU::CLower::CompileInstruction(uint32 nAddress, CCodeGen* codeGen, CMIPS* pCtx, bool nParent)
{
    uint32 nPrevOpcode = pCtx->m_pMemoryMap->GetWord(nAddress - 8);
    uint32 nNextOpcode = pCtx->m_pMemoryMap->GetWord(nAddress + 4);

    if(nNextOpcode & 0x80000000)
    {
	    return;
    }

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
    m_codeGen->BeginIfElse(nCondition);
    {
        m_codeGen->PushCst(m_nAddress + nOffset + 4);
        m_codeGen->PullRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));
    }
    m_codeGen->BeginIfElseAlt();
    {
        m_codeGen->PushCst(MIPS_INVALID_PC);
        m_codeGen->PullRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));
    }
    m_codeGen->EndIf();
}

void CMA_VU::CLower::PushIntegerRegister(unsigned int nRegister)
{
    if(nRegister == 0)
    {
        m_codeGen->PushCst(0);
    }
    else
    {
        m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[nRegister]));
    }
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
    for(unsigned int i = 0; i < 4; i++)
    {
        if(!VUShared::DestinationHasElement(m_nDest, i)) continue;

        ///////////////////////////////
        // Push context

        m_codeGen->PushRef(m_pCtx);

        ///////////////////////////////
        // Compute address

        PushIntegerRegister(m_nIS);
        m_codeGen->PushCst(static_cast<uint32>(VUShared::GetImm11Offset(m_nImm11)));
        m_codeGen->Add();
        m_codeGen->Shl(4);

        m_codeGen->PushCst(i * 4);
        m_codeGen->Add();

        ///////////////////////////////
        // Call

        m_codeGen->Call(reinterpret_cast<void*>(&CMemoryUtils::GetWordProxy), 2, true);

        ///////////////////////////////
        // Store value

        m_codeGen->PullRel(VUShared::GetVectorElement(m_nIT, i));
    }
}

//01
void CMA_VU::CLower::SQ()
{
    for(unsigned int i = 0; i < 4; i++)
    {
        if(!VUShared::DestinationHasElement(m_nDest, i)) continue;

        ///////////////////////////////
        // Push context

        m_codeGen->PushRef(m_pCtx);

        ///////////////////////////////
        // Push value

        m_codeGen->PushRel(VUShared::GetVectorElement(m_nIS, i));

        ///////////////////////////////
        // Compute address

        PushIntegerRegister(m_nIT);
        m_codeGen->PushCst(static_cast<uint32>(VUShared::GetImm11Offset(m_nImm11)));
        m_codeGen->Add();
        m_codeGen->Shl(4);

        m_codeGen->PushCst(i * 4);
        m_codeGen->Add();

        ///////////////////////////////
        // Call

        m_codeGen->Call(reinterpret_cast<void*>(&CMemoryUtils::SetWordProxy), 3, false);
    }
}

//04
void CMA_VU::CLower::ILW()
{
    //Push context
    m_codeGen->PushRef(m_pCtx);

    //Compute address
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIS]));
    m_codeGen->PushCst(static_cast<uint32>(VUShared::GetImm11Offset(m_nImm11)));
    m_codeGen->Add();

    m_codeGen->Shl(4);
    m_codeGen->PushCst(GetDestOffset(m_nDest));
    m_codeGen->Add();

    //Read memory
    m_codeGen->Call(reinterpret_cast<void*>(&CMemoryUtils::GetWordProxy), 2, true);
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIT]));
}

//05
void CMA_VU::CLower::ISW()
{
    //Push context
    m_codeGen->PushRef(m_pCtx);

    //Compute value
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIT]));
    m_codeGen->PushCst(0xFFFF);
    m_codeGen->And();

    //Compute address
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIS]));
    m_codeGen->PushCst(static_cast<uint32>(VUShared::GetImm11Offset(m_nImm11)));
    m_codeGen->Add();

    m_codeGen->Shl(4);
    m_codeGen->PushCst(GetDestOffset(m_nDest));
    m_codeGen->Add();

    m_codeGen->Call(reinterpret_cast<void*>(&CMemoryUtils::SetWordProxy), 3, false);
}

//08
void CMA_VU::CLower::IADDIU()
{
    PushIntegerRegister(m_nIS);
    m_codeGen->PushCst(static_cast<uint16>(m_nImm15));
    m_codeGen->Add();
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIT]));
}

//09
void CMA_VU::CLower::ISUBIU()
{
    PushIntegerRegister(m_nIS);
    m_codeGen->PushCst(static_cast<uint16>(m_nImm15));
    m_codeGen->Sub();
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIT]));
}

//11
void CMA_VU::CLower::FCSET()
{
    m_codeGen->PushCst(m_nImm24);
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2CF));
}

//12
void CMA_VU::CLower::FCAND()
{
    throw runtime_error("Reimplement.");
	//CCodeGen::Begin(m_pB);
	//{
	//	CCodeGen::PushVar(&m_pCtx->m_State.nCOP2CF);
	//	CCodeGen::PushCst(m_nImm24);
	//	CCodeGen::And();

	//	CCodeGen::PushCst(0);
	//	CCodeGen::Cmp(CCodeGen::CONDITION_EQ);

	//	CCodeGen::BeginIfElse(false);
	//	{
	//		CCodeGen::PushCst(1);
	//		CCodeGen::PullVar(&m_pCtx->m_State.nCOP2VI[1]);
	//	}
	//	CCodeGen::BeginIfElseAlt();
	//	{
	//		CCodeGen::PushCst(0);
	//		CCodeGen::PullVar(&m_pCtx->m_State.nCOP2VI[1]);
	//	}
	//	CCodeGen::EndIf();
	//}
	//CCodeGen::End();
}

//1A
void CMA_VU::CLower::FMAND()
{
    printf("Warning: Using FMAND.\r\n");

    //MAC flag temp
    m_codeGen->PushCst(0);

    for(unsigned int i = 0; i < 4; i++)
    {
        m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2SF.nV[3 - i]));
        m_codeGen->PushCst(0);
        m_codeGen->Cmp(CCodeGen::CONDITION_NE);
        m_codeGen->Shl(4 + i);
        m_codeGen->Or();
    }

    m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIS]));
    m_codeGen->And();
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIT]));
}

//20
void CMA_VU::CLower::B()
{
    m_codeGen->PushCst(1);
    SetBranchAddress(true, VUShared::GetBranch(m_nImm11) + 4);
}

//25
void CMA_VU::CLower::JALR()
{
    throw runtime_error("Reimplement.");
	//CCodeGen::Begin(m_pB);
	//{
	//	//Save PC
	//	CCodeGen::PushRel(offsetof(CMIPS, m_State.nPC));
	//	CCodeGen::PushCst(0x0C);
	//	CCodeGen::Add();
	//	CCodeGen::PullRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIT]));

	//	//Compute new PC
	//	CCodeGen::PushRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIS]));
	//	//Decomment -> CCodeGen::Shl(3);
	//	CCodeGen::PushCst(0x4000);
	//	CCodeGen::Add();
	//	CCodeGen::PullRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));
	//}
	//CCodeGen::End();
}

//28
void CMA_VU::CLower::IBEQ()
{
    //Operand 1
    PushIntegerRegister(m_nIS);
    m_codeGen->PushCst(0xFFFF);
    m_codeGen->And();

    //Operand 2
    PushIntegerRegister(m_nIT);
    m_codeGen->PushCst(0xFFFF);
    m_codeGen->And();

    m_codeGen->Cmp(CCodeGen::CONDITION_EQ);

    SetBranchAddress(true, VUShared::GetBranch(m_nImm11) + 4);
}

//29
void CMA_VU::CLower::IBNE()
{
    //Operand 1
    PushIntegerRegister(m_nIS);
    m_codeGen->PushCst(0xFFFF);
    m_codeGen->And();

    //Operand 2
    PushIntegerRegister(m_nIT);
    m_codeGen->PushCst(0xFFFF);
    m_codeGen->And();

    m_codeGen->Cmp(CCodeGen::CONDITION_EQ);

    SetBranchAddress(false, VUShared::GetBranch(m_nImm11) + 4);
}

//2C
void CMA_VU::CLower::IBLTZ()
{
    throw runtime_error("Reimplement.");
	//CCodeGen::Begin(m_pB);
	//{
	//	CCodeGen::PushVar(&m_pCtx->m_State.nCOP2VI[m_nIS]);
	//	CCodeGen::PushCst(0x8000);
	//	CCodeGen::And();
	//	
	//	CCodeGen::PushCst(0);
	//	CCodeGen::Cmp(CCodeGen::CONDITION_EQ);

	//	SetBranchAddress(false, VUShared::GetBranch(m_nImm11) + 4);
	//}
	//CCodeGen::End();
}

//2D
void CMA_VU::CLower::IBGTZ()
{
    throw runtime_error("Reimplement.");
	//CCodeGen::Begin(m_pB);
	//{
	//	CCodeGen::PushRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIS]));
	//	CCodeGen::SeX16();

	//	CCodeGen::PushCst(0);
	//	CCodeGen::Cmp(CCodeGen::CONDITION_GT);
	//
	//	SetBranchAddress(true, VUShared::GetBranch(m_nImm11) + 4);
	//}
	//CCodeGen::End();
}

//2E
void CMA_VU::CLower::IBLEZ()
{
    throw runtime_error("Reimplement.");
	//CCodeGen::Begin(m_pB);
	//{
	//	CCodeGen::PushRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIS]));
	//	CCodeGen::SeX16();

	//	CCodeGen::PushCst(0);
	//	CCodeGen::Cmp(CCodeGen::CONDITION_LE);
	//
	//	SetBranchAddress(true, VUShared::GetBranch(m_nImm11) + 4);
	//}
	//CCodeGen::End();
}

//2F
void CMA_VU::CLower::IBGEZ()
{
    m_codeGen->PushCst(0);
    
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIS]));
    m_codeGen->PushCst(0x8000);
    m_codeGen->And();

    m_codeGen->Cmp(CCodeGen::CONDITION_EQ);

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
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIS]));
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIT]));
    m_codeGen->Add();
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[m_nID]));
}

//31
void CMA_VU::CLower::ISUB()
{
    throw runtime_error("Reimplement.");
	//CCodeGen::Begin(m_pB);
	//{
	//	CCodeGen::PushVar(&m_pCtx->m_State.nCOP2VI[m_nIS]);
	//	CCodeGen::PushVar(&m_pCtx->m_State.nCOP2VI[m_nIT]);
	//	CCodeGen::Sub();
	//	CCodeGen::PullVar(&m_pCtx->m_State.nCOP2VI[m_nID]);
	//}
	//CCodeGen::End();
}

//32
void CMA_VU::CLower::IADDI()
{
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIS]));
    m_codeGen->PushCst(m_nImm5 | ((m_nImm5 & 0x10) != 0 ? 0xFFFFFFE0 : 0x0));
    m_codeGen->Add();
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIT]));
}

//34
void CMA_VU::CLower::IAND()
{
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIS]));
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIT]));
    m_codeGen->And();
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[m_nID]));
}

//35
void CMA_VU::CLower::IOR()
{
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIS]));
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIT]));
    m_codeGen->Or();
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[m_nID]));
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
    VUShared::MOVE(m_codeGen, m_nDest, m_nIT, m_nIS);
}

//0D
void CMA_VU::CLower::LQI()
{
    for(unsigned int i = 0; i < 4; i++)
    {
        if(!VUShared::DestinationHasElement(m_nDest, i)) continue;

        ///////////////////////////////
        // Push context

        m_codeGen->PushRef(m_pCtx);

        ///////////////////////////////
        // Compute address

        PushIntegerRegister(m_nIS);
        m_codeGen->Shl(4);

        m_codeGen->PushCst(i * 4);
        m_codeGen->Add();

        ///////////////////////////////
        // Call

        m_codeGen->Call(reinterpret_cast<void*>(&CMemoryUtils::GetWordProxy), 2, true);

        ///////////////////////////////
        // Store result

        m_codeGen->PullRel(VUShared::GetVectorElement(m_nIT, i)); 
    }

    m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIS]));
    m_codeGen->PushCst(1);
    m_codeGen->Add();
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIS]));
}

//0E
void CMA_VU::CLower::DIV()
{
    VUShared::DIV(m_codeGen, m_nIS, m_nFSF, m_nIT, m_nFTF);
}

//0F
void CMA_VU::CLower::MTIR()
{
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2[m_nIS].nV[m_nFSF]));
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIT]));
}

//19
void CMA_VU::CLower::MFP()
{
    throw runtime_error("Reimplement.");
	//CCodeGen::Begin(m_pB);
	//{
	//	for(unsigned int i = 0; i < 4; i++)
	//	{
	//		if(!VUShared::DestinationHasElement(m_nDest, i)) continue;

	//		CCodeGen::PushVar(&m_pCtx->m_State.nCOP2P);
	//		CCodeGen::PullVar(&m_pCtx->m_State.nCOP2[m_nIT].nV[i]);
	//	}
	//}
	//CCodeGen::End();
}

//1A
void CMA_VU::CLower::XTOP()
{
    //Push context
    m_codeGen->PushRef(m_pCtx);

    //Compute Address
    m_codeGen->PushCst(CVIF::VU1_TOP);

    m_codeGen->Call(reinterpret_cast<void*>(&CMemoryUtils::GetWordProxy), 2, true);
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIT]));
}

//1B
void CMA_VU::CLower::XGKICK()
{
    //Push context
    m_codeGen->PushRef(m_pCtx);

    //Push value
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIS]));

    //Compute Address
    m_codeGen->PushCst(CVIF::VU1_XGKICK);

    m_codeGen->Call(reinterpret_cast<void*>(&CMemoryUtils::SetWordProxy), 3, false);
}

//////////////////////////////////////////////////
//Vector1 Instructions
//////////////////////////////////////////////////

//0C
void CMA_VU::CLower::MR32()
{
    VUShared::MR32(m_codeGen, m_nDest, m_nIT, m_nIS);
}

//0D
void CMA_VU::CLower::SQI()
{
    for(unsigned int i = 0; i < 4; i++)
    {
        if(!VUShared::DestinationHasElement(m_nDest, i)) continue;

        ///////////////////////////////
        // Push context

        m_codeGen->PushRef(m_pCtx);

        ///////////////////////////////
        // Push value

        m_codeGen->PushRel(VUShared::GetVectorElement(m_nIS, i)); 

        ///////////////////////////////
        // Compute address

        PushIntegerRegister(m_nIT);
        m_codeGen->Shl(4);

        m_codeGen->PushCst(i * 4);
        m_codeGen->Add();

        ///////////////////////////////
        // Call

        m_codeGen->Call(reinterpret_cast<void*>(&CMemoryUtils::SetWordProxy), 3, false);
    }

    m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIT]));
    m_codeGen->PushCst(1);
    m_codeGen->Add();
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIT]));
}

//0F
void CMA_VU::CLower::MFIR()
{
    for(unsigned int i = 0; i < 4; i++)
    {
        if(!VUShared::DestinationHasElement(m_nDest, i)) continue;

        PushIntegerRegister(m_nIS);
        m_codeGen->SeX16();
        m_codeGen->PullRel(VUShared::GetVectorElement(m_nIT, i));
    }
}

//10
void CMA_VU::CLower::RGET()
{
    throw runtime_error("Reimplement.");
	//CCodeGen::Begin(m_pB);
	//{
	//	for(unsigned int i = 0; i < 4; i++)
	//	{
	//		if(!VUShared::DestinationHasElement(m_nDest, i)) continue;

	//		CCodeGen::PushVar(&m_pCtx->m_State.nCOP2R);
	//		CCodeGen::PushCst(0x3F800000);
	//		CCodeGen::PullVar(VUShared::GetVectorElement(m_pCtx, m_nIT, i));
	//	}
	//}
	//CCodeGen::End();
}

//////////////////////////////////////////////////
//Vector2 Instructions
//////////////////////////////////////////////////

//0F
void CMA_VU::CLower::ILWR()
{
    throw runtime_error("Reimplement.");
	//CCodeGen::Begin(m_pB);
	//{
	//	Push context
	//	CCodeGen::PushRef(m_pCtx);

	//	Compute Address
	//	CCodeGen::PushRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIS]));
	//	Decomment -> CCodeGen::Shl(4);
	//	CCodeGen::PushCst(GetDestOffset(m_nDest));
	//	CCodeGen::Add();

	//	CCodeGen::Call(reinterpret_cast<void*>(&CMemoryUtils::GetWordProxy), 2, true);

	//	CCodeGen::PullRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIT]));
	//}
	//CCodeGen::End();
}

//10
void CMA_VU::CLower::RINIT()
{
    throw runtime_error("Reimplement.");
//	VUShared::RINIT(m_pB, m_pCtx, m_nIS, m_nFSF);
}

//1E
void CMA_VU::CLower::ERCPR()
{
    throw runtime_error("Reimplement.");
	//CCodeGen::Begin(m_pB);
	//{
	//	CFPU::PushSingle(&m_pCtx->m_State.nCOP2[m_nIS].nV[m_nFSF]);
	//	CFPU::Rcpl();
	//	CFPU::PullSingle(&m_pCtx->m_State.nCOP2P);
	//}
	//CCodeGen::End();
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
    ///////////////////////////////////////////////////
    //Raise all components to the power of 2

    m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP2[m_nIS].nV[0]));
    m_codeGen->PushTop();
    m_codeGen->FP_Mul();

    m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP2[m_nIS].nV[1]));
    m_codeGen->PushTop();
    m_codeGen->FP_Mul();

    m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP2[m_nIS].nV[2]));
    m_codeGen->PushTop();
    m_codeGen->FP_Mul();

    ///////////////////////////////////////////////////
    //Sum all components

    m_codeGen->FP_Add();
    m_codeGen->FP_Add();

    ///////////////////////////////////////////////////
    //Extract root, inverse

    m_codeGen->FP_Rsqrt();

    m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP2P));
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
	B,				Illegal,		Illegal,		Illegal,		Illegal,		JALR,			Illegal,		Illegal,
	//0x28
	IBEQ,			IBNE,			Illegal,		Illegal,		IBLTZ,			IBGTZ,			IBLEZ,			IBGEZ,
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
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		ILWR,
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
