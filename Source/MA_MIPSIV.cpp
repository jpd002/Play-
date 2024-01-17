#include <stddef.h>
#include "MA_MIPSIV.h"
#include "MIPS.h"
#include "Jitter.h"
#include "MemoryUtils.h"
#include "COP_SCU.h"
#include "offsetof_def.h"
#include "placeholder_def.h"

// clang-format off
const CMA_MIPSIV::MemoryAccessIdxTraits CMA_MIPSIV::g_byteAccessIdxTraits =
{
	reinterpret_cast<void*>(&MemoryUtils_GetByteProxy),
	reinterpret_cast<void*>(&MemoryUtils_SetByteProxy),
	&CMipsJitter::Load8FromRefIdx,
	&CMipsJitter::Store8AtRefIdx,
	&CMipsJitter::SignExt8,
	1,
	true,
};

const CMA_MIPSIV::MemoryAccessIdxTraits CMA_MIPSIV::g_ubyteAccessIdxTraits =
{
	reinterpret_cast<void*>(&MemoryUtils_GetByteProxy),
	reinterpret_cast<void*>(&MemoryUtils_SetByteProxy),
	&CMipsJitter::Load8FromRefIdx,
	&CMipsJitter::Store8AtRefIdx,
	nullptr,
	1,
	false,
};

const CMA_MIPSIV::MemoryAccessIdxTraits CMA_MIPSIV::g_halfAccessIdxTraits =
{
	reinterpret_cast<void*>(&MemoryUtils_GetHalfProxy),
	reinterpret_cast<void*>(&MemoryUtils_SetHalfProxy),
	&CMipsJitter::Load16FromRefIdx,
	&CMipsJitter::Store16AtRefIdx,
	&CMipsJitter::SignExt16,
	2,
	true,
};

const CMA_MIPSIV::MemoryAccessIdxTraits CMA_MIPSIV::g_uhalfAccessIdxTraits =
{
	reinterpret_cast<void*>(&MemoryUtils_GetHalfProxy),
	reinterpret_cast<void*>(&MemoryUtils_SetHalfProxy),
	&CMipsJitter::Load16FromRefIdx,
	&CMipsJitter::Store16AtRefIdx,
	nullptr,
	2,
	false,
};

const CMA_MIPSIV::MemoryAccessIdxTraits CMA_MIPSIV::g_wordAccessIdxTraits =
{
	reinterpret_cast<void*>(&MemoryUtils_GetWordProxy),
	reinterpret_cast<void*>(&MemoryUtils_SetWordProxy),
	&CMipsJitter::LoadFromRefIdx,
	&CMipsJitter::StoreAtRefIdx,
	nullptr,
	4,
	true,
};

const CMA_MIPSIV::MemoryAccessIdxTraits CMA_MIPSIV::g_uwordAccessIdxTraits =
{
	reinterpret_cast<void*>(&MemoryUtils_GetWordProxy),
	reinterpret_cast<void*>(&MemoryUtils_SetWordProxy),
	&CMipsJitter::LoadFromRefIdx,
	&CMipsJitter::StoreAtRefIdx,
	nullptr,
	4,
	false,
};

static const uint32 g_LWMaskRight[4] =
{
    0x00FFFFFF,
    0x0000FFFF,
    0x000000FF,
    0x00000000,
};

static const uint32 g_LWMaskLeft[4] =
{
    0xFFFFFF00,
    0xFFFF0000,
    0xFF000000,
    0x00000000,
};

static const uint64 g_LDMaskRight[8] =
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

static const uint64 g_LDMaskLeft[8] =
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
// clang-format on

extern "C" uint32 LWL_Proxy(uint32 address, uint32 rt, CMIPS* context)
{
	uint32 alignedAddress = address & ~0x03;
	uint32 byteOffset = address & 0x03;
	uint32 accessType = 3 - byteOffset;
	uint32 memory = MemoryUtils_GetWordProxy(context, alignedAddress);
	memory <<= accessType * 8;
	rt &= g_LWMaskRight[byteOffset];
	rt |= memory;
	return rt;
}

extern "C" uint32 LWR_Proxy(uint32 address, uint32 rt, CMIPS* context)
{
	uint32 alignedAddress = address & ~0x03;
	uint32 byteOffset = address & 0x03;
	uint32 accessType = 3 - byteOffset;
	uint32 memory = MemoryUtils_GetWordProxy(context, alignedAddress);
	memory >>= byteOffset * 8;
	rt &= g_LWMaskLeft[accessType];
	rt |= memory;
	return rt;
}

extern "C" uint64 LDL_Proxy(uint32 address, uint64 rt, CMIPS* context)
{
	uint32 alignedAddress = address & ~0x07;
	uint32 byteOffset = address & 0x07;
	uint32 accessType = 7 - byteOffset;
	uint64 memory = MemoryUtils_GetDoubleProxy(context, alignedAddress);
	memory <<= accessType * 8;
	rt &= g_LDMaskRight[byteOffset];
	rt |= memory;
	return rt;
}

extern "C" uint64 LDR_Proxy(uint32 address, uint64 rt, CMIPS* context)
{
	uint32 alignedAddress = address & ~0x07;
	uint32 byteOffset = address & 0x07;
	uint32 accessType = 7 - byteOffset;
	uint64 memory = MemoryUtils_GetDoubleProxy(context, alignedAddress);
	memory >>= byteOffset * 8;
	rt &= g_LDMaskLeft[accessType];
	rt |= memory;
	return rt;
}

extern "C" void SWL_Proxy(uint32 address, uint32 rt, CMIPS* context)
{
	uint32 alignedAddress = address & ~0x03;
	uint32 byteOffset = address & 0x03;
	uint32 accessType = 3 - byteOffset;
	rt >>= accessType * 8;
	uint32 memory = MemoryUtils_GetWordProxy(context, alignedAddress);
	memory &= g_LWMaskLeft[byteOffset];
	memory |= rt;
	MemoryUtils_SetWordProxy(context, memory, alignedAddress);
}

extern "C" void SWR_Proxy(uint32 address, uint32 rt, CMIPS* context)
{
	uint32 alignedAddress = address & ~0x03;
	uint32 byteOffset = address & 0x03;
	uint32 accessType = 3 - byteOffset;
	rt <<= byteOffset * 8;
	uint32 memory = MemoryUtils_GetWordProxy(context, alignedAddress);
	memory &= g_LWMaskRight[accessType];
	memory |= rt;
	MemoryUtils_SetWordProxy(context, memory, alignedAddress);
}

extern "C" void SDL_Proxy(uint32 address, uint64 rt, CMIPS* context)
{
	uint32 alignedAddress = address & ~0x07;
	uint32 byteOffset = address & 0x07;
	uint32 accessType = 7 - byteOffset;
	rt >>= accessType * 8;
	uint64 memory = MemoryUtils_GetDoubleProxy(context, alignedAddress);
	memory &= g_LDMaskLeft[byteOffset];
	memory |= rt;
	MemoryUtils_SetDoubleProxy(context, memory, alignedAddress);
}

extern "C" void SDR_Proxy(uint32 address, uint64 rt, CMIPS* context)
{
	uint32 alignedAddress = address & ~0x07;
	uint32 byteOffset = address & 0x07;
	uint32 accessType = 7 - byteOffset;
	rt <<= byteOffset * 8;
	uint64 memory = MemoryUtils_GetDoubleProxy(context, alignedAddress);
	memory &= g_LDMaskRight[accessType];
	memory |= rt;
	MemoryUtils_SetDoubleProxy(context, memory, alignedAddress);
}

CMA_MIPSIV::CMA_MIPSIV(MIPS_REGSIZE nRegSize)
    : CMIPSArchitecture(nRegSize)
{
	SetupInstructionTables();
	SetupReflectionTables();
}

void CMA_MIPSIV::SetupInstructionTables()
{
	for(unsigned int i = 0; i < MAX_GENERAL_OPS; i++)
	{
		m_pOpGeneral[i] = std::bind(m_cOpGeneral[i], this);
	}

	for(unsigned int i = 0; i < MAX_SPECIAL_OPS; i++)
	{
		m_pOpSpecial[i] = std::bind(m_cOpSpecial[i], this);
	}

	for(unsigned int i = 0; i < MAX_SPECIAL2_OPS; i++)
	{
		m_pOpSpecial2[i] = std::bind(&CMA_MIPSIV::Illegal, this);
	}

	for(unsigned int i = 0; i < MAX_REGIMM_OPS; i++)
	{
		m_pOpRegImm[i] = std::bind(m_cOpRegImm[i], this);
	}
}

bool CMA_MIPSIV::Ensure64BitRegs()
{
	if(m_regSize != MIPS_REGSIZE_64)
	{
		Illegal();
		return false;
	}
	return true;
}

void CMA_MIPSIV::CompileInstruction(uint32 address, CMipsJitter* codeGen, CMIPS* ctx, uint32 instrPosition)
{
	SetupQuickVariables(address, codeGen, ctx, instrPosition);

	m_nRS = (uint8)((m_nOpcode >> 21) & 0x1F);
	m_nRT = (uint8)((m_nOpcode >> 16) & 0x1F);
	m_nRD = (uint8)((m_nOpcode >> 11) & 0x1F);
	m_nSA = (uint8)((m_nOpcode >> 6) & 0x1F);
	m_nImmediate = (uint16)(m_nOpcode & 0xFFFF);

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
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nPC));
	m_codeGen->PushCst(m_instrPosition);
	m_codeGen->Add();
	m_codeGen->PushCst(0xF0000000);
	m_codeGen->And();
	m_codeGen->PushCst((m_nOpcode & 0x03FFFFFF) << 2);
	m_codeGen->Or();

	m_codeGen->PullRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));
}

//03
void CMA_MIPSIV::JAL()
{
	//64-bit addresses?

	//Save the address in RA
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nPC));
	m_codeGen->PushCst(m_instrPosition + 8);
	m_codeGen->Add();
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[31].nV[0]));

	//Update jump address
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nPC));
	m_codeGen->PushCst(m_instrPosition);
	m_codeGen->Add();
	m_codeGen->PushCst(0xF0000000);
	m_codeGen->And();
	m_codeGen->PushCst((m_nOpcode & 0x03FFFFFF) << 2);
	m_codeGen->Or();
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nDelayedJumpAddr));
}

//04
void CMA_MIPSIV::BEQ()
{
	Template_BranchEq(true, false);
}

//05
void CMA_MIPSIV::BNE()
{
	Template_BranchEq(false, false);
}

//06
void CMA_MIPSIV::BLEZ()
{
	//Less/Equal & Not Likely
	Template_BranchLez(true, false);
}

//07
void CMA_MIPSIV::BGTZ()
{
	//Not Less/Equal & Not Likely
	Template_BranchLez(false, false);
}

//08
void CMA_MIPSIV::ADDI()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushCst(static_cast<int16>(m_nImmediate));
	m_codeGen->Add();
	if(m_regSize == MIPS_REGSIZE_64)
	{
		m_codeGen->PushTop();
		m_codeGen->SignExt();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
	}
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
}

//09
void CMA_MIPSIV::ADDIU()
{
	if(m_nRT == 0 && m_nRS == 0)
	{
		//Hack: PS2 IOP uses ADDIU R0, R0, $x for dynamic linking
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nPC));
		m_codeGen->PushCst(m_instrPosition);
		m_codeGen->Add();

		m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP0[CCOP_SCU::EPC]));

		m_codeGen->PushCst(MIPS_EXCEPTION_SYSCALL);
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nHasException));
	}
	else
	{
		if(m_nRT == 0) return;

		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		m_codeGen->PushCst(static_cast<int16>(m_nImmediate));
		m_codeGen->Add();
		if(m_regSize == MIPS_REGSIZE_64)
		{
			m_codeGen->PushTop();
			m_codeGen->SignExt();
			m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
		}
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	}
}

//0A
void CMA_MIPSIV::SLTI()
{
	Template_SetLessThanImm(true);
}

//0B
void CMA_MIPSIV::SLTIU()
{
	Template_SetLessThanImm(false);
}

//0C
void CMA_MIPSIV::ANDI()
{
	if(m_nRT == 0) return;

	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushCst(m_nImmediate);

	m_codeGen->And();
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));

	if(m_regSize == MIPS_REGSIZE_64)
	{
		m_codeGen->PushCst(0);
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
	}
}

//0D
void CMA_MIPSIV::ORI()
{
	if(m_nRT == 0) return;

	//Lower 32-bits
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushCst(m_nImmediate);
	m_codeGen->Or();
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));

	//Higher 32-bits (only if registers are different)
	if((m_regSize == MIPS_REGSIZE_64) && (m_nRS != m_nRT))
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[1]));
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
	}
}

//0E
void CMA_MIPSIV::XORI()
{
	if(m_nRT == 0) return;

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
	if(m_nRT == 0) return;

	m_codeGen->PushCst(m_nImmediate << 16);
	if(m_regSize == MIPS_REGSIZE_64)
	{
		m_codeGen->PushCst((m_nImmediate & 0x8000) ? 0xFFFFFFFF : 0x00000000);
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
	}
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
}

//10
void CMA_MIPSIV::COP0()
{
	if(m_pCtx->m_pCOP[0] != NULL)
	{
		m_pCtx->m_pCOP[0]->CompileInstruction(m_nAddress, m_codeGen, m_pCtx, m_instrPosition);
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
		m_pCtx->m_pCOP[1]->CompileInstruction(m_nAddress, m_codeGen, m_pCtx, m_instrPosition);
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
		m_pCtx->m_pCOP[2]->CompileInstruction(m_nAddress, m_codeGen, m_pCtx, m_instrPosition);
	}
	else
	{
		Illegal();
	}
}

//14
void CMA_MIPSIV::BEQL()
{
	Template_BranchEq(true, true);
}

//15
void CMA_MIPSIV::BNEL()
{
	Template_BranchEq(false, true);
}

//16
void CMA_MIPSIV::BLEZL()
{
	//Less/Equal & Likely
	Template_BranchLez(true, true);
}

//17
void CMA_MIPSIV::BGTZL()
{
	//Not Less/Equal & Likely
	Template_BranchLez(false, true);
}

//18
void CMA_MIPSIV::DADDI()
{
	//TODO: Check for overflow
	DADDIU();
}

//19
void CMA_MIPSIV::DADDIU()
{
	if(!Ensure64BitRegs()) return;
	if(m_nRT == 0) return;

	m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushCst64(static_cast<int16>(m_nImmediate));
	m_codeGen->Add64();
	m_codeGen->PullRel64(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
}

//1A
void CMA_MIPSIV::LDL()
{
	if(!Ensure64BitRegs()) return;
	if(m_nRT == 0) return;

	ComputeMemAccessAddrNoXlat();
	m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRT]));
	m_codeGen->PushCtx();
	m_codeGen->Call(reinterpret_cast<void*>(&LDL_Proxy), 3, Jitter::CJitter::RETURN_VALUE_64);
	m_codeGen->PullRel64(offsetof(CMIPS, m_State.nGPR[m_nRT]));
}

//1B
void CMA_MIPSIV::LDR()
{
	if(!Ensure64BitRegs()) return;
	if(m_nRT == 0) return;

	ComputeMemAccessAddrNoXlat();
	m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRT]));
	m_codeGen->PushCtx();
	m_codeGen->Call(reinterpret_cast<void*>(&LDR_Proxy), 3, Jitter::CJitter::RETURN_VALUE_64);
	m_codeGen->PullRel64(offsetof(CMIPS, m_State.nGPR[m_nRT]));
}

//20
void CMA_MIPSIV::LB()
{
	Template_Load32Idx(g_byteAccessIdxTraits);
}

//21
void CMA_MIPSIV::LH()
{
	Template_Load32Idx(g_halfAccessIdxTraits);
}

//22
void CMA_MIPSIV::LWL()
{
	CheckTLBExceptions(false);

	if(m_nRT == 0) return;

	ComputeMemAccessAddrNoXlat();
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->PushCtx();
	m_codeGen->Call(reinterpret_cast<void*>(&LWL_Proxy), 3, Jitter::CJitter::RETURN_VALUE_32);

	if(m_regSize == MIPS_REGSIZE_64)
	{
		m_codeGen->PushTop();
		m_codeGen->SignExt();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
	}
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
}

//23
void CMA_MIPSIV::LW()
{
	Template_Load32Idx(g_wordAccessIdxTraits);
}

//24
void CMA_MIPSIV::LBU()
{
	Template_Load32Idx(g_ubyteAccessIdxTraits);
}

//25
void CMA_MIPSIV::LHU()
{
	Template_Load32Idx(g_uhalfAccessIdxTraits);
}

//26
void CMA_MIPSIV::LWR()
{
	CheckTLBExceptions(false);

	if(m_nRT == 0) return;

	ComputeMemAccessAddrNoXlat();
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->PushCtx();
	m_codeGen->Call(reinterpret_cast<void*>(&LWR_Proxy), 3, Jitter::CJitter::RETURN_VALUE_32);

	if(m_regSize == MIPS_REGSIZE_64)
	{
		m_codeGen->PushTop();
		m_codeGen->SignExt();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[1]));
	}
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
}

//27
void CMA_MIPSIV::LWU()
{
	Template_Load32Idx(g_uwordAccessIdxTraits);
}

//28
void CMA_MIPSIV::SB()
{
	Template_Store32Idx(g_byteAccessIdxTraits);
}

//29
void CMA_MIPSIV::SH()
{
	Template_Store32Idx(g_halfAccessIdxTraits);
}

//2A
void CMA_MIPSIV::SWL()
{
	CheckTLBExceptions(true);
	ComputeMemAccessAddrNoXlat();
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->PushCtx();
	m_codeGen->Call(reinterpret_cast<void*>(&SWL_Proxy), 3, Jitter::CJitter::RETURN_VALUE_NONE);
}

//2B
void CMA_MIPSIV::SW()
{
	Template_Store32Idx(g_wordAccessIdxTraits);
}

//2C
void CMA_MIPSIV::SDL()
{
	if(!Ensure64BitRegs()) return;

	ComputeMemAccessAddrNoXlat();
	m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRT]));
	m_codeGen->PushCtx();
	m_codeGen->Call(reinterpret_cast<void*>(&SDL_Proxy), 3, Jitter::CJitter::RETURN_VALUE_NONE);
}

//2D
void CMA_MIPSIV::SDR()
{
	if(!Ensure64BitRegs()) return;

	ComputeMemAccessAddrNoXlat();
	m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRT]));
	m_codeGen->PushCtx();
	m_codeGen->Call(reinterpret_cast<void*>(&SDR_Proxy), 3, Jitter::CJitter::RETURN_VALUE_NONE);
}

//2E
void CMA_MIPSIV::SWR()
{
	CheckTLBExceptions(true);
	ComputeMemAccessAddrNoXlat();
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->PushCtx();
	m_codeGen->Call(reinterpret_cast<void*>(&SWR_Proxy), 3, Jitter::CJitter::RETURN_VALUE_NONE);
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
		m_pCtx->m_pCOP[1]->CompileInstruction(m_nAddress, m_codeGen, m_pCtx, m_instrPosition);
	}
	else
	{
		Illegal();
	}
}

//33
void CMA_MIPSIV::PREF()
{
	//Nothing to do
}

//36
void CMA_MIPSIV::LDC2()
{
	if(m_pCtx->m_pCOP[2] != NULL)
	{
		m_pCtx->m_pCOP[2]->CompileInstruction(m_nAddress, m_codeGen, m_pCtx, m_instrPosition);
	}
	else
	{
		Illegal();
	}
}

//37
void CMA_MIPSIV::LD()
{
	if(!Ensure64BitRegs()) return;
	if(m_nRT == 0) return;

	ComputeMemAccessPageRef();

	m_codeGen->PushCst(0);
	m_codeGen->BeginIf(Jitter::CONDITION_NE);
	{
		ComputeMemAccessRefIdx(8);

		m_codeGen->Load64FromRefIdx(1);
		m_codeGen->PullRel64(offsetof(CMIPS, m_State.nGPR[m_nRT]));
	}
	m_codeGen->Else();
	{
		ComputeMemAccessAddrNoXlat();

		m_codeGen->PushCtx();
		m_codeGen->PushIdx(1);
		m_codeGen->Call(reinterpret_cast<void*>(&MemoryUtils_GetDoubleProxy), 2, Jitter::CJitter::RETURN_VALUE_64);
		m_codeGen->PullRel64(offsetof(CMIPS, m_State.nGPR[m_nRT]));

		m_codeGen->PullTop();
	}
	m_codeGen->EndIf();
}

//39
void CMA_MIPSIV::SWC1()
{
	if(m_pCtx->m_pCOP[1] != NULL)
	{
		m_pCtx->m_pCOP[1]->CompileInstruction(m_nAddress, m_codeGen, m_pCtx, m_instrPosition);
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
		m_pCtx->m_pCOP[2]->CompileInstruction(m_nAddress, m_codeGen, m_pCtx, m_instrPosition);
	}
	else
	{
		Illegal();
	}
}

//3F
void CMA_MIPSIV::SD()
{
	if(!Ensure64BitRegs()) return;

	ComputeMemAccessPageRef();

	m_codeGen->PushCst(0);
	m_codeGen->BeginIf(Jitter::CONDITION_NE);
	{
		ComputeMemAccessRefIdx(8);

		m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRT]));
		m_codeGen->Store64AtRefIdx(1);
	}
	m_codeGen->Else();
	{
		ComputeMemAccessAddrNoXlat();

		m_codeGen->PushCtx();
		m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRT]));
		m_codeGen->PushIdx(2);
		m_codeGen->Call(reinterpret_cast<void*>(&MemoryUtils_SetDoubleProxy), 3, Jitter::CJitter::RETURN_VALUE_NONE);

		m_codeGen->PullTop();
	}
	m_codeGen->EndIf();
}

//////////////////////////////////////////////////
//Special Opcodes
//////////////////////////////////////////////////

//00
void CMA_MIPSIV::SLL()
{
	void (Jitter::CJitter::*shiftFunction)(uint8) = &Jitter::CJitter::Shl;
	Template_ShiftCst32(std::bind(shiftFunction, m_codeGen, std::placeholders::_1));
}

//02
void CMA_MIPSIV::SRL()
{
	void (Jitter::CJitter::*shiftFunction)(uint8) = &Jitter::CJitter::Srl;
	Template_ShiftCst32(std::bind(shiftFunction, m_codeGen, std::placeholders::_1));
}

//03
void CMA_MIPSIV::SRA()
{
	void (Jitter::CJitter::*shiftFunction)(uint8) = &Jitter::CJitter::Sra;
	Template_ShiftCst32(std::bind(shiftFunction, m_codeGen, std::placeholders::_1));
}

//04
void CMA_MIPSIV::SLLV()
{
	void (Jitter::CJitter::*shiftFunction)() = &Jitter::CJitter::Shl;
	Template_ShiftVar32(std::bind(shiftFunction, m_codeGen));
}

//06
void CMA_MIPSIV::SRLV()
{
	void (Jitter::CJitter::*shiftFunction)() = &Jitter::CJitter::Srl;
	Template_ShiftVar32(std::bind(shiftFunction, m_codeGen));
}

//07
void CMA_MIPSIV::SRAV()
{
	void (Jitter::CJitter::*shiftFunction)() = &Jitter::CJitter::Sra;
	Template_ShiftVar32(std::bind(shiftFunction, m_codeGen));
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

	if(m_nRD != 0)
	{
		//Save address in register
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nPC));
		m_codeGen->PushCst(m_instrPosition + 8);
		m_codeGen->Add();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
	}
}

//0A
void CMA_MIPSIV::MOVZ()
{
	Template_MovEqual(true);
}

//0B
void CMA_MIPSIV::MOVN()
{
	Template_MovEqual(false);
}

//0C
void CMA_MIPSIV::SYSCALL()
{
	//Save current EPC
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nPC));
	m_codeGen->PushCst(m_instrPosition);
	m_codeGen->Add();

	m_codeGen->PullRel(offsetof(CMIPS, m_State.nCOP0[CCOP_SCU::EPC]));
	m_codeGen->PushCst(MIPS_EXCEPTION_SYSCALL);
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
	if(m_nRD == 0) return;

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
	if(m_nRD == 0) return;

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
	if(!Ensure64BitRegs()) return;
	if(m_nRD == 0) return;

	m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->Shl64();
	m_codeGen->PullRel64(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

//16
void CMA_MIPSIV::DSRLV()
{
	if(!Ensure64BitRegs()) return;
	if(m_nRD == 0) return;

	m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->Srl64();
	m_codeGen->PullRel64(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

//17
void CMA_MIPSIV::DSRAV()
{
	if(!Ensure64BitRegs()) return;
	if(m_nRD == 0) return;

	m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->Sra64();
	m_codeGen->PullRel64(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

//18
void CMA_MIPSIV::MULT()
{
	Template_Mult32(true, 0);
}

//19
void CMA_MIPSIV::MULTU()
{
	Template_Mult32(false, 0);
}

//1A
void CMA_MIPSIV::DIV()
{
	Template_Div32(true, 0);
}

//1B
void CMA_MIPSIV::DIVU()
{
	Template_Div32(false, 0);
}

//20
void CMA_MIPSIV::ADD()
{
	Template_Add32(true);
}

//21
void CMA_MIPSIV::ADDU()
{
	Template_Add32(false);
}

//22
void CMA_MIPSIV::SUB()
{
	Template_Sub32(true);
}

//23
void CMA_MIPSIV::SUBU()
{
	Template_Sub32(false);
}

//24
void CMA_MIPSIV::AND()
{
	if(m_nRD == 0) return;

	if(m_regSize == MIPS_REGSIZE_32)
	{
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		m_codeGen->PushRel(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
		m_codeGen->And();
		m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
	}
	else
	{
		m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
		m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
		m_codeGen->And64();
		m_codeGen->PullRel64(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
	}
}

//25
void CMA_MIPSIV::OR()
{
	if(m_nRD == 0) return;

	//TODO: Use a 64-bits op
	unsigned int regCount = (m_regSize == MIPS_REGSIZE_64) ? 2 : 1;
	for(unsigned int i = 0; i < regCount; i++)
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
	if(m_nRD == 0) return;

	unsigned int regCount = (m_regSize == MIPS_REGSIZE_64) ? 2 : 1;
	for(unsigned int i = 0; i < regCount; i++)
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
	if(m_nRD == 0) return;

	unsigned int regCount = (m_regSize == MIPS_REGSIZE_64) ? 2 : 1;
	for(unsigned int i = 0; i < regCount; i++)
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
	Template_SetLessThanReg(true);
}

//2B
void CMA_MIPSIV::SLTU()
{
	Template_SetLessThanReg(false);
}

//2C
void CMA_MIPSIV::DADD()
{
	Template_Add64(true);
}

//2D
void CMA_MIPSIV::DADDU()
{
	Template_Add64(false);
}

//2E
void CMA_MIPSIV::DSUB()
{
	Template_Sub64(true);
}

//2F
void CMA_MIPSIV::DSUBU()
{
	Template_Sub64(false);
}

//34
void CMA_MIPSIV::TEQ()
{
#ifdef _DEBUG
	m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->Cmp64(Jitter::CONDITION_EQ);
	CheckTrap();
#endif
}

//38
void CMA_MIPSIV::DSLL()
{
	if(!Ensure64BitRegs()) return;
	if(m_nRD == 0) return;

	m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->Shl64(m_nSA);
	m_codeGen->PullRel64(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

//3A
void CMA_MIPSIV::DSRL()
{
	if(!Ensure64BitRegs()) return;
	if(m_nRD == 0) return;

	m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->Srl64(m_nSA);
	m_codeGen->PullRel64(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

//3B
void CMA_MIPSIV::DSRA()
{
	if(!Ensure64BitRegs()) return;
	if(m_nRD == 0) return;

	m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->Sra64(m_nSA);
	m_codeGen->PullRel64(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

//3C
void CMA_MIPSIV::DSLL32()
{
	if(!Ensure64BitRegs()) return;
	if(m_nRD == 0) return;

	m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->Shl64(m_nSA + 32);
	m_codeGen->PullRel64(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

//3E
void CMA_MIPSIV::DSRL32()
{
	if(!Ensure64BitRegs()) return;
	if(m_nRD == 0) return;

	m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->Srl64(m_nSA + 32);
	m_codeGen->PullRel64(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

//3F
void CMA_MIPSIV::DSRA32()
{
	if(!Ensure64BitRegs()) return;
	if(m_nRD == 0) return;

	m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRT].nV[0]));
	m_codeGen->Sra64(m_nSA + 32);
	m_codeGen->PullRel64(offsetof(CMIPS, m_State.nGPR[m_nRD].nV[0]));
}

//////////////////////////////////////////////////
//RegImm Opcodes
//////////////////////////////////////////////////

//00
void CMA_MIPSIV::BLTZ()
{
	//Not greater/equal & not likely
	Template_BranchGez(false, false);
}

//01
void CMA_MIPSIV::BGEZ()
{
	//Greater/equal & not likely
	Template_BranchGez(true, false);
}

//02
void CMA_MIPSIV::BLTZL()
{
	//Not greater/equal & likely
	Template_BranchGez(false, true);
}

//03
void CMA_MIPSIV::BGEZL()
{
	//Greater/equal & likely
	Template_BranchGez(true, true);
}

//0C
void CMA_MIPSIV::TEQI()
{
#ifdef _DEBUG
	m_codeGen->PushRel64(offsetof(CMIPS, m_State.nGPR[m_nRS].nV[0]));
	m_codeGen->PushCst64(static_cast<int16>(m_nImmediate));
	m_codeGen->Cmp64(Jitter::CONDITION_EQ);
	CheckTrap();
#endif
}

//10
void CMA_MIPSIV::BLTZAL()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nPC));
	m_codeGen->PushCst(m_instrPosition + 8);
	m_codeGen->Add();
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[CMIPS::RA].nV[0]));

	BLTZ();
}

//11
void CMA_MIPSIV::BGEZAL()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nPC));
	m_codeGen->PushCst(m_instrPosition + 8);
	m_codeGen->Add();
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[CMIPS::RA].nV[0]));

	BGEZ();
}

//12
void CMA_MIPSIV::BLTZALL()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nPC));
	m_codeGen->PushCst(m_instrPosition + 8);
	m_codeGen->Add();
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[CMIPS::RA].nV[0]));

	BLTZL();
}

//13
void CMA_MIPSIV::BGEZALL()
{
	m_codeGen->PushRel(offsetof(CMIPS, m_State.nPC));
	m_codeGen->PushCst(m_instrPosition + 8);
	m_codeGen->Add();
	m_codeGen->PullRel(offsetof(CMIPS, m_State.nGPR[CMIPS::RA].nV[0]));

	BGEZL();
}

//////////////////////////////////////////////////
//Opcode Tables
//////////////////////////////////////////////////

// clang-format off
CMA_MIPSIV::InstructionFuncConstant CMA_MIPSIV::m_cOpGeneral[MAX_GENERAL_OPS] =
{
	//0x00
	&CMA_MIPSIV::SPECIAL,		&CMA_MIPSIV::REGIMM,		&CMA_MIPSIV::J,				&CMA_MIPSIV::JAL,			&CMA_MIPSIV::BEQ,			&CMA_MIPSIV::BNE,			&CMA_MIPSIV::BLEZ,			&CMA_MIPSIV::BGTZ,
	//0x08
	&CMA_MIPSIV::ADDI,			&CMA_MIPSIV::ADDIU,			&CMA_MIPSIV::SLTI,			&CMA_MIPSIV::SLTIU,			&CMA_MIPSIV::ANDI,			&CMA_MIPSIV::ORI,			&CMA_MIPSIV::XORI,			&CMA_MIPSIV::LUI,
	//0x10
	&CMA_MIPSIV::COP0,			&CMA_MIPSIV::COP1,			&CMA_MIPSIV::COP2,			&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::BEQL,			&CMA_MIPSIV::BNEL,			&CMA_MIPSIV::BLEZL,			&CMA_MIPSIV::BGTZL,
	//0x18
	&CMA_MIPSIV::DADDI,			&CMA_MIPSIV::DADDIU,		&CMA_MIPSIV::LDL,			&CMA_MIPSIV::LDR,			&CMA_MIPSIV::SPECIAL2,		&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,
	//0x20
	&CMA_MIPSIV::LB,			&CMA_MIPSIV::LH,			&CMA_MIPSIV::LWL,			&CMA_MIPSIV::LW,			&CMA_MIPSIV::LBU,			&CMA_MIPSIV::LHU,			&CMA_MIPSIV::LWR,			&CMA_MIPSIV::LWU,
	//0x28
	&CMA_MIPSIV::SB,			&CMA_MIPSIV::SH,			&CMA_MIPSIV::SWL,			&CMA_MIPSIV::SW,			&CMA_MIPSIV::SDL,			&CMA_MIPSIV::SDR,			&CMA_MIPSIV::SWR,			&CMA_MIPSIV::CACHE,
	//0x30
	&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::LWC1,			&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::PREF,	    	&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::LDC2,			&CMA_MIPSIV::LD,
	//0x38
	&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::SWC1,			&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::SDC2,			&CMA_MIPSIV::SD,
};

CMA_MIPSIV::InstructionFuncConstant CMA_MIPSIV::m_cOpSpecial[MAX_SPECIAL_OPS] = 
{
	//0x00
	&CMA_MIPSIV::SLL,			&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::SRL,			&CMA_MIPSIV::SRA,			&CMA_MIPSIV::SLLV,			&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::SRLV,			&CMA_MIPSIV::SRAV,
	//0x08
	&CMA_MIPSIV::JR,			&CMA_MIPSIV::JALR,			&CMA_MIPSIV::MOVZ,			&CMA_MIPSIV::MOVN,			&CMA_MIPSIV::SYSCALL,		&CMA_MIPSIV::BREAK,			&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::SYNC,
	//0x10
	&CMA_MIPSIV::MFHI,			&CMA_MIPSIV::MTHI,			&CMA_MIPSIV::MFLO,			&CMA_MIPSIV::MTLO,			&CMA_MIPSIV::DSLLV,			&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::DSRLV,			&CMA_MIPSIV::DSRAV,
	//0x18
	&CMA_MIPSIV::MULT,			&CMA_MIPSIV::MULTU,			&CMA_MIPSIV::DIV,			&CMA_MIPSIV::DIVU,			&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,
	//0x20
	&CMA_MIPSIV::ADD,			&CMA_MIPSIV::ADDU,			&CMA_MIPSIV::SUB,			&CMA_MIPSIV::SUBU,			&CMA_MIPSIV::AND,			&CMA_MIPSIV::OR,			&CMA_MIPSIV::XOR,			&CMA_MIPSIV::NOR,
	//0x28
	&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::SLT,			&CMA_MIPSIV::SLTU,			&CMA_MIPSIV::DADD,  		&CMA_MIPSIV::DADDU,			&CMA_MIPSIV::DSUB,			&CMA_MIPSIV::DSUBU,
	//0x30
	&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::TEQ,			&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,
	//0x38
	&CMA_MIPSIV::DSLL,			&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::DSRL,			&CMA_MIPSIV::DSRA,			&CMA_MIPSIV::DSLL32,		&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::DSRL32,		&CMA_MIPSIV::DSRA32,
};

CMA_MIPSIV::InstructionFuncConstant CMA_MIPSIV::m_cOpRegImm[MAX_REGIMM_OPS] = 
{
	//0x00
	&CMA_MIPSIV::BLTZ,			&CMA_MIPSIV::BGEZ,			&CMA_MIPSIV::BLTZL,			&CMA_MIPSIV::BGEZL,			&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,
	//0x08
	&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::TEQI,			&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,
	//0x10
	&CMA_MIPSIV::BLTZAL,		&CMA_MIPSIV::BGEZAL,		&CMA_MIPSIV::BLTZALL,		&CMA_MIPSIV::BGEZALL,		&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,
	//0x18
	&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,		&CMA_MIPSIV::Illegal,
};
// clang-format on
