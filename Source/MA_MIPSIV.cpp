#include <stddef.h>
#include "MA_MIPSIV.h"
#include "MIPS.h"
#include "CodeGen.h"
#include "MemoryUtils.h"
#include "COP_SCU.h"
#include "offsetof_def.h"
#include "Integer64.h"

using namespace std::tr1;

CMA_MIPSIV			g_MAMIPSIV(MIPS_REGSIZE_64);

uint8				CMA_MIPSIV::m_nRS;
uint8				CMA_MIPSIV::m_nRT;
uint8				CMA_MIPSIV::m_nRD;
uint8				CMA_MIPSIV::m_nSA;
uint16				CMA_MIPSIV::m_nImmediate;

uint32 g_LWMaskRight[4] =
{
    0x00FFFFFF,
    0x0000FFFF,
    0x000000FF,
    0x00000000,
};

uint32 g_LWMaskLeft[4] =
{
    0xFFFFFF00,
    0xFFFF0000,
    0xFF000000,
    0x00000000,
};

uint64 g_LDMaskRight[8] =
{
    0x00FFFFFFFFFFFFFFULL,
    0x0000FFFFFFFFFFFFULL,
    0x000000FFFFFFFFFFULL,
    0x00000000FFFFFFFFULL,
    0x0000000000FFFFFFULL,
    0x000000000000FFFFULL,
    0x00000000000000FFULL,
    0x0000000000000000ULL,
};

uint64 g_LDMaskLeft[8] =
{
    0xFFFFFFFFFFFFFF00ULL,
    0xFFFFFFFFFFFF0000ULL,
    0xFFFFFFFFFF000000ULL,
    0xFFFFFFFF00000000ULL,
    0xFFFFFF0000000000ULL,
    0xFFFF000000000000ULL,
    0xFF00000000000000ULL,
    0x0000000000000000ULL,
};

void LWL_Proxy(uint32 address, uint32 rt, CMIPS* context)
{
    uint32 alignedAddress = address & ~0x03;
    uint32 byteOffset = address & 0x03;
    uint32 accessType = 3 - byteOffset;
    uint32 memory = CMemoryUtils::GetWordProxy(context, alignedAddress);
    memory <<= accessType * 8;
    context->m_State.nGPR[rt].nV0 &= g_LWMaskRight[byteOffset];
    context->m_State.nGPR[rt].nV0 |= memory;
    context->m_State.nGPR[rt].nV1 = context->m_State.nGPR[rt].nV0 & 0x80000000 ? 0xFFFFFFFF : 0x00000000;
}

void LWR_Proxy(uint32 address, uint32 rt, CMIPS* context)
{
    uint32 alignedAddress = address & ~0x03;
    uint32 byteOffset = address & 0x03;
    uint32 accessType = 3 - byteOffset;
    uint32 memory = CMemoryUtils::GetWordProxy(context, alignedAddress);
    memory >>= byteOffset * 8;
    context->m_State.nGPR[rt].nV0 &= g_LWMaskLeft[accessType];
    context->m_State.nGPR[rt].nV0 |= memory;
    context->m_State.nGPR[rt].nV1 = context->m_State.nGPR[rt].nV0 & 0x80000000 ? 0xFFFFFFFF : 0x00000000;
}

void LDL_Proxy(uint32 address, uint32 rt, CMIPS* context)
{
    uint32 alignedAddress = address & ~0x07;
    uint32 byteOffset = address & 0x07;
    uint32 accessType = 7 - byteOffset;
    INTEGER64 memory;
    memory.d0 = CMemoryUtils::GetWordProxy(context, alignedAddress + 0);
    memory.d1 = CMemoryUtils::GetWordProxy(context, alignedAddress + 4);
    memory.q <<= accessType * 8;
    context->m_State.nGPR[rt].nD0 &= g_LDMaskRight[byteOffset];
    context->m_State.nGPR[rt].nD0 |= memory.q;
}

void LDR_Proxy(uint32 address, uint32 rt, CMIPS* context)
{
    uint32 alignedAddress = address & ~0x07;
    uint32 byteOffset = address & 0x07;
    uint32 accessType = 7 - byteOffset;
    INTEGER64 memory;
    memory.d0 = CMemoryUtils::GetWordProxy(context, alignedAddress + 0);
    memory.d1 = CMemoryUtils::GetWordProxy(context, alignedAddress + 4);
    memory.q >>= byteOffset * 8;
    context->m_State.nGPR[rt].nD0 &= g_LDMaskLeft[accessType];
    context->m_State.nGPR[rt].nD0 |= memory.q;
}

void SWL_Proxy(uint32 address, uint32 rt, CMIPS* context)
{
    uint32 alignedAddress = address & ~0x03;
    uint32 byteOffset = address & 0x03;
    uint32 accessType = 3 - byteOffset;
    uint32 reg = context->m_State.nGPR[rt].nV0;
    reg >>= accessType * 8;
    uint32 memory = CMemoryUtils::GetWordProxy(context, alignedAddress);
    memory &= g_LWMaskLeft[byteOffset];
    memory |= reg;
    CMemoryUtils::SetWordProxy(context, memory, alignedAddress);
}

void SWR_Proxy(uint32 address, uint32 rt, CMIPS* context)
{
    uint32 alignedAddress = address & ~0x03;
    uint32 byteOffset = address & 0x03;
    uint32 accessType = 3 - byteOffset;
    uint32 reg = context->m_State.nGPR[rt].nV0;
    reg <<= byteOffset * 8;
    uint32 memory = CMemoryUtils::GetWordProxy(context, alignedAddress);
    memory &= g_LWMaskRight[accessType];
    memory |= reg;
    CMemoryUtils::SetWordProxy(context, memory, alignedAddress);
}

void SDL_Proxy(uint32 address, uint32 rt, CMIPS* context)
{
    uint32 alignedAddress = address & ~0x07;
    uint32 byteOffset = address & 0x07;
    uint32 accessType = 7 - byteOffset;
    uint64 reg = context->m_State.nGPR[rt].nD0;
    reg >>= accessType * 8;
    INTEGER64 memory;
    memory.d0 = CMemoryUtils::GetWordProxy(context, alignedAddress + 0);
    memory.d1 = CMemoryUtils::GetWordProxy(context, alignedAddress + 4);
    memory.q &= g_LDMaskLeft[byteOffset];
    memory.q |= reg;
    CMemoryUtils::SetWordProxy(context, memory.d0, alignedAddress + 0);
    CMemoryUtils::SetWordProxy(context, memory.d1, alignedAddress + 4);
}

void SDR_Proxy(uint32 address, uint32 rt, CMIPS* context)
{
    uint32 alignedAddress = address & ~0x07;
    uint32 byteOffset = address & 0x07;
    uint32 accessType = 7 - byteOffset;
    uint64 reg = context->m_State.nGPR[rt].nD0;
    reg <<= byteOffset * 8;
    INTEGER64 memory;
    memory.d0 = CMemoryUtils::GetWordProxy(context, alignedAddress + 0);
    memory.d1 = CMemoryUtils::GetWordProxy(context, alignedAddress + 4);
    memory.q &= g_LDMaskRight[accessType];
    memory.q |= reg;
    CMemoryUtils::SetWordProxy(context, memory.d0, alignedAddress + 0);
    CMemoryUtils::SetWordProxy(context, memory.d1, alignedAddress + 4);
}

CMA_MIPSIV::CMA_MIPSIV(MIPS_REGSIZE nRegSize) :
CMIPSArchitecture(nRegSize)
{
	SetupReflectionTables();
}

void CMA_MIPSIV::CompileInstruction(uint32 nAddress, CCodeGen* codeGen, CMIPS* pCtx, bool nParent)
{
	if(nParent)
	{
		SetupQuickVariables(nAddress, codeGen, pCtx);
	}

	m_nRS			= (uint8)((m_nOpcode >> 21) & 0x1F);
	m_nRT			= (uint8)((m_nOpcode >> 16) & 0x1F);
	m_nRD			= (uint8)((m_nOpcode >> 11) & 0x1F);
	m_nSA			= (uint8)((m_nOpcode >> 6) & 0x1F);
	m_nImmediate	= (uint16)(m_nOpcode & 0xFFFF);

	if(m_nOpcode)
	{
		m_pOpGeneral[(m_nOpcode >> 26)]();
	}
}

void CMA_MIPSIV::SPECIAL()
{
	m_pOpSpecial[m_nImmediate & 0x3F]();
}

void CMA_MIPSIV::SPECIAL2()
{
	m_pOpSpecial2[m_nImmediate & 0x3F]();
}

void CMA_MIPSIV::REGIMM()
{
	m_pOpRegImm[m_nRT]();
}

//////////////////////////////////////////////////
//General Opcodes
//////////////////////////////////////////////////

//02
void CMA_MIPSIV::J()
{
    m_codeGen->PushCst((m_nAddress & 0xF0000000) | ((m_nOpcode & 0x03FFFFFF) << 2));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));
}

//03
void CMA_MIPSIV::JAL()
{
	//64-bit addresses?

	//Save the address in RA
    m_codeGen->PushCst(m_nAddress + 8);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[31].nV[0]));

	//Update jump address
    m_codeGen->PushCst((m_nAddress & 0xF0000000) | ((m_nOpcode & 0x03FFFFFF) << 2));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));
}

//04
void CMA_MIPSIV::BEQ()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

	m_codeGen->Cmp64(CCodeGen::CONDITION_EQ);

	Branch(true);
}

//05
void CMA_MIPSIV::BNE()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

	m_codeGen->Cmp64(CCodeGen::CONDITION_EQ);

	Branch(false);
}

//06
void CMA_MIPSIV::BLEZ()
{
    //Less/Equal & Not Likely
    Template_BranchLez()(true, false);
}

//07
void CMA_MIPSIV::BGTZ()
{
    //Not Less/Equal & Not Likely
    Template_BranchLez()(false, false);
}

//08
void CMA_MIPSIV::ADDI()
{
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
    m_codeGen->PushCst(static_cast<int16>(m_nImmediate));
    m_codeGen->Add();
    m_codeGen->SeX();
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
}

//09
void CMA_MIPSIV::ADDIU()
{
/*
    if(m_nRT == 0) 
    {
        //Hack: PS2 IOP uses ADDIU R0, R0, $x for dynamic linking
        CCodeGen::PushRel(offsetof(CMIPS, m_State.nPC));
        CCodeGen::PushCst(4);
        CCodeGen::Sub();
        CCodeGen::PullRel(offsetof(CMIPS, m_State.nCOP0[CCOP_SCU::EPC]));

        CCodeGen::PushCst(1);
        CCodeGen::PullRel(offsetof(CMIPS, m_State.nHasException));
    }
*/
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
    m_codeGen->PushCst(static_cast<int16>(m_nImmediate));
    m_codeGen->Add();
    m_codeGen->SeX();
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
}

//0A
void CMA_MIPSIV::SLTI()
{
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));

    m_codeGen->PushCst(static_cast<int16>(m_nImmediate));
    m_codeGen->PushCst(m_nImmediate & 0x8000 ? 0xFFFFFFFF : 0x00000000);

    m_codeGen->Cmp64(CCodeGen::CONDITION_LT);

	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));

	//Clear higher 32-bits
	m_codeGen->PushCst(0);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
}

//0B
void CMA_MIPSIV::SLTIU()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));

	m_codeGen->PushCst(static_cast<int16>(m_nImmediate));
	m_codeGen->PushCst(m_nImmediate & 0x8000 ? 0xFFFFFFFF : 0x00000000);

	m_codeGen->Cmp64(CCodeGen::CONDITION_BL);

	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));

	//Clear higher 32-bits
	m_codeGen->PushCst(0);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
}

//0C
void CMA_MIPSIV::ANDI()
{
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
    m_codeGen->PushCst(m_nImmediate);
    
    m_codeGen->And();
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));

    m_codeGen->PushCst(0);
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
}

//0D
void CMA_MIPSIV::ORI()
{
	//Lower 32-bits
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushCst(m_nImmediate);
	m_codeGen->Or();
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));

	//Higher 32-bits (only if registers are different)
	if(m_nRS != m_nRT)
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
	}
}

//0E
void CMA_MIPSIV::XORI()
{
	//Lower 32-bits
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushCst(m_nImmediate);
	m_codeGen->Xor();
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));

	//Higher 32-bits
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
}

//0F
void CMA_MIPSIV::LUI()
{
	m_codeGen->PushCst(m_nImmediate << 16);
	m_codeGen->PushCst((m_nImmediate & 0x8000) ? 0xFFFFFFFF : 0x00000000);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
}

//10
void CMA_MIPSIV::COP0()
{
	if(m_pCtx->m_pCOP[0] != NULL)
	{
		m_pCtx->m_pCOP[0]->CompileInstruction(m_nAddress, m_codeGen, m_pCtx, false);
	}
	else
	{
		Illegal();
	}
}

//11
void CMA_MIPSIV::COP1()
{
	if(m_pCtx->m_pCOP[1] != NULL)
	{
		m_pCtx->m_pCOP[1]->CompileInstruction(m_nAddress, m_codeGen, m_pCtx, false);
	}
	else
	{
		Illegal();
	}
}

//12
void CMA_MIPSIV::COP2()
{
	if(m_pCtx->m_pCOP[2] != NULL)
	{
		m_pCtx->m_pCOP[2]->CompileInstruction(m_nAddress, m_codeGen, m_pCtx, false);
	}
	else
	{
		Illegal();
	}
}

//14
void CMA_MIPSIV::BEQL()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

	m_codeGen->Cmp64(CCodeGen::CONDITION_EQ);

	BranchLikely(true);
}

//15
void CMA_MIPSIV::BNEL()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

	m_codeGen->Cmp64(CCodeGen::CONDITION_EQ);

	BranchLikely(false);
}

//16
void CMA_MIPSIV::BLEZL()
{
    //Less/Equal & Likely
    Template_BranchLez()(true, true);
}

//17
void CMA_MIPSIV::BGTZL()
{
    //Not Less/Equal & Likely
    Template_BranchLez()(false, true);
}

//19
void CMA_MIPSIV::DADDIU()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));

	m_codeGen->PushCst(static_cast<int16>(m_nImmediate));
	m_codeGen->PushCst((m_nImmediate & 0x8000) == 0 ? 0x00000000 : 0xFFFFFFFF);

	m_codeGen->Add64();

	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
}

//1A
void CMA_MIPSIV::LDL()
{
    ComputeMemAccessAddr();
    m_codeGen->PushCst(m_nRT);
    m_codeGen->PushRef(m_pCtx);
    m_codeGen->Call(reinterpret_cast<void*>(&LDL_Proxy), 3, false);
}

//1B
void CMA_MIPSIV::LDR()
{
    ComputeMemAccessAddr();
    m_codeGen->PushCst(m_nRT);
    m_codeGen->PushRef(m_pCtx);
    m_codeGen->Call(reinterpret_cast<void*>(&LDR_Proxy), 3, false);
}

//20
void CMA_MIPSIV::LB()
{
    ComputeMemAccessAddr();

	m_codeGen->PushRef(m_pCtx);
	m_codeGen->PushIdx(1);
	m_codeGen->Call(reinterpret_cast<void*>(&CMemoryUtils::GetByteProxy), 2, true);

    m_codeGen->SeX8();
	m_codeGen->SeX();
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));

    m_codeGen->PullTop();
}

//21
void CMA_MIPSIV::LH()
{
    ComputeMemAccessAddr();

	m_codeGen->PushRef(m_pCtx);
	m_codeGen->PushIdx(1);
	m_codeGen->Call(reinterpret_cast<void*>(&CMemoryUtils::GetHalfProxy), 2, true);

    m_codeGen->SeX16();
	m_codeGen->SeX();
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));

    m_codeGen->PullTop();
}

//22
void CMA_MIPSIV::LWL()
{
    ComputeMemAccessAddr();
    m_codeGen->PushCst(m_nRT);
    m_codeGen->PushRef(m_pCtx);
    m_codeGen->Call(reinterpret_cast<void*>(&LWL_Proxy), 3, false);
}

//23
void CMA_MIPSIV::LW()
{
    Template_LoadUnsigned32()(reinterpret_cast<void*>(&CMemoryUtils::GetWordProxy));
}

//24
void CMA_MIPSIV::LBU()
{
    Template_LoadUnsigned32()(reinterpret_cast<void*>(&CMemoryUtils::GetByteProxy));
}

//25
void CMA_MIPSIV::LHU()
{
    Template_LoadUnsigned32()(reinterpret_cast<void*>(&CMemoryUtils::GetHalfProxy));
}

//26
void CMA_MIPSIV::LWR()
{
    ComputeMemAccessAddr();
    m_codeGen->PushCst(m_nRT);
    m_codeGen->PushRef(m_pCtx);
    m_codeGen->Call(reinterpret_cast<void*>(&LWR_Proxy), 3, false);
}

//27
void CMA_MIPSIV::LWU()
{
    ComputeMemAccessAddr();

	m_codeGen->PushRef(m_pCtx);
	m_codeGen->PushIdx(1);
	m_codeGen->Call(reinterpret_cast<void*>(&CMemoryUtils::GetWordProxy), 2, true);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));

    m_codeGen->PushCst(0);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

    m_codeGen->PullTop();
}

//28
void CMA_MIPSIV::SB()
{
	ComputeMemAccessAddr();

	m_codeGen->PushRef(m_pCtx);
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->PushIdx(2);
	m_codeGen->Call(reinterpret_cast<void*>(&CMemoryUtils::SetByteProxy), 3, false);

	m_codeGen->PullTop();
}

//29
void CMA_MIPSIV::SH()
{
	ComputeMemAccessAddr();

	m_codeGen->PushRef(m_pCtx);
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->PushIdx(2);
	m_codeGen->Call(reinterpret_cast<void*>(&CMemoryUtils::SetHalfProxy), 3, false);

	m_codeGen->PullTop();
}

//2A
void CMA_MIPSIV::SWL()
{
    ComputeMemAccessAddr();
    m_codeGen->PushCst(m_nRT);
    m_codeGen->PushRef(m_pCtx);
    m_codeGen->Call(reinterpret_cast<void*>(&SWL_Proxy), 3, false);
}

//2B
void CMA_MIPSIV::SW()
{
	ComputeMemAccessAddr();

	m_codeGen->PushRef(m_pCtx);
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->PushIdx(2);
	m_codeGen->Call(reinterpret_cast<void*>(&CMemoryUtils::SetWordProxy), 3, false);

	m_codeGen->PullTop();
}

//2C
void CMA_MIPSIV::SDL()
{
    ComputeMemAccessAddr();
    m_codeGen->PushCst(m_nRT);
    m_codeGen->PushRef(m_pCtx);
    m_codeGen->Call(reinterpret_cast<void*>(&SDL_Proxy), 3, false);
}

//2D
void CMA_MIPSIV::SDR()
{
    ComputeMemAccessAddr();
    m_codeGen->PushCst(m_nRT);
    m_codeGen->PushRef(m_pCtx);
    m_codeGen->Call(reinterpret_cast<void*>(&SDR_Proxy), 3, false);
}

//2E
void CMA_MIPSIV::SWR()
{
    ComputeMemAccessAddr();
    m_codeGen->PushCst(m_nRT);
    m_codeGen->PushRef(m_pCtx);
    m_codeGen->Call(reinterpret_cast<void*>(&SWR_Proxy), 3, false);
}

//2F
void CMA_MIPSIV::CACHE()
{
	//No cache in our system. Nothing to do.
}

//31
void CMA_MIPSIV::LWC1()
{
	if(m_pCtx->m_pCOP[1] != NULL)
	{
		m_pCtx->m_pCOP[1]->CompileInstruction(m_nAddress, m_codeGen, m_pCtx, false);
	}
	else
	{
		Illegal();
	}
}

//36
void CMA_MIPSIV::LDC2()
{
	if(m_pCtx->m_pCOP[2] != NULL)
	{
		m_pCtx->m_pCOP[2]->CompileInstruction(m_nAddress, m_codeGen, m_pCtx, false);
	}
	else
	{
		Illegal();
	}
}

//37
void CMA_MIPSIV::LD()
{
    ComputeMemAccessAddr();

    for(unsigned int i = 0; i < 2; i++)
    {
	    m_codeGen->PushRef(m_pCtx);
	    m_codeGen->PushIdx(1);
	    m_codeGen->Call(reinterpret_cast<void*>(&CMemoryUtils::GetWordProxy), 2, true);

        m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[i]));

        if(i != 1)
        {
            m_codeGen->PushCst(4);
            m_codeGen->Add();
        }
    }

    m_codeGen->PullTop();
}

//39
void CMA_MIPSIV::SWC1()
{
	if(m_pCtx->m_pCOP[1] != NULL)
	{
		m_pCtx->m_pCOP[1]->CompileInstruction(m_nAddress, m_codeGen, m_pCtx, false);
	}
	else
	{
		Illegal();
	}
}

//3E
void CMA_MIPSIV::SDC2()
{
	if(m_pCtx->m_pCOP[2] != NULL)
	{
		m_pCtx->m_pCOP[2]->CompileInstruction(m_nAddress, m_codeGen, m_pCtx, false);
	}
	else
	{
		Illegal();
	}
}

//3F
void CMA_MIPSIV::SD()
{
	ComputeMemAccessAddr();

	for(unsigned int i = 0; i < 2; i++)
	{
		m_codeGen->PushRef(m_pCtx);
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[i]));
		m_codeGen->PushIdx(2);
		m_codeGen->Call(reinterpret_cast<void*>(&CMemoryUtils::SetWordProxy), 3, false);

		if(i != 1)
		{
			m_codeGen->PushCst(4);
			m_codeGen->Add();
		}
	}

	m_codeGen->PullTop();
}

//////////////////////////////////////////////////
//Special Opcodes
//////////////////////////////////////////////////

//00
void CMA_MIPSIV::SLL()
{
    Template_ShiftCst32()(bind(&CCodeGen::Shl, m_codeGen, _1));
}

//02
void CMA_MIPSIV::SRL()
{
    Template_ShiftCst32()(bind(&CCodeGen::Srl, m_codeGen, _1));
}

//03
void CMA_MIPSIV::SRA()
{
    Template_ShiftCst32()(bind(&CCodeGen::Sra, m_codeGen, _1));
}

//04
void CMA_MIPSIV::SLLV()
{
    Template_ShiftVar32()(bind(&CCodeGen::Shl, m_codeGen));
}

//06
void CMA_MIPSIV::SRLV()
{
    Template_ShiftVar32()(bind(&CCodeGen::Srl, m_codeGen));
}

//07
void CMA_MIPSIV::SRAV()
{
    Template_ShiftVar32()(bind(&CCodeGen::Sra, m_codeGen));
}

//08
void CMA_MIPSIV::JR()
{
    //TODO: 64-bits addresses
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));
}

//09
void CMA_MIPSIV::JALR()
{
	//TODO: 64-bits addresses

	//Set the jump address
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));

	//Save address in register
    m_codeGen->PushCst(m_nAddress + 8);
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

//0A
void CMA_MIPSIV::MOVZ()
{
    Template_MovEqual()(true);
}

//0B
void CMA_MIPSIV::MOVN()
{
    Template_MovEqual()(false);
}

//0C
void CMA_MIPSIV::SYSCALL()
{
//    m_codeGen->PushRel(offsetof(CMIPS, m_State.nPC));
//    m_codeGen->PushCst(4);
//    m_codeGen->Sub();
    m_codeGen->PushCst(m_nAddress);
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP0[CCOP_SCU::EPC]));
    m_codeGen->PushCst(1);
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nHasException));
}

//0D
void CMA_MIPSIV::BREAK()
{
    //NOP
}

//0F
void CMA_MIPSIV::SYNC()
{
	//NOP
}

//10
void CMA_MIPSIV::MFHI()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nHI[0]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nHI[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
}

//11
void CMA_MIPSIV::MTHI()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nHI[0]));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nHI[1]));
}

//12
void CMA_MIPSIV::MFLO()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nLO[0]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nLO[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
}

//13
void CMA_MIPSIV::MTLO()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nLO[0]));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nLO[1]));
}

//14
void CMA_MIPSIV::DSLLV()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));

	m_codeGen->Shl64();

	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

//16
void CMA_MIPSIV::DSRLV()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));

	m_codeGen->Srl64();

	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

//18
void CMA_MIPSIV::MULT()
{
    Template_Mult32()(bind(&CCodeGen::MultS, m_codeGen), 0);
}

//19
void CMA_MIPSIV::MULTU()
{
    Template_Mult32()(bind(&CCodeGen::Mult, m_codeGen), 0);
}

//1A
void CMA_MIPSIV::DIV()
{
    Template_Div32()(bind(&CCodeGen::DivS, m_codeGen), 0);
}

//1B
void CMA_MIPSIV::DIVU()
{
    Template_Div32()(bind(&CCodeGen::Div, m_codeGen), 0);
}

//20
void CMA_MIPSIV::ADD()
{
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));

    m_codeGen->Add();
    m_codeGen->SeX();

    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

//21
void CMA_MIPSIV::ADDU()
{
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));

    m_codeGen->Add();
    m_codeGen->SeX();

    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

//22
void CMA_MIPSIV::SUB()
{
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));

    m_codeGen->Sub();
    m_codeGen->SeX();

    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

//23
void CMA_MIPSIV::SUBU()
{
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));

    m_codeGen->Sub();
    m_codeGen->SeX();

    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

//24
void CMA_MIPSIV::AND()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

	m_codeGen->And64();

	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

//25
void CMA_MIPSIV::OR()
{
    //TODO: Use a 64-bits op
    for(unsigned int i = 0; i < 2; i++)
    {
        m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[i]));
        m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[i]));

        m_codeGen->Or();

        m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[i]));
    }
}

//26
void CMA_MIPSIV::XOR()
{
    for(unsigned int i = 0; i < 2; i++)
    {
        m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[i]));
        m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[i]));

        m_codeGen->Xor();

        m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[i]));
    }
}

//27
void CMA_MIPSIV::NOR()
{
    for(unsigned int i = 0; i < 2; i++)
    {
        m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[i]));
        m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[i]));

        m_codeGen->Or();
        m_codeGen->Not();

        m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[i]));
    }
}

//2A
void CMA_MIPSIV::SLT()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

	m_codeGen->Cmp64(CCodeGen::CONDITION_LT);

	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));

	//Clear higher 32-bits
	m_codeGen->PushCst(0);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
}

//2B
void CMA_MIPSIV::SLTU()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

	m_codeGen->Cmp64(CCodeGen::CONDITION_BL);

	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));

	//Clear higher 32-bits
	m_codeGen->PushCst(0);
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
}

//2D
void CMA_MIPSIV::DADDU()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

	m_codeGen->Add64();

	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

//2F
void CMA_MIPSIV::DSUBU()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

	m_codeGen->Sub64();

	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

//38
void CMA_MIPSIV::DSLL()
{
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

    m_codeGen->Shl64(m_nSA);

    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

//3A
void CMA_MIPSIV::DSRL()
{
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

    m_codeGen->Srl64(m_nSA);

    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

//3B
void CMA_MIPSIV::DSRA()
{
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
    m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

    m_codeGen->Sra64(m_nSA);

    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
    m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

//3C
void CMA_MIPSIV::DSLL32()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

	m_codeGen->Shl64(m_nSA + 0x20);

	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

//3E
void CMA_MIPSIV::DSRL32()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

	m_codeGen->Srl64(m_nSA + 32);

	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

//3F
void CMA_MIPSIV::DSRA32()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));

	m_codeGen->Sra64(m_nSA + 32);

	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[1]));
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

//////////////////////////////////////////////////
//Special2 Opcodes
//////////////////////////////////////////////////

//////////////////////////////////////////////////
//RegImm Opcodes
//////////////////////////////////////////////////

//00
void CMA_MIPSIV::BLTZ()
{
    //Not greater/equal & not likely
    Template_BranchGez()(false, false);
}

//01
void CMA_MIPSIV::BGEZ()
{
    //Greater/equal & not likely
    Template_BranchGez()(true, false);
}

//02
void CMA_MIPSIV::BLTZL()
{
    //Not greater/equal & likely
    Template_BranchGez()(false, true);
}

//03
void CMA_MIPSIV::BGEZL()
{
    //Greater/equal & likely
    Template_BranchGez()(true, true);
}

//////////////////////////////////////////////////
//Opcode Tables
//////////////////////////////////////////////////

void (*CMA_MIPSIV::m_pOpGeneral[0x40])() =
{
	//0x00
	SPECIAL,		REGIMM,			J,				JAL,			BEQ,			BNE,			BLEZ,			BGTZ,
	//0x08
	ADDI,			ADDIU,			SLTI,			SLTIU,			ANDI,			ORI,			XORI,			LUI,
	//0x10
	COP0,			COP1,			COP2,			Illegal,		BEQL,			BNEL,			BLEZL,			BGTZL,
	//0x18
	Illegal,		DADDIU,			LDL,			LDR,			SPECIAL2,		Illegal,		Illegal,		Illegal,
	//0x20
	LB,				LH,				LWL,			LW,				LBU,			LHU,			LWR,			LWU,
	//0x28
	SB,				SH,				SWL,			SW,				SDL,			SDR,			SWR,			CACHE,
	//0x30
	Illegal,		LWC1,			Illegal,		Illegal,		Illegal,		Illegal,		LDC2,			LD,
	//0x38
	Illegal,		SWC1,			Illegal,		Illegal,		Illegal,		Illegal,		SDC2,			SD,
};

void (*CMA_MIPSIV::m_pOpSpecial[0x40])() = 
{
	//0x00
	SLL,			Illegal,		SRL,			SRA,			SLLV,			Illegal,		SRLV,			SRAV,
	//0x08
	JR,				JALR,			MOVZ,			MOVN,			SYSCALL,		BREAK,	    	Illegal,		SYNC,
	//0x10
	MFHI,			MTHI,			MFLO,			MTLO,			DSLLV,			Illegal,		DSRLV,			Illegal,
	//0x18
	MULT,			MULTU,			DIV,			DIVU,			Illegal,		Illegal,		Illegal,		Illegal,
	//0x20
	ADD,			ADDU,			SUB,	    	SUBU,			AND,			OR,				XOR,			NOR,
	//0x28
	Illegal,		Illegal,		SLT,			SLTU,			Illegal,		DADDU,			Illegal,		DSUBU,
	//0x30
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x38
	DSLL,			Illegal,		DSRL,			DSRA,			DSLL32,			Illegal,		DSRL32,			DSRA32,
};

void (*CMA_MIPSIV::m_pOpSpecial2[0x40])() = 
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
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x38
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
};

void (*CMA_MIPSIV::m_pOpRegImm[0x20])() = 
{
	//0x00
	BLTZ,			BGEZ,			BLTZL,			BGEZL,			Illegal,		Illegal,		Illegal,		Illegal,
	//0x08
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x10
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
	//0x18
	Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,		Illegal,
};
