#include <stddef.h>
#include "../MIPS.h"
#include "../MemoryUtils.h"
#include "MA_VU.h"
#include "Vpu.h"
#include "VUShared.h"
#include "offsetof_def.h"

#define LATENCY_EATAN 53
#define LATENCY_EEXP 43
#define LATENCY_ELENG 17
#define LATENCY_ERCPR 11
#define LATENCY_ERLENG 23
#define LATENCY_ERSADD 17
#define LATENCY_ERSQRT 17
#define LATENCY_ESADD 10
#define LATENCY_ESIN 28
#define LATENCY_ESQRT 11
#define LATENCY_ESUM 11

CMA_VU::CLower::CLower(uint32 vuMemAddressMask)
    : CMIPSInstructionFactory(MIPS_REGSIZE_32)
    , m_vuMemAddressMask(vuMemAddressMask)
{
}

void CMA_VU::CLower::CompileInstruction(uint32 address, CMipsJitter* codeGen, CMIPS* context, uint32 instrPosition)
{
	SetupQuickVariables(address, codeGen, context, instrPosition);

	if(IsLOI(context, address))
	{
		return;
	}

	m_nDest = (uint8)((m_nOpcode >> 21) & 0x000F);

	m_nFSF = ((m_nDest >> 0) & 0x03);
	m_nFTF = ((m_nDest >> 2) & 0x03);

	m_nIT = (uint8)((m_nOpcode >> 16) & 0x001F);
	m_nIS = (uint8)((m_nOpcode >> 11) & 0x001F);
	m_nID = (uint8)((m_nOpcode >> 6) & 0x001F);
	m_nImm5 = m_nID;
	m_nImm11 = (uint16)((m_nOpcode >> 0) & 0x07FF);
	m_nImm12 = (uint16)((m_nOpcode & 0x7FF) | (m_nOpcode & 0x00200000) >> 10);
	m_nImm15 = (uint16)((m_nOpcode & 0x7FF) | (m_nOpcode & 0x01E00000) >> 10);
	m_nImm15S = m_nImm15 | (m_nImm15 & 0x4000 ? 0x8000 : 0x0000);
	m_nImm24 = m_nOpcode & 0x00FFFFFF;

	if(m_nOpcode != OPCODE_NOP)
	{
		((this)->*(m_pOpGeneral[m_nOpcode >> 25]))();
	}
}

void CMA_VU::CLower::SetRelativePipeTime(uint32 relativePipeTime, uint32 compileHints)
{
	m_relativePipeTime = relativePipeTime;
	m_compileHints = compileHints;
}

void CMA_VU::CLower::SetBranchAddress(bool nCondition, int32 nOffset)
{
	m_codeGen->PushCst(0);
	m_codeGen->BeginIf(nCondition ? Jitter::CONDITION_NE : Jitter::CONDITION_EQ);
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nPC));
		m_codeGen->PushCst(m_instrPosition + nOffset + 4);
		m_codeGen->Add();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));
	}
	m_codeGen->EndIf();
}

bool CMA_VU::CLower::IsLOI(CMIPS* ctx, uint32 address)
{
	assert((address & 0x07) == 0);
	//Check for LOI bit
	uint32 upperInstruction = ctx->m_pMemoryMap->GetInstruction(address + 4);
	return (upperInstruction & 0x80000000) != 0;
}

void CMA_VU::CLower::ApplySumSeries(size_t target, const uint32* seriesConstants, const unsigned int* seriesExponents, unsigned int seriesLength)
{
	for(unsigned int i = 0; i < seriesLength; i++)
	{
		unsigned int exponent = seriesExponents[i];
		float constant = *reinterpret_cast<const float*>(&seriesConstants[i]);

		m_codeGen->FP_PushSingle(target);
		for(unsigned int j = 0; j < exponent - 1; j++)
		{
			m_codeGen->FP_PushSingle(target);
			m_codeGen->FP_Mul();
		}

		m_codeGen->FP_PushCst(constant);
		m_codeGen->FP_Mul();

		if(i != 0)
		{
			m_codeGen->FP_Add();
		}
	}
}

void CMA_VU::CLower::GenerateEATAN()
{
	auto destination = VUShared::g_pipeInfoP.heldValue;
	VUShared::QueueInPipeline(VUShared::g_pipeInfoP, m_codeGen, LATENCY_EATAN, m_relativePipeTime);

	static const uint32 pi4 = 0x3F490FDB;
	const unsigned int seriesLength = 8;
	static const uint32 seriesConstants[seriesLength] =
	    {
	        0x3F7FFFF5,
	        0xBEAAA61C,
	        0x3E4C40A6,
	        0xBE0E6C63,
	        0x3DC577DF,
	        0xBD6501C4,
	        0x3CB31652,
	        0xBB84D7E7,
	    };
	static const unsigned int seriesExponents[seriesLength] =
	    {
	        1,
	        3,
	        5,
	        7,
	        9,
	        11,
	        13,
	        15};

	ApplySumSeries(offsetof(CMIPS, m_State.nCOP2T),
	               seriesConstants, seriesExponents, seriesLength);

	{
		float constant = *reinterpret_cast<const float*>(&pi4);
		m_codeGen->FP_PushCst(constant);
		m_codeGen->FP_Add();
	}

	m_codeGen->FP_PullSingle(destination);
}

//////////////////////////////////////////////////
//General Instructions
//////////////////////////////////////////////////

//00
void CMA_VU::CLower::LQ()
{
	if(m_nDest == 0) return;

	m_codeGen->PushRelRef(offsetof(CMIPS, m_vuMem));
	VUShared::ComputeMemAccessAddr(
	    m_codeGen,
	    m_nIS,
	    static_cast<uint32>(VUShared::GetImm11Offset(m_nImm11)),
	    0,
	    m_vuMemAddressMask);

	VUShared::LQbase(m_codeGen, m_nDest, m_nIT);
}

//01
void CMA_VU::CLower::SQ()
{
	m_codeGen->PushRelRef(offsetof(CMIPS, m_vuMem));

	//Compute address
	VUShared::ComputeMemAccessAddr(
	    m_codeGen,
	    m_nIT,
	    static_cast<uint32>(VUShared::GetImm11Offset(m_nImm11)),
	    0,
	    m_vuMemAddressMask);

	VUShared::SQbase(m_codeGen, m_nDest, m_nIS);
}

//04
void CMA_VU::CLower::ILW()
{
	if((m_nIT & 0xF) == 0) return;

	m_codeGen->PushRelRef(offsetof(CMIPS, m_vuMem));

	VUShared::ComputeMemAccessAddr(
	    m_codeGen,
	    m_nIS,
	    static_cast<uint32>(VUShared::GetImm11Offset(m_nImm11)),
	    VUShared::GetDestOffset(m_nDest),
	    m_vuMemAddressMask);

	VUShared::ILWbase(m_codeGen, m_nIT);
}

//05
void CMA_VU::CLower::ISW()
{
	m_codeGen->PushRelRef(offsetof(CMIPS, m_vuMem));

	VUShared::ComputeMemAccessAddr(
	    m_codeGen,
	    m_nIS,
	    static_cast<uint32>(VUShared::GetImm11Offset(m_nImm11)),
	    0,
	    m_vuMemAddressMask);

	VUShared::ISWbase(m_codeGen, m_nDest, m_nIT);
}

//08
void CMA_VU::CLower::IADDIU()
{
	if((m_nIT & 0xF) == 0) return;

	VUShared::PushIntegerRegister(m_codeGen, m_nIS);
	m_codeGen->PushCst(static_cast<uint16>(m_nImm15));
	m_codeGen->Add();
	VUShared::PullIntegerRegister(m_codeGen, m_nIT);
}

//09
void CMA_VU::CLower::ISUBIU()
{
	if((m_nIT & 0xF) == 0) return;

	VUShared::PushIntegerRegister(m_codeGen, m_nIS);
	m_codeGen->PushCst(static_cast<uint16>(m_nImm15));
	m_codeGen->Sub();
	VUShared::PullIntegerRegister(m_codeGen, m_nIT);
}

//10
void CMA_VU::CLower::FCEQ()
{
	VUShared::CheckFlagPipeline(VUShared::g_pipeInfoClip, m_codeGen, m_relativePipeTime);

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2CF));
	m_codeGen->PushCst(0xFFFFFF);
	m_codeGen->And();

	m_codeGen->PushCst(m_nImm24);
	m_codeGen->Cmp(Jitter::CONDITION_EQ);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[1]));
}

//11
void CMA_VU::CLower::FCSET()
{
	m_codeGen->PushCst(m_nImm24);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2CF));

	m_codeGen->PushCst(m_nImm24);
	ResetFlagPipeline(VUShared::g_pipeInfoClip, m_codeGen);
}

//12
void CMA_VU::CLower::FCAND()
{
	VUShared::CheckFlagPipeline(VUShared::g_pipeInfoClip, m_codeGen, m_relativePipeTime);

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2CF));
	m_codeGen->PushCst(m_nImm24);
	m_codeGen->And();

	m_codeGen->PushCst(0);
	m_codeGen->Cmp(Jitter::CONDITION_NE);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[1]));
}

//13
void CMA_VU::CLower::FCOR()
{
	VUShared::CheckFlagPipeline(VUShared::g_pipeInfoClip, m_codeGen, m_relativePipeTime);

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2CF));
	m_codeGen->PushCst(m_nImm24);
	m_codeGen->Or();
	m_codeGen->PushCst(0xFFFFFF);
	m_codeGen->And();

	m_codeGen->PushCst(0xFFFFFF);
	m_codeGen->Cmp(Jitter::CONDITION_EQ);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[1]));
}

//15
void CMA_VU::CLower::FSSET()
{
	m_codeGen->PushCst(m_nImm12);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2T));
	VUShared::SetStatus(m_codeGen, offsetof(CMIPS, m_State.nCOP2T));
}

//16
void CMA_VU::CLower::FSAND()
{
	if((m_nIT & 0xF) == 0) return;

	VUShared::GetStatus(m_codeGen, offsetof(CMIPS, m_State.nCOP2VI[m_nIT & 0xF]), m_relativePipeTime);

	//Mask result
	VUShared::PushIntegerRegister(m_codeGen, m_nIT);
	m_codeGen->PushCst(m_nImm12);
	m_codeGen->And();
	VUShared::PullIntegerRegister(m_codeGen, m_nIT);
}

//17
void CMA_VU::CLower::FSOR()
{
	if((m_nIT & 0xF) == 0) return;

	VUShared::GetStatus(m_codeGen, offsetof(CMIPS, m_State.nCOP2VI[m_nIT & 0xF]), m_relativePipeTime);

	//Mask result
	VUShared::PushIntegerRegister(m_codeGen, m_nIT);
	m_codeGen->PushCst(m_nImm12);
	m_codeGen->Or();
	VUShared::PullIntegerRegister(m_codeGen, m_nIT);
}

//18
void CMA_VU::CLower::FMEQ()
{
	if((m_nIT & 0xF) == 0) return;

	VUShared::CheckFlagPipeline(VUShared::g_pipeInfoMac, m_codeGen, m_relativePipeTime);

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2MF));
	VUShared::PushIntegerRegister(m_codeGen, m_nIS);
	m_codeGen->Cmp(Jitter::CONDITION_EQ);
	VUShared::PullIntegerRegister(m_codeGen, m_nIT);
}

//1A
void CMA_VU::CLower::FMAND()
{
	if((m_nIT & 0xF) == 0) return;

	VUShared::CheckFlagPipeline(VUShared::g_pipeInfoMac, m_codeGen, m_relativePipeTime);

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2MF));
	VUShared::PushIntegerRegister(m_codeGen, m_nIS);
	m_codeGen->And();
	VUShared::PullIntegerRegister(m_codeGen, m_nIT);
}

//1B
void CMA_VU::CLower::FMOR()
{
	if((m_nIT & 0xF) == 0) return;

	VUShared::CheckFlagPipeline(VUShared::g_pipeInfoMac, m_codeGen, m_relativePipeTime);

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2MF));
	VUShared::PushIntegerRegister(m_codeGen, m_nIS);
	m_codeGen->Or();
	VUShared::PullIntegerRegister(m_codeGen, m_nIT);
}

//1C
void CMA_VU::CLower::FCGET()
{
	if((m_nIT & 0xF) == 0) return;

	VUShared::CheckFlagPipeline(VUShared::g_pipeInfoClip, m_codeGen, m_relativePipeTime);

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2CF));
	m_codeGen->PushCst(0xFFF);
	m_codeGen->And();
	VUShared::PullIntegerRegister(m_codeGen, m_nIT);
}

//20
void CMA_VU::CLower::B()
{
	m_codeGen->PushCst(1);
	SetBranchAddress(true, VUShared::GetBranch(m_nImm11) + 4);
}

//21
void CMA_VU::CLower::BAL()
{
	//Save PC
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nPC));
	m_codeGen->PushCst(m_instrPosition + 0x10);
	m_codeGen->Add();
	m_codeGen->Sra(3);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIT]));

	m_codeGen->PushCst(1);
	SetBranchAddress(true, VUShared::GetBranch(m_nImm11) + 4);
}

//24
void CMA_VU::CLower::JR()
{
	//Compute new PC
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIS]));
	m_codeGen->PushCst(0xFFFF);
	m_codeGen->And();
	m_codeGen->Shl(3);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));
}

//25
void CMA_VU::CLower::JALR()
{
	//Compute new PC
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIS]));
	m_codeGen->PushCst(0xFFFF);
	m_codeGen->And();
	m_codeGen->Shl(3);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));

	//Save PC
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nPC));
	m_codeGen->PushCst(m_instrPosition + 0x10);
	m_codeGen->Add();
	m_codeGen->Sra(3);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIT]));
}

//28
void CMA_VU::CLower::IBEQ()
{
	//Operand 1
	VUShared::PushIntegerRegister(m_codeGen, m_nIS);
	m_codeGen->PushCst(0xFFFF);
	m_codeGen->And();

	//Operand 2
	VUShared::PushIntegerRegister(m_codeGen, m_nIT);
	m_codeGen->PushCst(0xFFFF);
	m_codeGen->And();

	m_codeGen->Cmp(Jitter::CONDITION_EQ);

	SetBranchAddress(true, VUShared::GetBranch(m_nImm11) + 4);
}

//29
void CMA_VU::CLower::IBNE()
{
	//Operand 1
	VUShared::PushIntegerRegister(m_codeGen, m_nIS);
	m_codeGen->PushCst(0xFFFF);
	m_codeGen->And();

	//Operand 2
	VUShared::PushIntegerRegister(m_codeGen, m_nIT);
	m_codeGen->PushCst(0xFFFF);
	m_codeGen->And();

	m_codeGen->Cmp(Jitter::CONDITION_EQ);

	SetBranchAddress(false, VUShared::GetBranch(m_nImm11) + 4);
}

//2C
void CMA_VU::CLower::IBLTZ()
{
	//TODO: Merge IBLTZ and IBGEZ
	m_codeGen->PushCst(0);

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIS]));
	m_codeGen->PushCst(0x8000);
	m_codeGen->And();

	m_codeGen->Cmp(Jitter::CONDITION_EQ);

	SetBranchAddress(false, VUShared::GetBranch(m_nImm11) + 4);
}

//2D
void CMA_VU::CLower::IBGTZ()
{
	//TODO: Merge IBGTZ and IBLEZ
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIS]));
	m_codeGen->SignExt16();

	m_codeGen->PushCst(0);
	m_codeGen->Cmp(Jitter::CONDITION_GT);

	SetBranchAddress(true, VUShared::GetBranch(m_nImm11) + 4);
}

//2E
void CMA_VU::CLower::IBLEZ()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIS]));
	m_codeGen->SignExt16();

	m_codeGen->PushCst(0);
	m_codeGen->Cmp(Jitter::CONDITION_GT);

	SetBranchAddress(false, VUShared::GetBranch(m_nImm11) + 4);
}

//2F
void CMA_VU::CLower::IBGEZ()
{
	m_codeGen->PushCst(0);

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIS]));
	m_codeGen->PushCst(0x8000);
	m_codeGen->And();

	m_codeGen->Cmp(Jitter::CONDITION_EQ);

	SetBranchAddress(true, VUShared::GetBranch(m_nImm11) + 4);
}

//40
void CMA_VU::CLower::LOWEROP()
{
	((this)->*(m_pOpLower[m_nOpcode & 0x3F]))();
}

//////////////////////////////////////////////////
//LowerOp Instructions
//////////////////////////////////////////////////

//30
void CMA_VU::CLower::IADD()
{
	VUShared::IADD(m_codeGen, m_nID, m_nIS, m_nIT);
}

//31
void CMA_VU::CLower::ISUB()
{
	VUShared::ISUB(m_codeGen, m_nID, m_nIS, m_nIT);
}

//32
void CMA_VU::CLower::IADDI()
{
	VUShared::IADDI(m_codeGen, m_nIT, m_nIS, m_nImm5);
}

//34
void CMA_VU::CLower::IAND()
{
	VUShared::IAND(m_codeGen, m_nID, m_nIS, m_nIT);
}

//35
void CMA_VU::CLower::IOR()
{
	VUShared::IOR(m_codeGen, m_nID, m_nIS, m_nIT);
}

//3C
void CMA_VU::CLower::VECTOR0()
{
	((this)->*(m_pOpVector0[(m_nOpcode >> 6) & 0x1F]))();
}

//3D
void CMA_VU::CLower::VECTOR1()
{
	((this)->*(m_pOpVector1[(m_nOpcode >> 6) & 0x1F]))();
}

//3E
void CMA_VU::CLower::VECTOR2()
{
	((this)->*(m_pOpVector2[(m_nOpcode >> 6) & 0x1F]))();
}

//3F
void CMA_VU::CLower::VECTOR3()
{
	((this)->*(m_pOpVector3[(m_nOpcode >> 6) & 0x1F]))();
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
	VUShared::LQI(m_codeGen, m_nDest, m_nIT, m_nIS, m_vuMemAddressMask);
}

//0E
void CMA_VU::CLower::DIV()
{
	VUShared::DIV(m_codeGen, m_nIS, m_nFSF, m_nIT, m_nFTF, m_relativePipeTime);
}

//0F
void CMA_VU::CLower::MTIR()
{
	VUShared::MTIR(m_codeGen, m_nIT, m_nIS, m_nFSF);
}

//10
void CMA_VU::CLower::RNEXT()
{
	VUShared::RNEXT(m_codeGen, m_nDest, m_nIT);
}

//19
void CMA_VU::CLower::MFP()
{
	for(unsigned int i = 0; i < 4; i++)
	{
		if(!VUShared::DestinationHasElement(m_nDest, i)) continue;

		m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2P));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2[m_nIT].nV[i]));
	}
}

//1A
void CMA_VU::CLower::XTOP()
{
	//Push context
	m_codeGen->PushCtx();

	//Compute Address
	m_codeGen->PushCst(CVpu::VU_ADDR_TOP);

	m_codeGen->Call(reinterpret_cast<void*>(&MemoryUtils_GetWordProxy), 2, Jitter::CJitter::RETURN_VALUE_32);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIT]));
}

//1B
void CMA_VU::CLower::XGKICK()
{
	//Save XGKICK address to be used later
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nCOP2VI[m_nIS]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.xgkickAddress));
}

//1C
void CMA_VU::CLower::ESADD()
{
	auto destination = VUShared::g_pipeInfoP.heldValue;
	VUShared::QueueInPipeline(VUShared::g_pipeInfoP, m_codeGen, LATENCY_ESADD, m_relativePipeTime);

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

	m_codeGen->FP_PullSingle(destination);
}

//1D
void CMA_VU::CLower::EATANxy()
{
	//Compute t = (y - x) / (y + x)
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP2[m_nIS].nV[1]));
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP2[m_nIS].nV[0]));
	m_codeGen->FP_Sub();

	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP2[m_nIS].nV[1]));
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP2[m_nIS].nV[0]));
	m_codeGen->FP_Add();

	m_codeGen->FP_Div();
	m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP2T));

	GenerateEATAN();
}

//1E
void CMA_VU::CLower::ESQRT()
{
	auto destination = VUShared::g_pipeInfoP.heldValue;
	VUShared::QueueInPipeline(VUShared::g_pipeInfoP, m_codeGen, LATENCY_ESQRT, m_relativePipeTime);

	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP2[m_nIS].nV[m_nFSF]));
	m_codeGen->FP_Sqrt();
	m_codeGen->FP_PullSingle(destination);
}

//1F
void CMA_VU::CLower::ESIN()
{
	auto destination = VUShared::g_pipeInfoP.heldValue;
	VUShared::QueueInPipeline(VUShared::g_pipeInfoP, m_codeGen, LATENCY_ESIN, m_relativePipeTime);

	static const unsigned int seriesLength = 5;
	static const uint32 seriesConstants[seriesLength] =
	    {
	        0x3F800000,
	        0xBE2AAAA4,
	        0x3C08873E,
	        0xB94FB21F,
	        0x362E9C14};
	static const unsigned int seriesExponents[seriesLength] =
	    {
	        1,
	        3,
	        5,
	        7,
	        9};

	ApplySumSeries(offsetof(CMIPS, m_State.nCOP2[m_nIS].nV[m_nFSF]),
	               seriesConstants, seriesExponents, seriesLength);

	m_codeGen->FP_PullSingle(destination);
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
	VUShared::SQI(m_codeGen, m_nDest, m_nIS, m_nIT, m_vuMemAddressMask);
}

//0E
void CMA_VU::CLower::SQRT()
{
	VUShared::SQRT(m_codeGen, m_nIT, m_nFTF, m_relativePipeTime);
}

//0F
void CMA_VU::CLower::MFIR()
{
	VUShared::MFIR(m_codeGen, m_nDest, m_nIT, m_nIS);
}

//10
void CMA_VU::CLower::RGET()
{
	VUShared::RGET(m_codeGen, m_nDest, m_nIT);
}

//1A
void CMA_VU::CLower::XITOP()
{
	if((m_nIT & 0xF) == 0) return;

	//Push context
	m_codeGen->PushCtx();

	//Compute Address
	m_codeGen->PushCst(CVpu::VU_ADDR_ITOP);

	m_codeGen->Call(reinterpret_cast<void*>(&MemoryUtils_GetWordProxy), 2, Jitter::CJitter::RETURN_VALUE_32);
	VUShared::PullIntegerRegister(m_codeGen, m_nIT);
}

//1C
void CMA_VU::CLower::ERSADD()
{
	auto destination = VUShared::g_pipeInfoP.heldValue;
	VUShared::QueueInPipeline(VUShared::g_pipeInfoP, m_codeGen, LATENCY_ERSADD, m_relativePipeTime);

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
	//Inverse

	m_codeGen->FP_Rcpl();

	m_codeGen->FP_PullSingle(destination);
}

//1D
void CMA_VU::CLower::EATANxz()
{
	//Compute t = (z - x) / (z + x)
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP2[m_nIS].nV[2]));
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP2[m_nIS].nV[0]));
	m_codeGen->FP_Sub();

	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP2[m_nIS].nV[2]));
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP2[m_nIS].nV[0]));
	m_codeGen->FP_Add();

	m_codeGen->FP_Div();
	m_codeGen->FP_PullSingle(offsetof(CMIPS, m_State.nCOP2T));

	GenerateEATAN();
}

//1E
void CMA_VU::CLower::ERSQRT()
{
	auto destination = VUShared::g_pipeInfoP.heldValue;
	VUShared::QueueInPipeline(VUShared::g_pipeInfoP, m_codeGen, LATENCY_ERSQRT, m_relativePipeTime);

	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP2[m_nIS].nV[m_nFSF]));
	m_codeGen->FP_Rsqrt();
	m_codeGen->FP_PullSingle(destination);
}

//////////////////////////////////////////////////
//Vector2 Instructions
//////////////////////////////////////////////////

//0D
void CMA_VU::CLower::LQD()
{
	VUShared::LQD(m_codeGen, m_nDest, m_nIT, m_nIS, m_vuMemAddressMask);
}

//0E
void CMA_VU::CLower::RSQRT()
{
	VUShared::RSQRT(m_codeGen, m_nIS, m_nFSF, m_nIT, m_nFTF, m_relativePipeTime);
}

//0F
void CMA_VU::CLower::ILWR()
{
	VUShared::ILWR(m_codeGen, m_nDest, m_nIT, m_nIS, m_vuMemAddressMask);
}

//10
void CMA_VU::CLower::RINIT()
{
	VUShared::RINIT(m_codeGen, m_nIS, m_nFSF);
}

//1C
void CMA_VU::CLower::ELENG()
{
	auto destination = VUShared::g_pipeInfoP.heldValue;
	VUShared::QueueInPipeline(VUShared::g_pipeInfoP, m_codeGen, LATENCY_ELENG, m_relativePipeTime);

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
	//Extract root

	m_codeGen->FP_Sqrt();

	m_codeGen->FP_PullSingle(destination);
}

//1D
void CMA_VU::CLower::ESUM()
{
	auto destination = VUShared::g_pipeInfoP.heldValue;
	VUShared::QueueInPipeline(VUShared::g_pipeInfoP, m_codeGen, LATENCY_ESUM, m_relativePipeTime);

	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP2[m_nIS].nV[0]));
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP2[m_nIS].nV[1]));
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP2[m_nIS].nV[2]));
	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP2[m_nIS].nV[3]));

	m_codeGen->FP_Add();
	m_codeGen->FP_Add();
	m_codeGen->FP_Add();

	m_codeGen->FP_PullSingle(destination);
}

//1E
void CMA_VU::CLower::ERCPR()
{
	auto destination = VUShared::g_pipeInfoP.heldValue;
	VUShared::QueueInPipeline(VUShared::g_pipeInfoP, m_codeGen, LATENCY_ERCPR, m_relativePipeTime);

	m_codeGen->FP_PushSingle(offsetof(CMIPS, m_State.nCOP2[m_nIS].nV[m_nFSF]));
	m_codeGen->FP_Rcpl();
	m_codeGen->FP_PullSingle(destination);
}

//1F
void CMA_VU::CLower::EEXP()
{
	auto destination = VUShared::g_pipeInfoP.heldValue;
	VUShared::QueueInPipeline(VUShared::g_pipeInfoP, m_codeGen, LATENCY_EEXP, m_relativePipeTime);

	const unsigned int seriesLength = 6;
	static const uint32 seriesConstants[seriesLength] =
	    {
	        0x3E7FFFA8,
	        0x3D0007F4,
	        0x3B29D3FF,
	        0x3933E553,
	        0x36B63510,
	        0x353961AC};
	static const unsigned int seriesExponents[seriesLength] =
	    {
	        1,
	        2,
	        3,
	        4,
	        5,
	        6};

	ApplySumSeries(offsetof(CMIPS, m_State.nCOP2[m_nIS].nV[m_nFSF]),
	               seriesConstants, seriesExponents, seriesLength);

	m_codeGen->FP_PushCst(1.0f);
	m_codeGen->FP_Add();

	//Up to the fourth power
	m_codeGen->PushTop();
	m_codeGen->PushTop();
	m_codeGen->PushTop();
	m_codeGen->FP_Mul();
	m_codeGen->FP_Mul();
	m_codeGen->FP_Mul();

	m_codeGen->FP_Rcpl();

	m_codeGen->FP_PullSingle(destination);
}

//////////////////////////////////////////////////
//Vector3 Instructions
//////////////////////////////////////////////////

//0D
void CMA_VU::CLower::SQD()
{
	VUShared::SQD(m_codeGen, m_nDest, m_nIS, m_nIT, m_vuMemAddressMask);
}

//0E
void CMA_VU::CLower::WAITQ()
{
	VUShared::WAITQ(m_codeGen);
}

//0F
void CMA_VU::CLower::ISWR()
{
	VUShared::ISWR(m_codeGen, m_nDest, m_nIT, m_nIS, m_vuMemAddressMask);
}

//10
void CMA_VU::CLower::RXOR()
{
	VUShared::RXOR(m_codeGen, m_nIS, m_nFSF);
}

//1C
void CMA_VU::CLower::ERLENG()
{
	auto destination = VUShared::g_pipeInfoP.heldValue;
	VUShared::QueueInPipeline(VUShared::g_pipeInfoP, m_codeGen, LATENCY_ERLENG, m_relativePipeTime);

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

	m_codeGen->FP_PullSingle(destination);
}

//1E
void CMA_VU::CLower::WAITP()
{
	VUShared::WAITP(m_codeGen);
}

//////////////////////////////////////////////////
//Opcode Tables
//////////////////////////////////////////////////

// clang-format off
CMA_VU::CLower::InstructionFuncConstant CMA_VU::CLower::m_pOpGeneral[0x80] =
{
	//0x00
	&CMA_VU::CLower::LQ,			&CMA_VU::CLower::SQ,			&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::ILW,			&CMA_VU::CLower::ISW,			&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,
	//0x08
	&CMA_VU::CLower::IADDIU,		&CMA_VU::CLower::ISUBIU,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,
	//0x10
	&CMA_VU::CLower::FCEQ,			&CMA_VU::CLower::FCSET,			&CMA_VU::CLower::FCAND,			&CMA_VU::CLower::FCOR,			&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::FSSET,			&CMA_VU::CLower::FSAND,			&CMA_VU::CLower::FSOR,
	//0x18
	&CMA_VU::CLower::FMEQ,			&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::FMAND,			&CMA_VU::CLower::FMOR,			&CMA_VU::CLower::FCGET,			&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,
	//0x20
	&CMA_VU::CLower::B,				&CMA_VU::CLower::BAL,			&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::JR,			&CMA_VU::CLower::JALR,			&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,
	//0x28
	&CMA_VU::CLower::IBEQ,			&CMA_VU::CLower::IBNE,			&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::IBLTZ,			&CMA_VU::CLower::IBGTZ,			&CMA_VU::CLower::IBLEZ,			&CMA_VU::CLower::IBGEZ,
	//0x30
	&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,
	//0x38
	&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,
	//0x40
	&CMA_VU::CLower::LOWEROP,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,
	//0x48
	&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,
	//0x50
	&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,
	//0x58
	&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,
	//0x60
	&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,
	//0x68
	&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,
	//0x70
	&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,
	//0x78
	&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,
};

CMA_VU::CLower::InstructionFuncConstant CMA_VU::CLower::m_pOpLower[0x40] =
{
	//0x00
	&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,
	//0x08
	&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,
	//0x10
	&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,
	//0x18
	&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,
	//0x20
	&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,
	//0x28
	&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,
	//0x30
	&CMA_VU::CLower::IADD,			&CMA_VU::CLower::ISUB,			&CMA_VU::CLower::IADDI,			&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::IAND,			&CMA_VU::CLower::IOR,			&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,
	//0x38
	&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::VECTOR0,		&CMA_VU::CLower::VECTOR1,		&CMA_VU::CLower::VECTOR2,		&CMA_VU::CLower::VECTOR3,
};

CMA_VU::CLower::InstructionFuncConstant CMA_VU::CLower::m_pOpVector0[0x20] =
{
	//0x00
	&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,
	//0x08
	&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::MOVE,			&CMA_VU::CLower::LQI,			&CMA_VU::CLower::DIV,			&CMA_VU::CLower::MTIR,
	//0x10
	&CMA_VU::CLower::RNEXT,			&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,
	//0x18
	&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::MFP,			&CMA_VU::CLower::XTOP,			&CMA_VU::CLower::XGKICK,		&CMA_VU::CLower::ESADD,			&CMA_VU::CLower::EATANxy,		&CMA_VU::CLower::ESQRT, 		&CMA_VU::CLower::ESIN,
};

CMA_VU::CLower::InstructionFuncConstant CMA_VU::CLower::m_pOpVector1[0x20] =
{
	//0x00
	&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,
	//0x08
	&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::MR32,			&CMA_VU::CLower::SQI,			&CMA_VU::CLower::SQRT,			&CMA_VU::CLower::MFIR,
	//0x10
	&CMA_VU::CLower::RGET,			&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,
	//0x18
	&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::XITOP,			&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::ERSADD,		&CMA_VU::CLower::EATANxz,		&CMA_VU::CLower::ERSQRT,		&CMA_VU::CLower::Illegal,
};

CMA_VU::CLower::InstructionFuncConstant CMA_VU::CLower::m_pOpVector2[0x20] =
{
	//0x00
	&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,
	//0x08
	&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::LQD,			&CMA_VU::CLower::RSQRT,			&CMA_VU::CLower::ILWR,
	//0x10
	&CMA_VU::CLower::RINIT,			&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,
	//0x18
	&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::ELENG,			&CMA_VU::CLower::ESUM,			&CMA_VU::CLower::ERCPR,			&CMA_VU::CLower::EEXP,
};

CMA_VU::CLower::InstructionFuncConstant CMA_VU::CLower::m_pOpVector3[0x20] =
{
	//0x00
	&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,
	//0x08
	&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::SQD,			&CMA_VU::CLower::WAITQ,			&CMA_VU::CLower::ISWR,
	//0x10
	&CMA_VU::CLower::RXOR,			&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,
	//0x18
	&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::ERLENG,		&CMA_VU::CLower::Illegal,		&CMA_VU::CLower::WAITP,			&CMA_VU::CLower::Illegal,
};
// clang-format on
